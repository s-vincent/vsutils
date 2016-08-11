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
 * \file ipc_sem_posix.c
 * \brief Semaphore implementation for POSIX.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "ipc_sem_posix.h"

#if !defined(_WIN32) && !defined(_WIN64)
#include <unistd.h>
#endif

#if defined(_POSIX_SEMAPHORES)
#include <semaphore.h>

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \struct ipc_sem_posix
 * \brief Private structure for POSIX semaphore.
 */
struct ipc_sem_posix
{
  sem_t* sem; /**< POSIX semaphore descriptor. */
  char name[1024]; /**< POSIX semaphore name. */
};

ipc_sem ipc_sem_posix_new(void* value, int mode, int perm, unsigned int init)
{
  ipc_sem ret = NULL;
  struct ipc_sem_posix* priv = NULL;
  char* name = (char*)value;
  sem_t* sem;

  if(name == NULL)
  {
    errno = EINVAL;
    return NULL;
  }

  sem = sem_open(name, mode, (mode_t)perm, init);
  if(sem == NULL)
  {
    return NULL;
  }

  ret = malloc(sizeof(struct ipc_sem) + sizeof(struct ipc_sem_posix));
  if(!ret)
  {
    sem_close(sem);
    return NULL;
  }

  memset(ret, 0x00, sizeof(struct ipc_sem) + sizeof(struct ipc_sem_posix));
  ret->type = IPC_SEM_POSIX;
  ret->free = ipc_sem_posix_free;
  ret->lock = ipc_sem_posix_lock;
  ret->unlock = ipc_sem_posix_unlock;
  priv = (struct ipc_sem_posix*)&ret->priv;
  strncpy(priv->name, name, sizeof(priv->name));
  priv->sem = sem;

  return ret;
}

void ipc_sem_posix_free(ipc_sem* obj, int unlink)
{
  if(*obj)
  {
    struct ipc_sem_posix* priv = (struct ipc_sem_posix*)&(*obj)->priv;

    /* close semaphore */
    sem_close(priv->sem);

    if(unlink)
    {
      /* remove it from the system */
      sem_unlink(priv->name);
    }

    free(*obj);
    *obj = NULL;
  }
}

int ipc_sem_posix_lock(ipc_sem obj)
{
  struct ipc_sem_posix* priv = (struct ipc_sem_posix*)&obj->priv;

  return sem_wait(priv->sem);
}

int ipc_sem_posix_lock_timed(ipc_sem obj, const struct timespec* timeout)
{
  struct ipc_sem_posix* priv = (struct ipc_sem_posix*)&obj->priv;

  return sem_timedwait(priv->sem, timeout);
}

int ipc_sem_posix_unlock(ipc_sem obj)
{
  struct ipc_sem_posix* priv = (struct ipc_sem_posix*)&obj->priv;

  return sem_post(priv->sem);
}

#else

ipc_sem_posix ipc_sem_posix_new(void* value, int mode, int perm)
{
  (void)value;
  (void)mode;
  (void)perm;

  errno = ENOSYS;
  return NULL;
}

void ipc_sem_posix_free(ipc_sem* obj, int unlink)
{
  (void)obj;
  (void)unlink;
  return;
}

int ipc_sem_posix_lock(ipc_sem obj)
{
  (void)obj;
  errno = ENOSYS;
  return -1;
}

int ipc_sem_posix_lock_timed(ipc_sem obj, const struct timespec* timeout)
{
  (void)obj;
  (void)timeout;
  errno = ENOSYS;
  return -1;
}

int ipc_sem_posix_unlock(ipc_sem obj)
{
  (void)obj;
  errno = ENOSYS;
  return -1;
}

#endif

#ifdef __cplusplus
}
#endif

