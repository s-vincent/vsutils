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
 * \file ipc_mq.c
 * \brief IPC message queue.
 * \author Sebastien Vincent
 * \date 2016
 */

#if !defined(_WIN32) && !defined(_WIN64)
/* Unix variants */
#include <unistd.h>
#endif

#include <stdlib.h>

#include "ipc_mq.h"
#include "ipc_mq_impl.h"
#include "ipc_mq_posix.h"
#include "ipc_mq_sysv.h"
/*#include "ipc_mq_win.h"*/

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

ipc_mq ipc_mq_new(enum ipc_mq_type type, void* value, int mode, int perm)
{
  switch(type)
  {
    case IPC_MQ_SYSV:
      return ipc_mq_sysv_new(value, mode, perm);
      break;
    case IPC_MQ_POSIX:
      return ipc_mq_posix_new(value, mode, perm);
      break;
    /*
    case IPC_MQ_WIN:
         return ipc_mq_win_new(value, mode, perm);
         break;
    */
    default:
      return NULL;
      break;
  }
}

void ipc_mq_free(ipc_mq* obj, int unlink)
{
  (*obj)->free(obj, unlink);
}

size_t ipc_mq_get_max_msg_size(ipc_mq obj)
{
  return obj->get_max_msg_size(obj);
}

int ipc_mq_send(ipc_mq obj, const struct ipc_mq_data* data, size_t data_size)
{
  return obj->send(obj, data, data_size);
}

int ipc_mq_recv(ipc_mq obj, struct ipc_mq_data* data, size_t data_size)
{
  return obj->recv(obj, data, data_size);
}

int ipc_mq_is_supported(enum ipc_mq_type type)
{
#if defined(_WIN32) || defined(_WIN64)
  return type == IPC_MQ_WIN;
#else
  switch(type)
  {
    case IPC_MQ_WIN:
      return 0;
      break;
    case IPC_MQ_SYSV:
      return 1;
      break;
    case IPC_MQ_POSIX:
#if defined(_POSIX_MESSAGE_PASSING)
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

  return 0;
}

enum ipc_mq_type ipc_mq_get_best_type(void)
{
#if defined(_WIN32) || defined(_WIN64)
  return IPC_MQ_WIN;
#elif defined(_POSIX_MESSAGE_PASSING)
  return IPC_MQ_POSIX;
#else
  return IPC_MQ_SYSV;
#endif
}

#ifdef __cplusplus
}
#endif

