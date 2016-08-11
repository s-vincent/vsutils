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
 * \file ipc_shm_sysv.c
 * \brief Shared memory implementation for System V.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "ipc_shm_sysv.h"

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

#if !defined(_WIN32) && !defined(_WIN64)

#include <sys/ipc.h>
#include <sys/shm.h>

/**
 * \struct ipc_shm_sysv
 * \brief Private structure for System V shared memory.
 */
struct ipc_shm_sysv
{
  int shmid; /**< System V shared memory descriptor. */
  key_t key; /**< System V shared memory key. */
};

ipc_shm ipc_shm_sysv_new(void* value, int mode, int perm, size_t size)
{
  ipc_shm ret = NULL;
  struct ipc_shm_sysv* priv = NULL;
  key_t key = (key_t)(intptr_t)value;
  int flags = ((mode & O_CREAT) ? IPC_CREAT : 0) | perm;
  int shmid = -1;
  void* data = NULL;

  shmid = shmget(key, size, flags);
  if(shmid == -1)
  {
    return NULL;
  }

  /* attach memory */
  data = shmat(shmid, NULL, 0);

  if(!data)
  {
    return NULL;
  }

  ret = malloc(sizeof(struct ipc_shm) + sizeof(struct ipc_shm_sysv));
  if(!ret)
  {
    return NULL;
  }

  memset(ret, 0x00, sizeof(struct ipc_shm) + sizeof(struct ipc_shm_sysv));
  ret->type = IPC_SHM_SYSV;
  ret->free = ipc_shm_sysv_free;
  ret->get_data = ipc_shm_sysv_get_data;
  ret->get_data_size = ipc_shm_sysv_get_data_size;
  ret->data = data;
  ret->data_size = size;
  priv = (struct ipc_shm_sysv*)&ret->priv;
  priv->shmid = shmid;
  priv->key = key;

  return ret;
}

void ipc_shm_sysv_free(ipc_shm* obj, int unlink)
{
  if(*obj)
  {
    struct ipc_shm_sysv* priv = (struct ipc_shm_sysv*)&(*obj)->priv;

    shmdt((*obj)->data);

    if(unlink)
    {
      shmctl(priv->shmid, IPC_RMID, NULL);
    }

    free(*obj);
    *obj = NULL;
  }
}

void* ipc_shm_sysv_get_data(ipc_shm obj)
{
  return obj->data;
}

size_t ipc_shm_sysv_get_data_size(ipc_shm obj)
{
  return obj->data_size;
}

#else

ipc_shm ipc_shm_sysv_new(void* value, int mode, int perm)
{
  (void)value;
  (void)mode;
  (void)perm;

  errno = ENOSYS;
  return NULL;
}

void ipc_shm_sysv_free(ipc_shm* obj, int unlink)
{
  (void)obj;
  (void)unlink;
  return;
}

void* ipc_shm_sysv_get_data(ipc_shm obj)
{
  (void)obj;
  errno = ENOSYS;
  return NULL;
}

size_t ipc_shm_sysv_get_data_size(ipc_shm obj)
{
  (void)obj;

  errno = ENOSYS;
  return 0;
}

#endif

#ifdef __cplusplus
}
#endif

