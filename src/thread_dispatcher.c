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
 * \file thread_dispatcher.c
 * \brief Thread dispatcher for tasks.
 * \author Sebastien Vincent
 * \date 2016-2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>

#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#include "thread_dispatcher.h"

/**
 * \struct thread_dispatcher.
 * \brief Thread dispatcher.
 */
struct thread_dispatcher
{
  /**
   * \brief Number of threads.
   */
  size_t nb_threads;

  /**
   * \brief Mutex to protect the start condition.
   */
  pthread_mutex_t mutex_start;

  /**
   * \brief Condition to notify the start.
   */
  pthread_cond_t cond_start;

  /**
   * \brief RW mutex to protect run state.
   */
  pthread_rwlock_t mutex_run;

  /**
   * \brief Status of the dispatcher.
   */
  int run;

  /**
   * \brief Next thread number for "random" thread selection.
   */
  uint32_t next_select;

  /**
   * \brief Array of threads worker.
   */
  struct thread_worker* threads;
};

/**
 * \brief Thread worker.
 */
struct thread_worker
{
  /**
   * \brief Thread identifier.
   */
  pthread_t id;

  /**
   * \brief Mutex to protect the queue.
   */
  pthread_mutex_t mutex_tasks;

  /**
   * \brief Condition to notify a task ready to process.
   */
  pthread_cond_t cond_tasks;

  /**
   * \brief Dispatcher parent.
   */
  struct thread_dispatcher* dispatcher;

  /**
   * \brief List of tasks.
   */
  struct list_head tasks;
};

/**
 * \brief Atomically get run state.
 * \param obj thread dispatcher.
 * \return run state or -99 if failure.
 */
static int thread_dispatcher_get_run(struct thread_dispatcher* obj)
{
  if(pthread_rwlock_rdlock(&obj->mutex_run) == 0)
  {
    int run = obj->run;
    pthread_rwlock_unlock(&obj->mutex_run);
    return run;
  }

  return -99;
}

/**
 * \brief Atomically set run state.
 * \param obj thread dispatcher.
 * \param run run state to set or -1 if failure.
 */
static int thread_dispatcher_set_run(struct thread_dispatcher* obj, int run)
{
  if(pthread_rwlock_wrlock(&obj->mutex_run) == 0)
  {
    obj->run = run;
    pthread_rwlock_unlock(&obj->mutex_run);
    return 0;
  }

  return -1;
}

/**
 * \brief Initialize a thread worker.
 * \param dispatcher thread dispatcher.
 * \param worker worker to initialize.
 * \return 0 if success, -1 otherwise.
 */
static int thread_worker_init(struct thread_dispatcher* dispatcher,
    struct thread_worker* worker)
{
  assert(worker);

  list_head_init(&worker->tasks);
  if(pthread_mutex_init(&worker->mutex_tasks, NULL) != 0)
  {
    return -1;
  }

  if(pthread_cond_init(&worker->cond_tasks, NULL) != 0)
  {
    pthread_mutex_destroy(&worker->mutex_tasks);
    return -1;
  }
  worker->dispatcher = dispatcher;
  return 0;
}

/**
 * \brief Destroy a valid thread worker.
 * \param worker worker to destroy.
 */
static void thread_worker_destroy(struct thread_worker* worker)
{
  struct list_head* pos = NULL;
  struct list_head* tmp = NULL;

  assert(worker);

  /* cleanup worker tasks queue */
  list_head_iterate_safe(&worker->tasks, pos, tmp)
  {
    struct thread_dispatcher_task* t = list_head_get(pos,
        struct thread_dispatcher_task, list);
    list_head_remove(&worker->tasks, &t->list);
    free(t);
  }

  pthread_mutex_destroy(&worker->mutex_tasks);
  pthread_cond_destroy(&worker->cond_tasks);
}

/**
 * \brief Pop the first task to process for the thread worker.
 * \param worker thread worker.
 * \param task task that will be popped, it will be filled with data from the
 * manager.
 * \return 0 if success, -1 on failure (thread dispatcher wants to exit).
 * \note This function is blocking until a task is available.
 */
static int thread_worker_pop(struct thread_worker* worker,
  struct thread_dispatcher_task* task)
{
  struct thread_dispatcher_task* t = NULL;
  int ret = -1;

  assert(worker && task);

