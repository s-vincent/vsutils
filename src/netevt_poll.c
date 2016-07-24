/*
 * Copyright (C) 2014 Sebastien Vincent.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * \file netevt_poll.c
 * \brief Network event poll implementation.
 * \author Sebastien Vincent
 * \date 2014-2016
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/time.h>
#include <sys/poll.h>

#include "list.h"
#include "util_net.h"
#include "netevt_poll.h"

/**
 * \struct netevt_poll
 * \brief Poll network event implementation.
 */
struct netevt_poll
{
  int nsock; /**< Current number of sockets. */
  unsigned int fds_next; /**< Next free index of fds array. */
  struct pollfd fds[NET_SFD_SETSIZE]; /**< Poll structure to handle event. */
};

/**
 * \brief Create a new network event implementation based on poll.
 * \return valid pointer if success, NULL otherwise.
 */
static struct netevt_poll* netevt_poll_new(void)
{
  struct netevt_poll* ret = malloc(sizeof(struct netevt_poll));

  if(!ret)
  {
    return NULL;
  }

  memset(ret, 0x00, sizeof(struct netevt_poll));
  ret->nsock = 0;
  ret->fds_next = 0;

  return ret;
}

/**
 * \brief Create a new network event implementation based on poll.
 * \param obj pointer on network event implementation.
 */
static void netevt_poll_free(struct netevt_poll** obj)
{
  free(*obj);
  *obj = NULL;
}

/**
 * \brief Implementation specific with poll for the waiting of network event.
 * \param impl network event implementation.
 * \param obj network event manager.
 * \param timeout timeout in second.
 * \param events allocated array of netevt_event.
 * \param nb_events number of elements in events array.
 * \return 0 if timeout, number of elements notified if success and -1 if error.
 */
static int netevt_poll_wait(struct netevt_impl* impl, netevt obj, int timeout,
    struct netevt_event* events, size_t nb_events)
{
  int ret = 0;
  struct netevt_poll* impl_poll = impl->priv;
  struct list_head* sockets = netevt_get_sockets_list(obj);
  /* -1 is infinite */
  int timeout_ms = timeout != -1 ? timeout * 1000 : -1;

  ret = poll(impl_poll->fds, impl_poll->nsock, timeout_ms);

  if(ret == -1)
  {
    /* error */
    return -1;
  }
  else if(ret == 0)
  {
    /* timeout */
    return 0;
  }
  else if(ret > 0)
  {
    /* at least one descriptor is ready for read, write or other */
    struct list_head* pos = NULL;
    struct list_head* tmp = NULL;
    unsigned int idx = 0;
    size_t nb = 0;

    list_head_iterate_safe(sockets, pos, tmp)
    {
      struct netevt_socket* s = list_head_get(pos, struct netevt_socket,
          list);
      int already = 0;

      if(impl_poll->fds[idx].revents == 0)
      {
        /* no events to go the next */
        idx++;
        continue;
      }

      /* 0 = check read state
       * 1 = check write state
       * 2 = check exception state
       */
      for(unsigned int i = 0 ; i < 3 ; i++)
      {
        int evt = 0;
        int state = 0;

        if(nb > nb_events)
        {
          return nb;
        }

        if(i == 0)
        {
          /* read */
          evt = POLLIN;
          state = NETEVT_STATE_READ;
        }
        else if(i == 1)
        {
          /* write */
          evt = POLLOUT;
          state = NETEVT_STATE_WRITE;
        }
        else if(i == 2)
        {
          /* out-of-band */
          evt = POLLPRI;
          state = NETEVT_STATE_OTHER;
        }

        if(impl_poll->fds[idx].revents & evt)
        {
          if(!already)
          {
            events[nb].socket = *s;
            events[nb].ptr = s;
            events[nb].state = state;
            already = 1;
          }
          else
          {
            events[nb].state |= state;
          }
        }
      }

      if(already)
      {
        nb++;
      }
      idx++;
    }
  }

  return ret;
}

/**
 * \brief Add a socket to monitore by the manager.
 * \param impl network event implementation.
 * \param obj network event manager.
 * \param sock socket descriptor.
 * \param event_mask combination of NETEVT_STATE_* flag (read, write, ...).
 * \return 0 if success, -1 otherwise.
 */
