/**
 * \file thread_dispatcher.h
 * \brief Thread dispatcher for tasks.
 * \author Sebastien Vincent
 * \date 2014
 */

#ifndef VSUTILS_THREAD_DISPATCHER_H
#define VSUTILS_THREAD_DISPATCHER_H

#include "list.h"

/**
 * \typedef thread_dispatcher
 * \brief Opaque type for thread dispatcher.
 */
typedef struct thread_dispatcher* thread_dispatcher;

/**
 * \struct thread_task
 * \brief Task for the thread dispatcher.
 *
 * The data member is passed to the run and cleanup functions.\n
 * The run function is executed when the task is processed by a worker
 * thread.\n
 * The cleanup function (if not NULL) is executed after the run function.
 */
struct thread_task
{
  void* data; /**< Application data. */
  void (*run)(void*); /**< Run function. */
  void (*cleanup)(void*); /**< Cleanup function. */
  struct list_head list; /**< For list management. */
};

/**
 * \brief Create a new thread dispatcher with "nb" worker thread.
 */
thread_dispatcher thread_dispatcher_new(unsigned nb);

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
 * \brief Push a task to the thread dispatcher.
 * \param obj thread dispatcher.
 * \param task task to be pushed, task members will be copied so if task is
 * allocated, you can delete it after the call.
 * \return 0 if success, -1 on failure.
 */
int thread_dispatcher_push(thread_dispatcher obj, struct thread_task* task);

/**
 * \brief Pop the first task of the thread dispatcher.
 * \param obj thread dispatcher.
 * \param task task that will be popped, it will be filled with data from the
 * manager.
 * \return 0 if success, -1 on failure.
 * \note This function is blocking until a task is available.
 */
int thread_dispatcher_pop(thread_dispatcher obj, struct thread_task* task);

/**
 * \brief Clean all the task of the thread dispatcher.
 * \param obj thread dispatcher.
 * \return 0 if success, -1 otherwise.
 */
int thread_dispatcher_clean(thread_dispatcher obj);

#endif /* VSUTILS_THREAD_DISPATCHER_H */

