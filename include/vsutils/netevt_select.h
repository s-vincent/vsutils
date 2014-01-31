/**
 * \file netevt_select.h
 * \brief Network event select implementation.
 * \author Sebastien Vincent
 * \date 2014
 */

#ifndef VSUTILS_NETEVT_SELECT_H
#define VSUTILS_NETEVT_SELECT_H

#include "netevt.h"

/**
 * \brief Initialize netevt_impl structure with select implementation.
 * \param impl structure to initialize.
 * \return 0 if success, -1 otherwise.
 */
int netevt_select_init(struct netevt_impl* impl);

/**
 * \brief Destroy netevt_impl structure.
 * \param impl structure to destroy.
 * \return 0 if success, -1 otherwise.
 */
int netevt_select_destroy(struct netevt_impl* impl);

#endif /* VSUTILS_NETEVT_SELECT_H */