static int netevt_poll_add_socket(struct netevt_impl* impl, netevt obj,
    struct netevt_socket* sock, int event_mask)
{
  int ret = 0;
  struct netevt_poll* impl_poll = impl->priv;
  unsigned int idx = impl_poll->fds_next;

  (void)obj;

  if(idx >= NET_SFD_SETSIZE)
  {
    return -1;
  }

  impl_poll->fds[idx].fd = sock->sock;
  impl_poll->fds[idx].events = 0;

  if(event_mask & NETEVT_STATE_READ)
  {
    impl_poll->fds[idx].events |= POLLIN;
  }
  if(event_mask & NETEVT_STATE_WRITE)
  {
    impl_poll->fds[idx].events |= POLLOUT;
  }
  if(event_mask & NETEVT_STATE_OTHER)
  {
    impl_poll->fds[idx].events |= POLLPRI;
  }

  impl_poll->fds_next++;
  impl_poll->nsock++;

  return ret;
}

/**
 * \brief Modify a socket.
 * \param impl network event implementation.
 * \param obj network event manager.
 * \param sock socket descriptor.
 * \param event_mask combination of NETEVT_STATE_* flag (read, write, ...).
 * \return 0 if success, -1 otherwise.
 */
static int netevt_poll_set_socket(struct netevt_impl* impl, netevt obj,
    struct netevt_socket* sock, int event_mask)
{
  int ret = 0;
  struct netevt_poll* impl_poll = impl->priv;
  unsigned int i = 0;

  (void)obj;

  for(i = 0 ; i < NET_SFD_SETSIZE ; i++)
  {
    if(impl_poll->fds[i].fd == sock->sock)
    {
      /* reset and modify the event mask */
      impl_poll->fds[i].events = 0;

      if(event_mask & NETEVT_STATE_READ)
      {
        impl_poll->fds[i].events |= POLLIN;
      }
      if(event_mask & NETEVT_STATE_WRITE)
      {
        impl_poll->fds[i].events |= POLLOUT;
      }
      if(event_mask & NETEVT_STATE_OTHER)
      {
        impl_poll->fds[i].events |= POLLPRI;
      }
      ret = 0;
      break;
    }
  }

  if(i == NET_SFD_SETSIZE)
  {
    /* not found */
    ret = -1;
  }

  return ret;
}

/**
 * \brief Remove a socket from the manager.
 * \param impl network event implementation.
 * \param obj network event manager.
 * \param sock socket descriptor.
 * \return 0 if success, -1 otherwise.
 */
static int netevt_poll_remove_socket(struct netevt_impl* impl, netevt obj,
    struct netevt_socket* sock)
{
  int ret = 0;
  struct netevt_poll* impl_poll = impl->priv;
  unsigned int i = 0;

  (void)obj;

  for(i = 0 ; i < NET_SFD_SETSIZE ; i++)
  {
    if(impl_poll->fds[i].fd == sock->sock)
    {
      impl_poll->fds[i].fd = -1;
      impl_poll->fds[i].events = 0;
      break;
    }
  }

  if(i == NET_SFD_SETSIZE)
  {
    /* not found */
    return -1;
  }

  impl_poll->nsock--;
  impl_poll->fds_next--;

  if(i < (unsigned int)impl_poll->nsock)
  {
    /* reorder array */
    for(unsigned int j = i ; j < (unsigned int)impl_poll->nsock ; j++)
    {
      impl_poll->fds[j].fd = impl_poll->fds[j + 1].fd;
      impl_poll->fds[j].events = impl_poll->fds[j + 1].events;
    }
  }

  return ret;
}

int netevt_poll_init(struct netevt_impl* impl)
{
  int ret = -1;
  struct netevt_poll* impl_poll = netevt_poll_new();

  if(impl_poll)
  {
    impl->wait = netevt_poll_wait;
    impl->add_socket = netevt_poll_add_socket;
    impl->set_socket = netevt_poll_set_socket;
    impl->remove_socket = netevt_poll_remove_socket;
    impl->priv = impl_poll;
    ret = 0;
  }

  return ret;
}

int netevt_poll_destroy(struct netevt_impl* impl)
{
  impl->wait = NULL;
  impl->add_socket = NULL;
  impl->remove_socket = NULL;
  netevt_poll_free((struct netevt_poll**)&impl->priv);
  impl->priv = NULL;

  return 0;
}

