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
 * \file ipc_sem_sysv.c
 * \brief Semaphore implementation for System V.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>

#include "ipc_sem_sysv.h"

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

#if !defined(_WIN32) && !defined(_WIN64)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

/**
 * \struct ipc_sem_sysv
 * \brief Private structure for SYSV semaphore.
 */
struct ipc_sem_sysv
{
  int semid; /**< System V semaphore descriptor. */
  key_t key; /**< System V semaphore key. */
};

ipc_sem ipc_sem_sysv_new(void* value, int mode, int perm, unsigned int init)
{
  ipc_sem ret = NULL;
  struct ipc_sem_sysv* priv = NULL;
  key_t key = (key_t)(intptr_t)value;
  int flags = ((mode & O_CREAT) ? IPC_CREAT : 0) | perm;
  int semid = -1;

  semid = semget(key, 1, flags);
  if(semid == -1)
  {
    return NULL;
  }

  ret = malloc(sizeof(struct ipc_sem) + sizeof(struct ipc_sem_sysv));
  if(!ret)
  {
    return NULL;
  }

  /* set initial value for the semaphore */
  if(semctl(semid, 0, SETVAL, init) == -1)
  {
    return NULL;
  }

  memset(ret, 0x00, sizeof(struct ipc_sem) + sizeof(struct ipc_sem_sysv));
  ret->type = IPC_SEM_SYSV;
  ret->free = ipc_sem_sysv_free;
  ret->lock = ipc_sem_sysv_lock;
  ret->unlock = ipc_sem_sysv_unlock;
  priv = (struct ipc_sem_sysv*)&ret->priv;
  priv->semid = semid;
  priv->key = key;

  return ret;
}

void ipc_sem_sysv_free(ipc_sem* obj, int unlink)
{
  if(*obj)
  {
    struct ipc_sem_sysv* priv = (struct ipc_sem_sysv*)&(*obj)->priv;

    if(unlink)
    {
      semctl(priv->semid, 0, IPC_RMID, NULL);
    }

    free(*obj);
    *obj = NULL;
  }
}

int ipc_sem_sysv_lock(ipc_sem obj)
{
  struct ipc_sem_sysv* priv = (struct ipc_sem_sysv*)&obj->priv;
  struct sembuf buf;

  buf.sem_num = 0;
  /* lock so decrement */
  buf.sem_op = -1;
  buf.sem_flg = 0;

  return semop(priv->semid, &buf, 1);
}

int ipc_sem_sysv_lock_timed(ipc_sem obj, const struct timespec* timeout)
{
#ifdef _GNU_SOURCE
  struct ipc_sem_sysv* priv = (struct ipc_sem_sysv*)&obj->priv;
  struct sembuf buf;

  buf.sem_num = 0;
  /* lock so decrement */
  buf.sem_op = -1;
  buf.sem_flg = 0;

  return semtimedop(priv->semid, &buf, 1, timeout);
#else
  (void)obj;
  (void)timeout;
  errno = ENOSYS;
  return -1;
#endif
}

int ipc_sem_sysv_unlock(ipc_sem obj)
{
  struct ipc_sem_sysv* priv = (struct ipc_sem_sysv*)&obj->priv;
  struct sembuf buf;

  buf.sem_num = 0;
  /* unlock so increment */
  buf.sem_op = 1;
  buf.sem_flg = 0;

  return semop(priv->semid, &buf, 1);
}

#else

ipc_sem_sysv ipc_sem_sysv_new(void* value, int mode, int perm)
{
  (void)value;
  (void)mode;
  (void)perm;

  errno = ENOSYS;
  return NULL;
}

void ipc_sem_sysv_free(ipc_sem* obj, int unlink)
{
  (void)obj;
  (void)unlink;
  return;
}

int ipc_sem_sysv_lock(ipc_sem obj)
{
  (void)obj;
  errno = ENOSYS;
  return -1;
}

int ipc_sem_sysv_lock_timed(ipc_sem obj, const struct timespec* timeout)
{
  (void)obj;
  (void)timeout;
  errno = ENOSYS;
  return -1;
}

int ipc_sem_sysv_unlock(ipc_sem obj)
{
  (void)obj;
  errno = ENOSYS;
  return -1;
}

#endif

#ifdef __cplusplus
}
#endif

