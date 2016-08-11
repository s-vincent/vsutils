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
 * \file ipc_mq_sysv.h
 * \brief Message queue implementation for System V.
 * \author Sebastien Vincent
 * \date 2016
 */

#ifndef IPC_MQ_SYSV_H
#define IPC_MQ_SYSV_H

#include "ipc_mq.h"
#include "ipc_mq_impl.h"

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \brief Gets a new message queue object.
 * \param value value obtained via ftok() or magic cookie value.
 * \param mode (O_RDONLY, O_RDWR or O_WRONLY).
 * \param perm permissions (bitfield of S_IRUSR, S_IWUSR, S_IRGRP, S_IWGRP,
 * S_IROTH or S_IWOTH).
 * \return System V message queue pointer.
 */
ipc_mq ipc_mq_sysv_new(void* value, int mode, int perm);

/**
 * \brief Frees IPC message queue object.
 * \param obj IPC message queue object.
 * \param unlink positive value to destroy the message queue.
 */
void ipc_mq_sysv_free(ipc_mq* obj, int unlink);

/**
 * \brief Return max message size.
 * \param obj IPC message queue object.
 * \return max message size.
 */
size_t ipc_mq_sysv_get_max_msg_size(ipc_mq obj);

/**
 * \brief Send data in message queue.
 * \param obj IPC message queue object.
 * \param data data to send.
 * \param data_size Size of data.
 * \return 0 if success, -1 if failure.
 */
int ipc_mq_sysv_send(ipc_mq obj, const struct ipc_mq_data* data,
    size_t data_size);

/**
 * \brief Receive data from message queue.
 * \param obj IPC message queue object.
 * \param data Buffer to receive data.
 * \param data_size Size of buffer.
 * \return Number of bytes received, -1 if failure.
 */
int ipc_mq_sysv_recv(ipc_mq obj, struct ipc_mq_data* data, size_t data_size);

#ifdef __cplusplus
}
#endif

#endif /* IPC_MQ_SYSV_H */

