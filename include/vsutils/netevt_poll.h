/**
 * \file netevt_poll.h
 * \brief Network event poll implementation.
 * \author Sebastien Vincent
 * \date 2014
 */

#ifndef VSUTILS_NETEVT_POLL_H
#define VSUTILS_NETEVT_POLL_H

#include "netevt.h"

/**
 * \brief Initialize netevt_impl structure with poll implementation.
 * \param impl structure to initialize.
 * \return 0 if success, -1 otherwise.
 */
int netevt_poll_init(struct netevt_impl* impl);

/**
 * \brief Destroy netevt_impl structure.
 * \param impl structure to destroy.
 * \return 0 if success, -1 otherwise.
 */
int netevt_poll_destroy(struct netevt_impl* impl);

#endif /* VSUTILS_NETEVT_POLL_H */

