/**
 * \file netevt_kqueue.h
 * \brief Network event kqueue implementation.
 * \author Sebastien Vincent
 * \date 2014
 */

#ifndef VSUTILS_NETEVT_KQUEUE_H
#define VSUTILS_NETEVT_KQUEUE_H

#include "netevt.h"

/**
 * \brief Initialize netevt_impl structure with kqueue implementation.
 * \param impl structure to initialize.
 * \return 0 if success, -1 otherwise.
 */
int netevt_kqueue_init(struct netevt_impl* impl);

/**
 * \brief Destroy netevt_impl structure.
 * \param impl structure to destroy.
 * \return 0 if success, -1 otherwise.
 */
int netevt_kqueue_destroy(struct netevt_impl* impl);

#endif /* VSUTILS_NETEVT_KQUEUE_H */

