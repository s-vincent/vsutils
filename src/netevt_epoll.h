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
 * \file netevt_epoll.h
 * \brief Network event epoll implementation.
 * \author Sebastien Vincent
 * \date 2014
 */

#ifndef VSUTILS_NETEVT_EPOLL_H
#define VSUTILS_NETEVT_EPOLL_H

#include "netevt.h"
#include "netevt_impl.h"

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