  if(pthread_mutex_lock(&worker->mutex_tasks) == 0)
  {
    struct list_head* pos = NULL;
    int run = thread_dispatcher_get_run(worker->dispatcher);

    /* if arrived after condition signaled */
    if(run <= 0)
    {
      pthread_mutex_unlock(&worker->mutex_tasks);
      return run;
    }

    while(list_head_is_empty(&worker->tasks))
    {
      /* wait for a task */
      pthread_cond_wait(&worker->cond_tasks, &worker->mutex_tasks);

      run = thread_dispatcher_get_run(worker->dispatcher);

      /* condition signaled or spurious wake up, check if stop/exit */
      if(run <= 0)
      {
        pthread_mutex_unlock(&worker->mutex_tasks);
        return -1;
      }
    }

    pos = worker->tasks.next;
    t = list_head_get(pos, struct thread_dispatcher_task, list);
    ret = 0;

    /* fill task with thread_dispatcher_task data from list */
    task->data = t->data;
    task->run = t->run;
    task->cleanup = t->cleanup;

    /* remove task from list */
    list_head_remove(&worker->tasks, &t->list);
    pthread_mutex_unlock(&worker->mutex_tasks);
  }

  if(ret == 0)
  {
    free(t);
  }

  return ret;
}

/**
 * \brief Worker thread function that wait for a task to execute.
 * \param data the thread dispatcher.
 * \return NULL.
 */
static void* thr_worker(void* data)
{
  struct thread_worker* worker = (struct thread_worker*)data;
  struct thread_dispatcher* dispatcher = NULL;
  int run = 0;

  assert(worker);

  dispatcher = worker->dispatcher;

  /*
   * block signals because this worker thread will not do signal handler
   * management
   */
  sigset_t mask;
  sigfillset(&mask);
  pthread_sigmask(SIG_BLOCK, &mask, NULL);

  while(run >= 0)
  {
    struct thread_dispatcher_task task;

    memset(&task, 0x00, sizeof(struct thread_dispatcher_task));
    run = thread_dispatcher_get_run(dispatcher);

    if(run == 0)
    {
      /* stop case */
      if(pthread_mutex_lock(&dispatcher->mutex_start) == 0)
      {
        while(run == 0)
        {
          /* wait for start */
          pthread_cond_wait(&dispatcher->cond_start, &dispatcher->mutex_start);

          run = thread_dispatcher_get_run(dispatcher);
        }
        pthread_mutex_unlock(&dispatcher->mutex_start);
      }
      else
      {
        /* problem locking mutex? */
        sched_yield();
      }

      continue;
    }
    else if(run == -1)
    {
      /* free case */
      break;
    }
    else if(run < 0)
    {
      /* probably pthread_rwlock_rdlock has failed due to too many calls */
      sched_yield();
    }
    else
    {
      /* running case */
      /* get a task for the queue */
      if(thread_worker_pop(worker, &task) == 0)
      {
        /* process task then cleanup */
        task.run(task.data);
        task.cleanup(task.data);
      }
    }
  }

  return NULL;
}

thread_dispatcher thread_dispatcher_new(size_t nb)
{
  struct thread_dispatcher* ret = NULL;

  assert(nb);

  ret = malloc(sizeof(struct thread_dispatcher) +
      (sizeof(struct thread_worker) * nb));
  if(!ret)
  {
    return NULL;
  }

  ret->run = 0;
  ret->nb_threads = 0;
  ret->threads = (struct thread_worker*)(((char*)ret) +
      sizeof(struct thread_dispatcher));

  if(pthread_mutex_init(&ret->mutex_start, NULL) != 0)
  {
    free(ret);
    return NULL;
  }

  if(pthread_cond_init(&ret->cond_start, NULL) != 0)
  {
    pthread_mutex_destroy(&ret->mutex_start);
    free(ret);
    return NULL;
  }

  if(pthread_rwlock_init(&ret->mutex_run, NULL) != 0)
  {
    pthread_cond_destroy(&ret->cond_start);
    pthread_mutex_destroy(&ret->mutex_start);
    free(ret);
    return NULL;
  }

  ret->next_select = 0;

  for(size_t i = 0 ; i < nb ; i++)
  {
    struct thread_worker* worker = &ret->threads[i];

    if(thread_worker_init(ret, worker) != 0)
    {
      break;
    }
    else if(pthread_create(&worker->id, NULL, thr_worker, worker) != 0)
    {
      thread_worker_destroy(worker);
      break;
    }
    else
    {
      ret->nb_threads++;
    }
  }

  if(ret->nb_threads != nb)
  {
    thread_dispatcher_set_run(ret, -1);

    /* error creating all threads, cancel the created ones */
    for(size_t i = 0 ; i < ret->nb_threads ; i++)
    {
      if(pthread_mutex_lock(&ret->mutex_start) == 0)
      {
        /* tell the threads that they have to quit */
        pthread_cond_broadcast(&ret->cond_start);
        pthread_mutex_unlock(&ret->mutex_start);

        /* join the threads and destroy private thread worker stuff */
        pthread_join(ret->threads[i].id, NULL);
      }
      else
      {
        pthread_cancel(ret->threads[i].id);
      }

      thread_worker_destroy(&ret->threads[i]);
    }

    pthread_mutex_destroy(&ret->mutex_start);
    pthread_cond_destroy(&ret->cond_start);
    free(ret);
    return NULL;
  }

  return ret;
}

