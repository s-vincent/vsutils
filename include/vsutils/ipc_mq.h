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
 * \file ipc_mq.h
 * \brief IPC message queue.
 * \author Sebastien Vincent
 * \date 2016
 */

#ifndef VSUTILS_IPC_MQ_H
#define VSUTILS_IPC_MQ_H

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \enum ipc_mq_type
 * \brief Enumerations for message queue type.
 */
enum ipc_mq_type
{
  IPC_MQ_SYSV, /**< SystemV MQ IPC. */
  IPC_MQ_POSIX, /**< POSIX MQ IPC. */
};

/**
 * \struct ipc_mq_data
 * \brief Data message for IPC message queue.
 */
struct ipc_mq_data
{
  /**
   * \brief Type for System V or priority for POSIX message queue.
   * Value 0 is not allowed, just put 1 for default.
   */
  long priv;

  /**
   * \brief Data.
   */
  char data[];
};

/**
 * \typedef ipc_mq
 * \brief Typedef for IPC message queue.
 */
typedef struct ipc_mq* ipc_mq;

/**
 * \brief Gets a new message queue object.
 * \param type type of message queue (POSIX, SYSV or Windows).
 * \param value opaque value. It can be of several types:
 * - For POSIX: string in the form "/my_mq_name";
 * - For System V: key_t value obtained via ftok() or magic cookie value.
 * \param mode (O_RDONLY, O_RDWR or O_WRONLY).
 * \param perm permissions (bitfield of S_IRUSR, S_IWUSR, S_IRGRP, S_IWGRP,
 * S_IROTH or S_IWOTH).
 * \return IPC message queue pointer.
 */
ipc_mq ipc_mq_new(enum ipc_mq_type type, void* value, int mode, int perm);

/**
 * \brief Closes and frees IPC message queue object.
 * \param obj IPC message queue object.
 * \param unlink positive value to destroy the message queue.
 */
void ipc_mq_free(ipc_mq* obj, int unlink);

/**
 * \brief Return max message size.
 * \param obj IPC message queue object.
 * \return max message size.
 */
size_t ipc_mq_get_max_msg_size(ipc_mq obj);

/**
 * \brief Send data in message queue.
 * \param obj IPC message queue object.
 * \param data data to send.
 * \param data_size Size of payload data (data membe of ipc_mq_data).
 * \return 0 if success, -1 if failure.
 * \note Size of data have to be identical as the one received.
 */
int ipc_mq_send(ipc_mq obj, const struct ipc_mq_data* data, size_t data_size);

/**
 * \brief Receive data from message queue.
 * \param obj IPC message queue object.
 * \param data Buffer to receive data.
 * \param data_size Size of buffer (data member of ipc_mq_data).
 * \return Number of bytes received, -1 if failure.
 * \note Size of data have to be identical as the one sent.
 */
int ipc_mq_recv(ipc_mq obj, struct ipc_mq_data* data, size_t data_size);

/**
 * \brief Check if message queue type is supported.
 * \param type message queue type.
 * \return 1 if supported, 0 otherwise.
 */
int ipc_mq_is_supported(enum ipc_mq_type type);

/**
 * \brief Returns "best" message queue type for the current OS.
 * \return Best message queue type for the current OS.
 */
enum ipc_mq_type ipc_mq_get_best_type(void);

#ifdef __cplusplus
}
#endif

#endif /* VSUTILS_IPC_MQ_H */

