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
 * \file ipc_mq_impl.h
 * \brief IPC message queue definition.
 * \author Sebastien Vincent.
 * \date 2016
 */

#ifndef IPC_MQ_IMPL_H
#define IPC_MQ_IMPL_H

#include "ipc_mq.h"

/**
 * \struct ipc_mq
 * \brief IPC message queue.
 */
struct ipc_mq
{
  /**
   * \brief Message queue type.
   */
  enum ipc_mq_type type; /**< Message queue type. */

  /**
   * \brief Function to free the object.
   */
  void (*free)(ipc_mq*, int);

  /**
   * \brief Function to get max message size.
   */
  size_t (*get_max_msg_size)(struct ipc_mq* obj);

  /**
   * \brief Function to send a message to the queue.
   */
  int (*send)(ipc_mq, const struct ipc_mq_data*, size_t);

  /**
   * \brief Function to receive a message from the queue.
   */
  int (*recv)(ipc_mq, struct ipc_mq_data*, size_t data_size);

  /**
   * \brief Private implementation.
   */
  char priv[];
};

#endif /* IPC_MQ_IMPL_H */

