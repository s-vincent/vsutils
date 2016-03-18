/**
 * \file ipc_mq_posix.h
 * \brief Message queue implementation for POSIX.
 * \author Sebastien Vincent
 * \date 2016
 */

#ifndef IPC_MQ_POSIX_H
#define IPC_MQ_POSIX_H

#include "ipc_mq.h"
#include "ipc_mq_impl.h"

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \brief Gets a new message queue object.
 * \param value name of the message queue in the form "/name".
 * \param mode (O_RDONLY, O_RDWR or O_WRONLY).
 * \param perm permissions (bitfield of S_IRUSR, S_IWUSR, S_IRGRP, S_IWGRP,
 * S_IROTH or S_IWOTH).
 * \return POSIX message queue pointer.
 */
ipc_mq ipc_mq_posix_new(void* value, int mode, int perm);

/**
 * \brief Frees IPC message queue object.
 * \param obj IPC message queue object.
 * \param unlink positive value to destroy the message queue.
 */
void ipc_mq_posix_free(ipc_mq* obj, int unlink);

/**
 * \brief Return max message size.
 * \param obj IPC message queue object.
 * \return max message size.
 */
size_t ipc_mq_posix_get_max_msg_size(ipc_mq obj);

/**
 * \brief Send data in message queue.
 * \param obj IPC message queue object.
 * \param data data to send.
 * \param data_size Size of data.
 * \return 0 if success, -1 if failure.
 */
int ipc_mq_posix_send(ipc_mq obj, const struct ipc_mq_data* data, size_t data_size);

/**
 * \brief Receive data from message queue.
 * \param obj IPC message queue object.
 * \param data Buffer to receive data.
 * \param data_size Size of buffer.
 * \return Number of bytes received, -1 if failure.
 */
int ipc_mq_posix_recv(ipc_mq obj, struct ipc_mq_data* data, size_t data_size);

#ifdef __cplusplus
}
#endif

#endif /* IPC_MQ_POSIX_H */
