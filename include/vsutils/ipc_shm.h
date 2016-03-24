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
 * \file ipc_shm.h
 * \brief IPC shared memory.
 * \author Sebastien Vincent
 * \date 2016
 */

#ifndef VSUTILS_IPC_SHM_H
#define VSUTILS_IPC_SHM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \enum ipc_shm_type
 * \brief Enumerations for shared memory type.
 */
enum ipc_shm_type
{
    IPC_SHM_SYSV, /**< SystemV shared memory IPC. */
    IPC_SHM_POSIX, /**< POSIX shared memory IPC. */
    IPC_SHM_WIN /**< Windows shared memory IPC. */
};

/**
 * \typedef ipc_shm
 * \brief Typedef for IPC shared memory.
 */
typedef struct ipc_shm* ipc_shm;

/**
 * \brief Gets a new shared memory object.
 * \param type type of shared memory (POSIX, SYSV or Windows).
 * \param value opaque value. It can be of several types:
 * - For POSIX: string in the form "/my_mq_name";
 * - For System V: key_t value obtained via ftok() or magic cookie value.
 * \param mode (O_RDONLY, O_RDWR or O_WRONLY).
 * \param perm permissions (bitfield of S_IRUSR, S_IWUSR, S_IRGRP, S_IWGRP,
 * S_IROTH or S_IWOTH).
 * \param size size of the shared memory.
 * \return IPC shared memory pointer.
 * \warning Some parameters of permission may be ignored for IPC_SHM_WIN type.
 */
ipc_shm ipc_shm_new(enum ipc_shm_type type, void* value, int mode, int perm,
        size_t size);

/**
 * \brief Closes and frees IPC shared memory object.
 * \param obj IPC shared memory object.
 * \param unlink positive value to destroy the shared memory.
 */
void ipc_shm_free(ipc_shm* obj, int unlink);

/**
 * \brief Returns data pointer.
 * \param obj IPC shared memory object.
 * \return shared data pointer.
 */
void* ipc_shm_get_data(ipc_shm obj);

/**
 * \brief Returns data size.
 * \param obj IPC shared memory object.
 * \return shared data size.
 */
size_t ipc_shm_get_data_size(ipc_shm obj);

/**
 * \brief Send data in shared memory.
 * \param type IPC shared memory type.
 * \return 1 if supported, 0 otherwise.
 */
int ipc_shm_is_supported(enum ipc_shm_type type);

/**
 * \brief Returns "best" shared memory type for the current OS.
 * \return Best shared memory type for the current OS.
 */
enum ipc_shm_type ipc_shm_get_best_type();

#ifdef __cplusplus
}
#endif

#endif /* VSUTILS_IPC_SHM_H */

