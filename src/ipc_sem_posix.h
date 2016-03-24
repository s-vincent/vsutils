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
 * \file ipc_sem_posix.h
 * \brief Semaphore implementation for POSIX.
 * \author Sebastien Vincent
 * \date 2016
 */

#ifndef IPC_SEM_POSIX_H
#define IPC_SEM_POSIX_H

#include "ipc_sem.h"
#include "ipc_sem_impl.h"

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \brief Gets a new semaphore object.
 * \param value name of the semaphore in the form "/name".
 * \param mode (O_RDONLY, O_RDWR or O_WRONLY).
 * \param perm permissions (bitfield of S_IRUSR, S_IWUSR, S_IRGRP, S_IWGRP,
 * S_IROTH or S_IWOTH).
 * \param init initial value for the semaphore.
 * \return POSIX semaphore pointer.
 */
ipc_sem ipc_sem_posix_new(void* value, int mode, int perm, unsigned int init);

/**
 * \brief Frees IPC semaphore object.
 * \param obj IPC semaphore object.
 * \param unlink positive value to destroy the semaphore.
 */
void ipc_sem_posix_free(ipc_sem* obj, int unlink);

/**
 * \brief Lock a semaphore (decrement value).
 * \param obj IPC semaphore object.
 * \return 0 if success, -1 otherwise.
 */
int ipc_sem_posix_lock(ipc_sem obj);

/**
 * \brief Lock a semaphore (decrement value).
 * \param obj IPC semaphore object.
 * \param timeout timeout.
 * \return 0 if success, -1 otherwise.
 */
int ipc_sem_posix_lock_timed(ipc_sem obj, const struct timespec* timeout);

/**
 * \brief Unlock a semaphore (increment value).
 * \param obj IPC semaphore object.
 * \return 0 if success, -1 otherwise.
 */
int ipc_sem_posix_unlock(ipc_sem obj);

#ifdef __cplusplus
}
#endif

#endif /* IPC_SEM_POSIX_H */