void thread_dispatcher_free(thread_dispatcher* obj)
{
  assert(obj);

  if(pthread_mutex_lock(&(*obj)->mutex_start) == 0)
  {
    /* tell the threads that they have to quit */
    thread_dispatcher_set_run(*obj, -1);

    pthread_cond_broadcast(&(*obj)->cond_start);
    pthread_mutex_unlock(&(*obj)->mutex_start);
  }

  /* wait for the threads */
  for(size_t i = 0 ; i < (*obj)->nb_threads ; i++)
  {
    struct thread_worker* worker = &(*obj)->threads[i];

    if(pthread_mutex_lock(&worker->mutex_tasks) == 0)
    {
      /*
       * unblock worker waiting for tasks
       * worker will then check for run variable and exit
       */
      pthread_cond_signal(&worker->cond_tasks);;
      pthread_mutex_unlock(&worker->mutex_tasks);

      pthread_join(worker->id, NULL);
    }
    else
    {
      pthread_cancel(worker->id);
    }

    thread_worker_destroy(worker);
  }

  /* do not care about success or failure of these calls */
  pthread_mutex_destroy(&(*obj)->mutex_start);
  pthread_cond_destroy(&(*obj)->cond_start);

  free(*obj);
  *obj = NULL;
}

int thread_dispatcher_push_random(thread_dispatcher obj,
    struct thread_dispatcher_task* task)
{
  uint32_t color = 0;

  assert(obj && task);

  color = obj->next_select;
  obj->next_select++;

  return thread_dispatcher_push(obj, task, color);
}

int thread_dispatcher_push(thread_dispatcher obj,
    struct thread_dispatcher_task* task, uint32_t color)
{
  struct thread_dispatcher_task* t = NULL;
  struct thread_worker* worker = NULL;
  size_t selected = 0;

  assert(obj && task);

  t = malloc(sizeof(struct thread_dispatcher_task));
  if(!t)
  {
    return -1;
  }

  t->data = task->data;
  t->run = task->run;
  t->cleanup = task->cleanup;
  list_head_init(&t->list);

  /* select the thread to dispatch task */
  selected = color % obj->nb_threads;

  worker = &obj->threads[selected];

  /* enqueue in the selected thread worker queue */
  if(pthread_mutex_lock(&worker->mutex_tasks) == 0)
  {
    int first = list_head_is_empty(&worker->tasks);
    list_head_add_tail(&worker->tasks, &t->list);

    /* notify one of worker threads that wait about a task */
    if(first)
    {
      if(pthread_cond_signal(&worker->cond_tasks) != 0)
      {
        /* it should happen only if cond_tasks is
         * not initialized
         */
        pthread_mutex_unlock(&worker->mutex_tasks);
        return -1;
      }
    }
    pthread_mutex_unlock(&worker->mutex_tasks);
  }
  else
  {
    free(t);
    return -1;
  }

  return 0;
}

int thread_dispatcher_clean(thread_dispatcher obj)
{
  struct list_head* pos = NULL;
  struct list_head* tmp = NULL;
  int run = 0;
  int ret = 0;

  assert(obj);

  run = thread_dispatcher_get_run(obj);

  /* do not clean while running or destroyed */
  if(run != 0)
  {
    return -1;
  }

  for(size_t i = 0 ; i < obj->nb_threads ; i++)
  {
    struct thread_worker* worker = &obj->threads[i];

    if(pthread_mutex_lock(&worker->mutex_tasks) == 0)
    {
      list_head_iterate_safe(&worker->tasks, pos, tmp)
      {
        struct thread_dispatcher_task* t = list_head_get(pos,
            struct thread_dispatcher_task, list);
        list_head_remove(&worker->tasks, &t->list);
        free(t);
      }

      pthread_cond_broadcast(&worker->cond_tasks);
      pthread_mutex_unlock(&worker->mutex_tasks);
    }
    else
    {
      /* mark as not all threads are clean */
      ret = -1;
    }
  }

  return ret;
}

int thread_dispatcher_start(thread_dispatcher obj)
{
  assert(obj);

  if(pthread_mutex_lock(&obj->mutex_start) == 0)
  {
    thread_dispatcher_set_run(obj, 1);
    pthread_cond_broadcast(&obj->cond_start);
    pthread_mutex_unlock(&obj->mutex_start);
  }
  else
  {
    return -1;
  }

  return 0;
}

int thread_dispatcher_stop(thread_dispatcher obj)
{
  assert(obj);

  if(pthread_mutex_lock(&obj->mutex_start) == 0)
  {
    thread_dispatcher_set_run(obj, 0);
    pthread_cond_broadcast(&obj->cond_start);
    pthread_mutex_unlock(&obj->mutex_start);
  }
  else
  {
    return -1;
  }

  return 0;
}

