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
 * \file ipc_shm_impl.h
 * \brief IPC shared memory definition.
 * \author Sebastien Vincent.
 * \date 2016
 */

#ifndef IPC_SHM_IMPL_H
#define IPC_SHM_IMPL_H

#include "ipc_shm.h"

/**
 * \struct ipc_shm
 * \brief IPC shared memory.
 */
struct ipc_shm
{
    /**
     * \brief Shared memory type.
     */
    enum ipc_shm_type type;

    /**
     * \brief Shared memory data.
     */
    void* data;
   
    /**
     * \brief Shared memory size.
     */
    size_t data_size;

    /**
     * \brief Function to free the object.
     */
    void (*free)(ipc_shm*, int);

    /**
     * \brief Function to get shared data pointer.
     */
    void* (*get_data)(struct ipc_shm* obj);

    /**
     * \brief Function to get shared data size.
     */
    size_t (*get_data_size)(struct ipc_shm* obj);

    /**
     * \brief Private implementation.
     */
    char priv[];
};

#endif /* IPC_SHM_IMPL_H */

