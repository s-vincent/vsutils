/*
 * Copyright (C) 2014-2016 Sebastien Vincent.
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
 * \file thread_pool.h
 * \brief Thread pool for tasks.
 * \author Sebastien Vincent
 * \date 2014-2016
 */

#ifndef VSUTILS_THREAD_POOL_H
#define VSUTILS_THREAD_POOL_H

#include "list.h"

/**
 * \typedef thread_pool
 * \brief Opaque type for thread pool.
 */
typedef struct thread_pool* thread_pool;

/**
 * \struct thread_pool_task
 * \brief Task for the thread pool.
 *
 * The data member is passed to the run and cleanup functions.\n
 * The run function is executed when the task is processed by a worker
 * thread.\n
 * The cleanup function (if not NULL) is executed after the run function.
 */
struct thread_pool_task
{
  void* data; /**< Application data. */
  void (*run)(void*); /**< Run function. */
  void (*cleanup)(void*); /**< Cleanup function. */
  struct list_head list; /**< For list management. */
};

/**
 * \brief Create a new thread pool.
 * \param nb number of threads to launcher.
 * \return new thread pool or NULL if failure.
 */
thread_pool thread_pool_new(size_t nb);

/**
 * \brief Delete a thread pool.
 * \param obj pointer on thread_pool.
 */
void thread_pool_free(thread_pool* obj);

/**
 * \brief Start the thread pool.
 * \param obj thread pool.
 */
void thread_pool_start(thread_pool obj);

/**
 * \brief Stop the thread pool.
 * \param obj thread pool.
 */
void thread_pool_stop(thread_pool obj);

/**
 * \brief Push a task to the thread pool.
 * \param obj thread pool.
 * \param task task to be pushed, task members will be copied so if task is
 * allocated, you can delete it after the call.
 * \return 0 if success, -1 on failure.
 */
int thread_pool_push(thread_pool obj, struct thread_pool_task* task);

/**
 * \brief Clean tasks of the thread pool.
 * \param obj thread pool.
 * \return 0 if success, -1 otherwise.
 * \note Call only this function when thread dispatcher is stopped.
 */
int thread_pool_clean(thread_pool obj);

#endif /* VSUTILS_THREAD_POOL_H */

