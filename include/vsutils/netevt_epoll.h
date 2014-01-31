/**
 * \file netevt_epoll.h
 * \brief Network event epoll implementation.
 * \author Sebastien Vincent
 * \date 2014
 */

#ifndef VSUTILS_NETEVT_EPOLL_H
#define VSUTILS_NETEVT_EPOLL_H

#include "netevt.h"

/**
 * \brief Initialize netevt_impl structure with epoll implementation.
 * \param impl structure to initialize.
 * \return 0 if success, -1 otherwise.
 */
int netevt_epoll_init(struct netevt_impl* impl);

/**
 * \brief Destroy netevt_impl structure.
 * \param impl structure to destroy.
 * \return 0 if success, -1 otherwise.
 */
int netevt_epoll_destroy(struct netevt_impl* impl);

#endif /* VSUTILS_NETEVT_EPOLL_H */

