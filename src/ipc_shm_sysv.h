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
 * \file ipc_shm_sysv.h
 * \brief Shared memory implementation for System V.
 * \author Sebastien Vincent
 * \date 2016
 */

#ifndef IPC_SHM_SYSV_H
#define IPC_SHM_SYSV_H

#include "ipc_shm.h"
#include "ipc_shm_impl.h"

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \brief Gets a new shared memory object.
 * \param value value obtained via ftok() or magic cookie value.
 * \param mode (O_RDONLY, O_RDWR or O_WRONLY).
 * \param perm permissions (bitfield of S_IRUSR, S_IWUSR, S_IRGRP, S_IWGRP,
 * S_IROTH or S_IWOTH).
 * \param size size of the shared memory.
 * \return System V shared memory pointer.
 */
ipc_shm ipc_shm_sysv_new(void* value, int mode, int perm, size_t size);

/**
 * \brief Frees IPC shared memory object.
 * \param obj IPC shared memory object.
 * \param unlink positive value to destroy the shared memory.
 */
void ipc_shm_sysv_free(ipc_shm* obj, int unlink);

/**
 * \brief Returns data pointer.
 * \param obj IPC shared memory object.
 * \return shared data pointer.
 */
void* ipc_shm_sysv_get_data(ipc_shm obj);

/**
 * \brief Returns data size.
 * \param obj IPC shared memory object.
 * \return shared data size.
 */
size_t ipc_shm_sysv_get_data_size(ipc_shm obj);

#ifdef __cplusplus
}
#endif

#endif /* IPC_SHM_SYSV_H */

