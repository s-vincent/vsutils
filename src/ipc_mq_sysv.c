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
 * \file ipc_mq_sysv.c
 * \brief Message queue implementation for System V.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>

#include "ipc_mq_sysv.h"

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

#if !defined(_WIN32) && !defined(_WIN64)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

/**
 * \struct ipc_mq_sysv
 * \brief Private structure for System V message queue.
 */
struct ipc_mq_sysv
{
    int msqid; /**< System V message queue descriptor. */
    key_t key; /**< System V message queue key. */
    size_t max_msg_size; /**< Maximum size for a message. */
};

ipc_mq ipc_mq_sysv_new(void* value, int mode, int perm)
{
    ipc_mq ret = NULL;
    struct ipc_mq_sysv* priv = NULL;
    key_t key = (key_t)(intptr_t)value;
    int flags = ((mode & O_CREAT) ? IPC_CREAT : 0) | perm;
    int msqid = -1;
    struct msqid_ds attr;

    msqid = msgget(key, flags);
    if(msqid == -1)
    {
        return NULL;
    }

    if(msgctl(msqid, IPC_STAT, &attr) != 0)
    {
        return NULL;
    }

    ret = malloc(sizeof(struct ipc_mq) + sizeof(struct ipc_mq_sysv));
    if(!ret)
    {
        return NULL;
    }
   
    memset(ret, 0x00, sizeof(struct ipc_mq) + sizeof(struct ipc_mq_sysv));
    ret->type = IPC_MQ_SYSV;
    ret->free = ipc_mq_sysv_free;
    ret->get_max_msg_size = ipc_mq_sysv_get_max_msg_size;
    ret->send = ipc_mq_sysv_send;
    ret->recv = ipc_mq_sysv_recv;
    priv = (struct ipc_mq_sysv*)&ret->priv;
    priv->msqid = msqid;
    priv->key = key;
    priv->max_msg_size = attr.msg_qbytes > 8192 ? 8192 : attr.msg_qbytes;
    
    return ret;
}

void ipc_mq_sysv_free(ipc_mq* obj, int unlink)
{
    if(*obj)
    {
        struct ipc_mq_sysv* priv = (struct ipc_mq_sysv*)&(*obj)->priv;

        if(unlink)
        {
            msgctl(priv->msqid, IPC_RMID, NULL);
        }

        free(*obj);
        *obj = NULL;
    }
}

size_t ipc_mq_sysv_get_max_msg_size(ipc_mq obj)
{
    struct ipc_mq_sysv* priv = (struct ipc_mq_sysv*)&obj->priv;

    return priv->max_msg_size;
}

int ipc_mq_sysv_send(ipc_mq obj, const struct ipc_mq_data* data, size_t data_size)
{
    struct ipc_mq_sysv* priv = NULL;

    if(!obj || !data || data_size == 0)
    {
        errno = EINVAL;
        return -1;
    }

    priv = (struct ipc_mq_sysv*)&obj->priv;

    return msgsnd(priv->msqid, data, data_size, 0);
}

int ipc_mq_sysv_recv(ipc_mq obj, struct ipc_mq_data* data, size_t data_size)
{
    struct ipc_mq_sysv* priv = NULL;

    if(!obj || !data || data_size == 0)
    {
        errno = EINVAL;
        return -1;
    }

    priv = (struct ipc_mq_sysv*)&obj->priv;

    return msgrcv(priv->msqid, data, data_size, data->priv, 0);
}

#else

ipc_mq ipc_mq_sysv_new(void* value, int mode, int perm)
{
    (void)value;
    (void)mode;
    (void)perm;

    errno = ENOSYS;
    return NULL;
}

void ipc_mq_sysv_free(ipc_mq* obj, int unlink)
{
    (void)obj;
    (void)unlink;
    return;
}

size_t ipc_mq_sysv_get_max_msg_size(ipc_mq obj)
{
    (void)obj;
    
    errno = ENOSYS;
    return 0;
}

int ipc_mq_sysv_send(ipc_mq obj, const ipc_mq_data* data, size_t data_size)
{
    (void)obj;
    (void)data;
    (void)data_size;
    errno = ENOSYS;
    return NULL;
}

int ipc_mq_sysv_recv(ipc_mq obj, ipc_mq_data* data, size_t data_size)
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

