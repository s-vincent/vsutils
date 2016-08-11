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
 * \file ipc_sem.h
 * \brief IPC semaphore.
 * \author Sebastien Vincent
 * \date 2016
 */

#ifndef VSUTILS_IPC_SEM_H
#define VSUTILS_IPC_SEM_H

#include <stdint.h>

#include <time.h>

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \enum ipc_sem_type
 * \brief Enumerations for semaphore type.
 */
enum ipc_sem_type
{
  IPC_SEM_SYSV, /**< SystemV semaphore IPC. */
  IPC_SEM_POSIX, /**< POSIX semaphore IPC. */
  IPC_SEM_WIN /**< Windows semaphore IPC. */
};

/**
 * \typedef ipc_sem
 * \brief Typedef for IPC semaphore.
 */
typedef struct ipc_sem* ipc_sem;

/**
 * \brief Gets a new semaphore object.
 * \param type type of semaphore (POSIX, SYSV or Windows).
 * \param value opaque value. It can be of several types:
 * - For POSIX: string in the form "/my_mq_name";
 * - For System V: key_t value obtained via ftok() or magic cookie value.
 * \param mode (O_RDONLY, O_RDWR or O_WRONLY).
 * \param perm permissions (bitfield of S_IRUSR, S_IWUSR, S_IRGRP, S_IWGRP,
 * S_IROTH or S_IWOTH).
 * \param init initial value for the semaphore.
 * \return IPC semaphore pointer.
 * \warning Some parameters of permission may be ignored for IPC_SEM_WIN type.
 */
ipc_sem ipc_sem_new(enum ipc_sem_type type, void* value, int mode, int perm,
    unsigned int init);

/**
 * \brief Closes and frees IPC semaphore object.
 * \param obj IPC semaphore object.
 * \param unlink positive value to destroy the semaphore.
 */
void ipc_sem_free(ipc_sem* obj, int unlink);

/**
 * \brief Lock a semaphore (decrement value).
 * \param obj IPC semaphore object.
 * \return 0 if success, -1 otherwise.
 */
int ipc_sem_lock(ipc_sem obj);

/**
 * \brief Lock a semaphore (decrement value).
 * \param obj IPC semaphore object.
 * \param timeout timeout.
 * \return 0 if success, -1 otherwise.
 */
int ipc_sem_lock_timed(ipc_sem obj, const struct timespec* timeout);

/**
 * \brief Unlock a semaphore (increment value).
 * \param obj IPC semaphore object.
 * \return 0 if success, -1 otherwise.
 */
int ipc_sem_unlock(ipc_sem obj);

/**
 * \brief Send data in semaphore.
 * \param type IPC semaphore type.
 * \return 1 if supported, 0 otherwise.
 */
int ipc_sem_is_supported(enum ipc_sem_type type);

/**
 * \brief Returns "best" semaphore type for the current OS.
 * \return Best semaphore type for the current OS.
 */
enum ipc_sem_type ipc_sem_get_best_type(void);

#ifdef __cplusplus
}
#endif

#endif /* VSUTILS_IPC_SEM_H */

