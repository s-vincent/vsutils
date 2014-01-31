/**
 * \file netevt_epoll.c
 * \brief Network event epoll implementation.
 * \author Sebastien Vincent
 * \date 2014
 */

#include <stdlib.h>
#include <errno.h>

#include "netevt_epoll.h"

#ifdef __linux__

#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/time.h>
#include <sys/epoll.h>

#include "list.h"
#include "util_net.h"

/**
 * \struct netevt_epoll
 * \brief Epoll network event implementation.
 */
struct netevt_epoll
{
  int nsock; /**< Current number of sockets. */
  int efd; /**< Epoll descriptor. */
  struct epoll_event events[NET_SFD_SETSIZE]; /**< Array of epoll events. */
};

/**
 * \brief Create a new network event implementation based on epoll.
 * \return valid pointer if success, NULL otherwise.
 */
static struct netevt_epoll* netevt_epoll_new(void)
{
  struct netevt_epoll* ret = malloc(sizeof(struct netevt_epoll));

  if(!ret)
  {
    return NULL;
  }

  memset(ret, 0x00, sizeof(struct netevt_epoll));

  ret->efd = epoll_create(NET_SFD_SETSIZE);
  if(ret->efd == -1)
  {
    free(ret);
    return NULL;
  }

  ret->nsock = 0;

  return ret;
}

/**
 * \brief Create a new network event implementation based on epoll.
 * \param obj pointer on network event implementation.
 */
static void netevt_epoll_free(struct netevt_epoll** obj)
{
  close((*obj)->efd);
  free(*obj);
  *obj = NULL;
}

/**
 * \brief Implementation specific with epoll for the waiting of network event.
 * \param impl network event implementation.
 * \param obj network event manager.
 * \param timeout timeout in second.
 * \param events allocated array of netevt_event.
 * \param events_nb number of elements in events array.
 * \return 0 if timeout, number of elements notified if success and -1 if error.
 */
static int netevt_epoll_wait(struct netevt_impl* impl, netevt obj, int timeout,
    struct netevt_event* events, size_t nb_events)
{
  int ret = 0;
  struct netevt_epoll* impl_epoll = impl->priv;
  /* -1 is infinite */
  int timeout_ms = timeout != -1 ? timeout * 1000 : -1;

  (void)obj;

  ret = epoll_wait(impl_epoll->efd, impl_epoll->events, NET_SFD_SETSIZE,
      timeout_ms);

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
    size_t nb = 0;

    /* at least one descriptor is ready for read, write or other */
    for(int i = 0 ; i < ret ; i++)
    {
      int already = 0;

      if(impl_epoll->events[i].events == 0)
      {
        continue;
      }

      /* TODO error */

      /* 0 = check read state
       * 1 = check write state
       * 2 = check exception state
       */
      for(unsigned int j = 0 ; j < 3 ; j++)
      {
        int evt = 0;
        int state = 0;

        if(nb > nb_events)
        {
          return nb;
        }

        if(j == 0)
        {
          /* read */
          evt = EPOLLIN;
          state = NETEVT_STATE_READ;
        }
        else if(j == 1)
        {
          /* write */
          evt = EPOLLOUT;
          state = NETEVT_STATE_WRITE;
        }
        else if(j == 2)
        {
          /* out-of-band */
          evt = EPOLLPRI;
          state = NETEVT_STATE_OTHER;
        }

        if(impl_epoll->events[i].events & evt)
        {
          if(!already)
          {
            events[nb].socket = *((struct netevt_socket*)impl_epoll->events[i].data.ptr);
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
static int netevt_epoll_add_socket(struct netevt_impl* impl, netevt obj,
    struct netevt_socket* sock, int event_mask)
{
  int ret = 0;
  struct netevt_epoll* impl_epoll = impl->priv;
  struct epoll_event evt;

  (void)obj;

  if(sock->sock > NET_SFD_SETSIZE)
  {
    return -1;
  }

  memset(&evt, 0x00, sizeof(struct epoll_event));
  evt.events = 0;
  evt.data.ptr = sock;

  if(event_mask & NETEVT_STATE_READ)
  {
    evt.events |= EPOLLIN;
  }
  if(event_mask & NETEVT_STATE_WRITE)
  {
    evt.events |= EPOLLOUT;
  }
  if(event_mask & NETEVT_STATE_OTHER)
  {
    evt.events |= EPOLLPRI;
  }

  ret = epoll_ctl(impl_epoll->efd, EPOLL_CTL_ADD, sock->sock, &evt); 

  if(ret != -1)
  {
    impl_epoll->nsock++;
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
static int netevt_epoll_remove_socket(struct netevt_impl* impl, netevt obj,
    struct netevt_socket* sock)
{
  int ret = 0;
  struct netevt_epoll* impl_epoll = impl->priv;

  (void)obj;

  if(sock->sock > NET_SFD_SETSIZE)
  {
    return -1;
  }

  epoll_ctl(impl_epoll->efd, EPOLL_CTL_DEL, sock->sock, NULL);

  return ret;
}

int netevt_epoll_init(struct netevt_impl* impl)
{
  int ret = -1;
  struct netevt_epoll* impl_epoll = netevt_epoll_new();

  if(impl_epoll)
  {
    impl->wait = netevt_epoll_wait;
    impl->add_socket = netevt_epoll_add_socket;
    impl->remove_socket = netevt_epoll_remove_socket;
    impl->priv = impl_epoll;
    ret = 0;
  }

  return ret;
}

int netevt_epoll_destroy(struct netevt_impl* impl)
{
  impl->wait = NULL;
  impl->add_socket = NULL;
  impl->remove_socket = NULL;
  netevt_epoll_free((struct netevt_epoll**)&impl->priv);
  impl->priv = NULL;

  return 0;
}

#else

int netevt_epoll_init(struct netevt_impl* impl)
{
  (void)impl;
  errno = ENOSYS;
  return -1;
}

int netevt_epoll_destroy(struct netevt_impl* impl)
{
  (void)impl;
  errno = ENOSYS;
  return -1;
}

#endif /* __linux__ */

