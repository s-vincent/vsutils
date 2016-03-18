/**
 * \file netevt_impl.h
 * \brief Netevt implementation defintion.
 * \author Sebastien Vincent
 * \date 2016
 */

#ifndef VSUTILS_NETEVT_IMPL_H
#define VSUTILS_NETEVT_IMPL_H

#include "netevt.h"

/**
 * \struct netevt_impl
 * \brief Event method specific (select, poll, ...) implementation.
 */
struct netevt_impl
{
  /**
   * \brief Wait network events.
   */
  int (*wait)(struct netevt_impl* impl, netevt obj, int timeout,
      struct netevt_event* events, size_t nb_events);

  /**
   * \brief Add a socket.
   */
  int (*add_socket)(struct netevt_impl* impl, netevt obj,
      struct netevt_socket* sock, int event_mask);

  /**
   * \brief Remove a socket.
   */
  int (*remove_socket)(struct netevt_impl* impl, netevt obj,
      struct netevt_socket* sock);

  /**
   * \brief Private data for the implementation.
   */
  void* priv;
};

#endif /* VSUTILS_NETEVT_IMPL_H */

