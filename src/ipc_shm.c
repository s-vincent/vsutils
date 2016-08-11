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
 * \file ipc_shm.c
 * \brief IPC shared memory.
 * \author Sebastien Vincent
 * \date 2016
 */

#if !defined(_WIN32) && !defined(_WIN64)
/* Unix variants */
#include <unistd.h>
#endif

#include "ipc_shm.h"
#include "ipc_shm_impl.h"
#include "ipc_shm_posix.h"
#include "ipc_shm_sysv.h"
/*#include "ipc_shm_win.h"*/

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

ipc_shm ipc_shm_new(enum ipc_shm_type type, void* value, int mode, int perm,
    size_t size)
{
  switch(type)
  {
    case IPC_SHM_SYSV:
      return ipc_shm_sysv_new(value, mode, perm, size);
      break;
    case IPC_SHM_POSIX:
      return ipc_shm_posix_new(value, mode, perm, size);
      break;
    /*
    case IPC_SHM_WIN:
      return ipc_shm_win_new(value, mode, perm);
      break;
    */
    default:
      return NULL;
      break;
  }
}

void ipc_shm_free(ipc_shm* obj, int unlink)
{
  (*obj)->free(obj, unlink);
}

void* ipc_shm_get_data(ipc_shm obj)
{
  return obj->get_data(obj);
}

size_t ipc_shm_get_data_size(ipc_shm obj)
{
  return obj->get_data_size(obj);
}

int ipc_shm_is_supported(enum ipc_shm_type type)
{
#if defined(_WIN32) || defined(_WIN64)
  return type == IPC_SHM_WIN;
#else
  switch(type)
  {
    case IPC_SHM_WIN:
      return 0;
      break;
    case IPC_SHM_SYSV:
      return 1;
      break;
    case IPC_SHM_POSIX:
#if defined(_POSIX_SHARED_MEMORY_OBJECTS)
      return 1;
#else
      return 0;
#endif
      break;
    default:
      return 0;
      break;
  }
#endif
}

enum ipc_shm_type ipc_shm_get_best_type(void)
{
#if defined(_WIN32) || defined(_WIN64)
  return IPC_SHM_WIN;
#elif defined(_POSIX_MESSAGE_PASSING)
  return IPC_SHM_POSIX;
#else
  return IPC_SHM_SYSV;
#endif
}

#ifdef __cplusplus
}
#endif

