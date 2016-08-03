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
 * \file thread_dispatcher.h
 * \brief Thread dispatcher for tasks.
 * \author Sebastien Vincent
 * \date 2016
 */

#ifndef VSUTILS_THREAD_DISPATCHER_H
#define VSUTILS_THREAD_DISPATCHER_H

#include <stdint.h>
#include "list.h"

/**
 * \typedef thread_dispatcher
 * \brief Opaque type for thread dispatcher.
 */
typedef struct thread_dispatcher* thread_dispatcher;

/**
 * \struct thread_dispatcher_task
 * \brief Task for the thread dispatcher.
 *
 * The data member is passed to the run and cleanup functions.\n
 * The run function is executed when the task is processed by a worker
 * thread. The cleanup function is executed after the run function.
 */
struct thread_dispatcher_task
{
  /**
   * \brief Application data.
   */
  void* data;

  /**
   * \brief Run function.
   */
  void (*run)(void*);

  /**
   * \brief Cleanup function.
   */
  void (*cleanup)(void*);

  /**
   * \brief For list management.
   */
  struct list_head list;
};

/**
 * \brief Create a new thread dispatcher with "nb" worker thread.
 * \param nb number of threads to launch.
 * \return new thread dispatcher or NULL if failure.
 */
thread_dispatcher thread_dispatcher_new(size_t nb);

/**
 * \brief Delete a thread dispatcher.
 * \param obj pointer on thread_dispatcher.
 */
void thread_dispatcher_free(thread_dispatcher* obj);

/**
 * \brief Start the thread dispatcher.
 * \param obj thread dispatcher.
 */
void thread_dispatcher_start(thread_dispatcher obj);

/**
 * \brief Stop the thread dispatcher.
 * \param obj thread dispatcher.
 */
void thread_dispatcher_stop(thread_dispatcher obj);

/**
 * \brief Push a task to be dispatch to random thread.
 * \param obj thread dispatcher.
 * \param task task to be pushed, task members will be copied so if task is
 * allocated, you can delete it after the call.
 * \return 0 if success, -1 on failure.
 */
int thread_dispatcher_push_random(thread_dispatcher obj,
    struct thread_dispatcher_task* task);

/**
 * \brief Push a task to be dispatch to specific thread determined by "color".
 * \param obj thread dispatcher.
 * \param task task to be pushed, task members will be copied so if task is
 * allocated, you can delete it after the call.
 * \param color task with same "color" will never run concurrently, so use it
 * to run portion of code sequentially.
 * \return 0 if success, -1 on failure.
 */
int thread_dispatcher_push(thread_dispatcher obj,
    struct thread_dispatcher_task* task, uint32_t color);

/**
 * \brief Clean all tasks of the thread dispatcher.
 * \param obj thread dispatcher.
 * \return 0 if success, -1 otherwise.
 * \note Call only this function when thread dispatcher is stopped.
 */
int thread_dispatcher_clean(thread_dispatcher obj);

#endif /* VSUTILS_THREAD_DISPATCHER_H */

