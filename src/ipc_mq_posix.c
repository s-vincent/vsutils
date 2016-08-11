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
 * \file ipc_mq_posix.c
 * \brief Message queue implementation for POSIX.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "ipc_mq_posix.h"

#if !defined(_WIN32) && !defined(_WIN64)
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

#if defined(_POSIX_MESSAGE_PASSING)
#include <mqueue.h>

/**
 * \struct ipc_mq_posix
 * \brief Private structure for POSIX message queue.
 */
struct ipc_mq_posix
{
  mqd_t mq; /**< POSIX message queue descriptor. */
  char name[1024]; /**< POSIX message queue name. */
  size_t max_msg_size; /**< Maximum size for a message. */
};

ipc_mq ipc_mq_posix_new(void* value, int mode, int perm)
{
  ipc_mq ret = NULL;
  struct ipc_mq_posix* priv = NULL;
  char* name = (char*)value;
  mqd_t mq;
  struct mq_attr attr;

  if(name == NULL)
  {
    errno = EINVAL;
    return NULL;
  }

  mq = mq_open(name, mode, (mode_t)perm, NULL);
  if(mq == (mqd_t)-1)
  {
    return NULL;
  }

  if(mq_getattr(mq, &attr) != 0)
  {
    mq_close(mq);
    return NULL;
  }

  ret = malloc(sizeof(struct ipc_mq) + sizeof(struct ipc_mq_posix));
  if(!ret)
  {
    mq_close(mq);
    return NULL;
  }

  memset(ret, 0x00, sizeof(struct ipc_mq) + sizeof(struct ipc_mq_posix));
  ret->type = IPC_MQ_POSIX;
  ret->free = ipc_mq_posix_free;
  ret->get_max_msg_size = ipc_mq_posix_get_max_msg_size;
  ret->send = ipc_mq_posix_send;
  ret->recv = ipc_mq_posix_recv;
  priv = (struct ipc_mq_posix*)&ret->priv;
  strncpy(priv->name, name, sizeof(priv->name));
  priv->mq = mq;
  priv->max_msg_size = attr.mq_msgsize;

  return ret;
}

void ipc_mq_posix_free(ipc_mq* obj, int unlink)
{
  if(*obj)
  {
    struct ipc_mq_posix* priv = (struct ipc_mq_posix*)&(*obj)->priv;

    /* close message queue */
    mq_close(priv->mq);

    if(unlink)
    {
      /* remove it from the system */
      mq_unlink(priv->name);
    }

    free(*obj);
    *obj = NULL;
  }
}

size_t ipc_mq_posix_get_max_msg_size(ipc_mq obj)
{
  struct ipc_mq_posix* priv = (struct ipc_mq_posix*)&obj->priv;

  return priv->max_msg_size;
}

int ipc_mq_posix_send(ipc_mq obj, const struct ipc_mq_data* data,
    size_t data_size)
{
  struct ipc_mq_posix* priv = NULL;

  if(!obj || !data || data_size == 0)
  {
    errno = EINVAL;
    return -1;
  }

  priv = (struct ipc_mq_posix*)&obj->priv;

  return mq_send(priv->mq, data->data, data_size, (unsigned int)data->priv);
}

int ipc_mq_posix_recv(ipc_mq obj, struct ipc_mq_data* data, size_t data_size)
{
  struct ipc_mq_posix* priv = NULL;

  if(!obj || !data || data_size == 0)
  {
    errno = EINVAL;
    return -1;
  }

  priv = (struct ipc_mq_posix*)&obj->priv;

  return mq_receive(priv->mq, data->data, data_size,
      (unsigned int*)&data->priv);
}

#else

ipc_mq ipc_mq_posix_new(void* value, int mode, int perm)
{
  (void)value;
  (void)mode;
  (void)perm;

  errno = ENOSYS;
  return NULL;
}

void ipc_mq_posix_free(ipc_mq* obj, int unlink)
{
  (void)obj;
  (void)unlink;
  return;
}

int ipc_mq_posix_send(ipc_mq obj, const struct ipc_mq_data* data,
    size_t data_size)
{
  (void)obj;
  (void)data;
  (void)data_size;
  errno = ENOSYS;
  return NULL;
}

int ipc_mq_posix_recv(ipc_mq obj, struct ipc_mq_data* data, size_t data_size)
{
  (void)obj;
  (void)data;
  (void)data_size;
  errno = ENOSYS;
  return NULL;
}

#endif

#ifdef __cplusplus
}
#endif

