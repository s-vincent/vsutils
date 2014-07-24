/**
 * \file netevt_kqueue.c
 * \brief Network event kqueue implementation.
 * \author Sebastien Vincent
 * \date 2014
 */

#include <stdlib.h>
#include <errno.h>

#include "netevt_kqueue.h"

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)

#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include "list.h"
#include "util_net.h"

/**
 * \struct netevt_kqueue
 * \brief Kqueue network event implementation.
 */
struct netevt_kqueue
{
  int nsock; /**< Current number of sockets. */
  int kq; /**< Kqueue descriptor. */
  unsigned int fds_next; /**< Next index in mntrs array. */
  struct kevent mntrs[NET_SFD_SETSIZE]; /**< Array of monitor events. */
  struct kevent trgrd[NET_SFD_SETSIZE]; /**< Array of triggered events. */
};

/**
 * \brief Create a new network event implementation based on kqueue.
 * \return valid pointer if success, NULL otherwise.
 */
static struct netevt_kqueue* netevt_kqueue_new(void)
{
  struct netevt_kqueue* ret = malloc(sizeof(struct netevt_kqueue));

  if(!ret)
  {
    return NULL;
  }

  memset(ret, 0x00, sizeof(struct netevt_kqueue));

  ret->kq = kqueue();
  if(ret->kq == -1)
  {
    free(ret);
    return NULL;
  }

  ret->nsock = 0;

  return ret;
}

/**
 * \brief Create a new network event implementation based on kqueue.
 * \param obj pointer on network event implementation.
 */
static void netevt_kqueue_free(struct netevt_kqueue** obj)
{
  close((*obj)->kq);
  free(*obj);
  *obj = NULL;
}

/**
 * \brief Implementation specific with kqueue for the waiting of network event.
 * \param impl network event implementation.
 * \param obj network event manager.
 * \param timeout timeout in second.
 * \param events allocated array of netevt_event.
 * \param events_nb number of elements in events array.
 * \return 0 if timeout, number of elements notified if success and -1 if error.
 */
static int netevt_kqueue_wait(struct netevt_impl* impl, netevt obj, int timeout,
    struct netevt_event* events, size_t nb_events)
{
  int ret = 0;
  struct netevt_kqueue* impl_kqueue = impl->priv;
  struct timespec ts;

  /* -1 is infinite */
  if(timeout != -1)
  {
    ts.tv_sec = timeout;
    ts.tv_nsec = 0;
  }

  ret = kevent(impl_kqueue->kq, impl_kqueue->mntrs, impl_kqueue->nsock,
      impl_kqueue->trgrd, NET_SFD_SETSIZE,
      ((timeout != -1) ? &ts : NULL));

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
    for(unsigned int i = 0 ; i < ret ; i++)
    {
      int already = 0;

      /* TODO EV_EOF / EV_ERROR */

      /* 0 = check read state
       * 1 = check write state
       */
      for(unsigned int j = 0 ; j < 3 ; j++)
      {
        int evt = 0;
        int state = 0;
        int extra = 0;

        if(nb > nb_events)
        {
          return nb;
        }

        if(j == 0)
        {
          /* read */
          evt = EVFILT_READ;
          state = NETEVT_STATE_READ;
        }
        else if(j == 1)
        {
          /* write */
          evt = EVFILT_WRITE;
          state = NETEVT_STATE_WRITE;
        }
        else if(j == 2)
        {
          evt = EVFILT_READ;
          extra = EV_OOBAND; 
          state = NETEVT_STATE_OTHER;
        }

        if(impl_kqueue->trgrd[i].filter == evt &&
          (extra == 0 ? 1 : (impl_kqueue->trgrd[i].flags & EV_OOBAND)))
        {
          if(!already)
          {
            events[nb].socket = *((struct netevt_socket*)impl_kqueue->trgrd[i].udata);
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
static int netevt_kqueue_add_socket(struct netevt_impl* impl, netevt obj,
    struct netevt_socket* sock, int event_mask)
{
  int ret = 0;
  struct netevt_kqueue* impl_kqueue = impl->priv;
  int evt = 0;
  int extra = 0;

  if(impl_kqueue->fds_next >= NET_SFD_SETSIZE)
  {
    return -1;
  }

  if(event_mask & NETEVT_STATE_READ)
  {
    evt |= EVFILT_READ;
  }
  if(event_mask & NETEVT_STATE_WRITE)
  {
    evt |= EVFILT_WRITE;
  }
  if(event_mask & NETEVT_STATE_OTHER)
  {
    evt |= EVFILT_READ;
    extra = EV_OOBAND;
  }

  EV_SET(&impl_kqueue->mntrs[impl_kqueue->fds_next], sock->sock, evt,
      EV_ADD | EV_ENABLE, extra, 0, sock); 
  impl_kqueue->fds_next++;
  impl_kqueue->nsock++;

  return ret;
}

/**
 * \brief Remove a socket from the manager.
 * \param impl network event implementation.
 * \param obj network event manager.
 * \param sock socket descriptor.
 * \return 0 if success, -1 otherwise.
 */
static int netevt_kqueue_remove_socket(struct netevt_impl* impl, netevt obj,
    struct netevt_socket* sock)
{
  int ret = 0;
  struct netevt_kqueue* impl_kqueue = impl->priv;
  unsigned int i = 0;

  for(i = 0 ; i < NET_SFD_SETSIZE ; i++)
  {
    if(impl_kqueue->mntrs[i].ident == sock->sock)
    {
      EV_SET(&impl_kqueue->mntrs[i], sock->sock,
          EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0,
          0, sock); 
      kevent(impl_kqueue->kq, &impl_kqueue->mntrs[i], 1, NULL, 0, NULL);
      break;
    }
  }

  if(i == NET_SFD_SETSIZE)
  {
    /* not found */
    return -1;
  }

  impl_kqueue->nsock--;
  impl_kqueue->fds_next--;

  if(i < impl_kqueue->nsock)
  {
    /* reorder array */
    for(unsigned int j = i ; j < impl_kqueue->nsock ; j++)
    {
      impl_kqueue->mntrs[j] = impl_kqueue->mntrs[j + 1];
    }
  }

  return ret;
}

int netevt_kqueue_init(struct netevt_impl* impl)
{
  int ret = -1;
  struct netevt_kqueue* impl_kqueue = netevt_kqueue_new();

  if(impl_kqueue)
  {
    impl->wait = netevt_kqueue_wait;
    impl->add_socket = netevt_kqueue_add_socket;
    impl->remove_socket = netevt_kqueue_remove_socket;
    impl->priv = impl_kqueue;
    ret = 0;
  }

  return ret;
}

int netevt_kqueue_destroy(struct netevt_impl* impl)
{
  impl->wait = NULL;
  impl->add_socket = NULL;
  impl->remove_socket = NULL;
  netevt_kqueue_free((struct netevt_kqueue**)&impl->priv);
  impl->priv = NULL;

  return 0;
}

#else

int netevt_kqueue_init(struct netevt_impl* impl)
{
  (void)impl;
  errno = ENOSYS;
  return -1;
}

int netevt_kqueue_destroy(struct netevt_impl* impl)
{
  (void)impl;
  errno = ENOSYS;
  return -1;
}

#endif /* __FreeBSD || __OpenBSD__ || __NetBSD__ || __APPLE__ */

