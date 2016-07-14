/*
 * Copyright (C) 2016 Sebastien Vincent.
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
 * \file ipc_sem_impl.h
 * \brief IPC semaphore definition.
 * \author Sebastien Vincent.
 * \date 2016
 */

#ifndef IPC_SEM_IMPL_H
#define IPC_SEM_IMPL_H

#include "ipc_sem.h"

/**
 * \struct ipc_sem
 * \brief IPC semaphore.
 */
struct ipc_sem
{
    /**
     * \brief Semaphore type.
     */
    enum ipc_sem_type type;

    /**
     * \brief Function to free the object.
     */
    void (*free)(ipc_sem*, int);

    /**
     * \brief Function to lock a semaphore.
     */
    int (*lock)(ipc_sem);

    /**
     * \brief Function to unlock a semaphore.
     */
    int (*unlock)(ipc_sem);

    /**
     * \brief Private implementation.
     */
    char priv[];
};


#endif /* IPC_SEM_IMPL_H */

