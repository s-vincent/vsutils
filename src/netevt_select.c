/**
 * \file netevt_select.c
 * \brief Network event select implementation.
 * \author Sebastien Vincent
 * \date 2014
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/time.h>
#include <sys/select.h>

#include "list.h"
#include "util_net.h"
#include "netevt_select.h"

/**
 * \struct netevt_select
 * \brief Select network event implementation.
 */
struct netevt_select
{
  int nsock; /**< Maximum socket descriptor + 1. */
  sfd_set fdsr; /**< Read set. */
  sfd_set fdsw; /**< Write set. */
  sfd_set fdse; /**< Exception set. */
};

/**
 * \brief Create a new network event implementation based on select.
 * \return valid pointer if success, NULL otherwise.
 */
static struct netevt_select* netevt_select_new(void)
{
  struct netevt_select* ret = malloc(sizeof(struct netevt_select));

  if(!ret)
  {
    return NULL;
  }

  NET_SFD_ZERO(&ret->fdsr);
  NET_SFD_ZERO(&ret->fdsw);
  NET_SFD_ZERO(&ret->fdse);

  return ret;
}

/**
 * \brief Create a new network event implementation based on select.
 * \param obj pointer on network event implementation.
 */
static void netevt_select_free(struct netevt_select** obj)
{
  free(*obj);
  *obj = NULL;
}

/**
 * \brief Implementation specific with select for the waiting of network event.
 * \param impl network event implementation.
 * \param obj network event manager.
 * \param timeout timeout in second.
 * \param events allocated array of netevt_event.
 * \param nb_events number of elements in events array.
 * \return 0 if timeout, number of elements notified if success and -1 if error.
 */
static int netevt_select_wait(struct netevt_impl* impl, netevt obj, int timeout,
    struct netevt_event* events, size_t nb_events)
{
  int ret = 0;
  struct netevt_select* impl_select = impl->priv;
  struct timeval tv;
  struct list_head* sockets = netevt_get_sockets_list(obj);
  sfd_set fdsr = impl_select->fdsr;
  sfd_set fdsw = impl_select->fdsw;
  sfd_set fdse = impl_select->fdse;

  /* -1 is infinite */
  if(timeout != -1)
  {
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
  }

  ret = select(NET_SFD_SETSIZE, (fd_set*)&fdsr,
      (fd_set*)&fdsw, (fd_set*)&fdse,
      (timeout != -1 ? &tv : NULL));

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
  else /* ret > 0 */
  {
    /* at least one descriptor is ready for read, write or other */
    struct list_head* pos = NULL;
    struct list_head* tmp = NULL;
    size_t nb = 0;

    list_head_iterate_safe(sockets, pos, tmp)
    {
      struct netevt_socket* s = list_head_get(pos, struct netevt_socket, list);
      /* if entry has alreay an event set */
      int already = 0;

      /* 0 = check read state
       * 1 = check write state
       * 2 = check exception state
       */
      for(unsigned int i = 0 ; i < 3 ; i++)
      {
        fd_set* fds = NULL;
        int state = 0;

        if(nb > nb_events)
        {
          return nb;
        }

        if(i == 0)
        {
          /* read */
          fds = (fd_set*)&fdsr;
          state = NETEVT_STATE_READ;
        }
        else if(i == 1)
        {
          /* write */
          fds = (fd_set*)&fdsw;
          state = NETEVT_STATE_WRITE;
        }
        else if(i == 2)
        {
          /* exception */
          fds = (fd_set*)&fdse;
          state = NETEVT_STATE_OTHER;
        }

        if(NET_SFD_ISSET(s->sock, fds))
        {
          if(!already)
          {
            events[nb].socket = *s;
            events[nb].state = state;
          }
          else
          {
            events[nb].state |= state;
          }
          already = 1;
        }
      }

      if(already)
      {
        nb++;
      }

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
static int netevt_select_add_socket(struct netevt_impl* impl, netevt obj,
    struct netevt_socket* sock, int event_mask)
{
  int ret = 0;
  struct netevt_select* impl_select = impl->priv;

  (void)obj;

  if(sock->sock >= NET_SFD_SETSIZE)
  {
    return -1;
  }

  if(event_mask & NETEVT_STATE_READ)
  {
    NET_SFD_SET(sock->sock, &impl_select->fdsr);
  }
  if(event_mask & NETEVT_STATE_WRITE)
  {
    NET_SFD_SET(sock->sock, &impl_select->fdsw);
  }
  if(event_mask & NETEVT_STATE_OTHER)
  {
    NET_SFD_SET(sock->sock, &impl_select->fdse);
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
static int netevt_select_remove_socket(struct netevt_impl* impl, netevt obj,
    struct netevt_socket* sock)
{
  int ret = 0;
  struct netevt_select* impl_select = impl->priv;

  (void)obj;

  if(sock->sock >= NET_SFD_SETSIZE)
  {
    return -1;
  }

  if(NET_SFD_ISSET(sock->sock, &impl_select->fdsr))
  {
    NET_SFD_CLR(sock->sock, &impl_select->fdsr);
  }
  if(NET_SFD_ISSET(sock->sock, &impl_select->fdsw))
  {
    NET_SFD_CLR(sock->sock, &impl_select->fdsw);
  }
  if(NET_SFD_ISSET(sock->sock, &impl_select->fdse))
  {
    NET_SFD_CLR(sock->sock, &impl_select->fdse);
  }

  return ret;
}

int netevt_select_init(struct netevt_impl* impl)
{
  int ret = -1;
  struct netevt_select* impl_select = netevt_select_new();

  if(impl_select)
  {
    impl->wait = netevt_select_wait;
    impl->add_socket = netevt_select_add_socket;
    impl->remove_socket = netevt_select_remove_socket;
    impl->priv = impl_select;
    ret = 0;
  }

  return ret;
}

int netevt_select_destroy(struct netevt_impl* impl)
{
  impl->wait = NULL;
  impl->add_socket = NULL;
  impl->remove_socket = NULL;
  netevt_select_free((struct netevt_select**)&impl->priv);
  impl->priv = NULL;

  return 0;
}

