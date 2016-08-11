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
 * \file ipc_shm_posix.c
 * \brief Shared memory implementation for POSIX.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ipc_shm_posix.h"

#if !defined(_WIN32) && !defined(_WIN64)
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#endif

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

#if defined(_POSIX_SHARED_MEMORY_OBJECTS)

/**
 * \struct ipc_shm_posix
 * \brief Private structure for POSIX shared memory.
 */
struct ipc_shm_posix
{
  int shm; /**< POSIX shared memory descriptor. */
  char name[1024]; /**< POSIX shared memory name. */
};

ipc_shm ipc_shm_posix_new(void* value, int mode, int perm, size_t size)
{
  ipc_shm ret = NULL;
  struct ipc_shm_posix* priv = NULL;
  char* name = (char*)value;
  int shm = -1;
  void* data = NULL;

  if(name == NULL)
  {
    errno = EINVAL;
    return NULL;
  }

  shm = shm_open(name, mode, (mode_t)perm);
  if(shm == -1)
  {
    return NULL;
  }

  if(ftruncate(shm, size) == -1 ||
      !(data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0)))
  {
    close(shm);
    return NULL;
  }

  ret = malloc(sizeof(struct ipc_shm) + sizeof(struct ipc_shm_posix));
  if(!ret)
  {
    close(shm);
    return NULL;
  }

  memset(ret, 0x00, sizeof(struct ipc_shm) + sizeof(struct ipc_shm_posix));
  ret->type = IPC_SHM_POSIX;
  ret->data = data;
  ret->data_size = size;
  ret->free = ipc_shm_posix_free;
  ret->get_data = ipc_shm_posix_get_data;
  ret->get_data_size = ipc_shm_posix_get_data_size;
  priv = (struct ipc_shm_posix*)&ret->priv;
  strncpy(priv->name, name, sizeof(priv->name));
  priv->shm = shm;

  return ret;
}

void ipc_shm_posix_free(ipc_shm* obj, int unlink)
{
  if(*obj)
  {
    struct ipc_shm_posix* priv = (struct ipc_shm_posix*)&(*obj)->priv;

    /* remove memory mapping */
    munmap((*obj)->data, (*obj)->data_size);
    /* close shared memory */
    close(priv->shm);

    if(unlink)
    {
      /* remove it from the system */
      shm_unlink(priv->name);
    }

    free(*obj);
    *obj = NULL;
  }
}

void* ipc_shm_posix_get_data(ipc_shm obj)
{
  return obj->data;
}

size_t ipc_shm_posix_get_data_size(ipc_shm obj)
{
  return obj->data_size;
}

#else

ipc_shm ipc_shm_posix_new(void* value, int mode, int perm)
{
  (void)value;
  (void)mode;
  (void)perm;

  errno = ENOSYS;
  return NULL;
}

void ipc_shm_posix_free(ipc_shm* obj, int unlink)
{
  (void)obj;
  (void)unlink;

  errno = ENOSYS;
  return;
}

void* ipc_shm_posix_get_data(ipc_shm obj)
{
  (void)obj;
  errno = ENOSYS;
  return NULL;
}

size_t ipc_shm_posix_get_data_size(ipc_shm obj)
{
  (void)obj;

  errno = ENOSYS;
  return 0;
}

#endif

#ifdef __cplusplus
}
#endif

