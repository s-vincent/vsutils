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
 * \file thread_pool.c
 * \brief Thread pool for tasks.
 * \author Sebastien Vincent
 * \date 2014-2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#include "thread_pool.h"

/**
 * \struct thread_pool.
 * \brief Thread pool.
 */
struct thread_pool
{
  struct list_head tasks; /**< List of tasks. */
  pthread_mutex_t mutex_tasks; /**< Mutex to protect tasks list. */
  pthread_cond_t cond_tasks; /**< Condition for push/pop tasks. */
  pthread_mutex_t mutex_start; /**< Mutex to protect the start condition. */
  pthread_cond_t cond_start; /**< Condition to notify the start. */
  volatile sig_atomic_t run; /**< Status of the pool. */
  size_t nb_threads; /**< Number of threads. */
  pthread_t* threads; /**< Array of worker threads. */
};

/**
 * \brief Pop the first task of the thread pool.
 * \param obj thread pool.
 * \param task task that will be popped, it will be filled with data from the
 * manager.
 * \return 0 if success, -1 on failure.
 * \note This function is blocking until a task is available.
 */
int thread_pool_pop(thread_pool obj, struct thread_pool_task* task)
{
  struct thread_pool_task* t = NULL;
  int ret = -1;

  pthread_mutex_lock(&obj->mutex_tasks);
  {
    struct list_head* pos = NULL;

    if(obj->run <= 0)
    {
      pthread_mutex_unlock(&obj->mutex_tasks);
      return -1;
    }

    while(list_head_is_empty(&obj->tasks))
    {
      /* wait for a task */
      pthread_cond_wait(&obj->cond_tasks, &obj->mutex_tasks);

      /* condition signaled or spurious wake up, check if stop/exit */
      if(obj->run <= 0)
      {
      	pthread_mutex_unlock(&obj->mutex_tasks);
        return -1;
      }
    }

    pos = obj->tasks.next;
    t = list_head_get(pos, struct thread_pool_task, list);
    ret = 0;

    /* fill task with thread_pool_task data from list */
    task->data = t->data;
    task->run = t->run;
    task->cleanup = t->cleanup;

    /* remove task from list */
    list_head_remove(&obj->tasks, &t->list);
    pthread_mutex_unlock(&obj->mutex_tasks);
  }

  if(ret == 0)
  {
    free(t);
  }

  return ret;
}

/**
 * \brief Worker thread function that wait for a task to execute.
 * \param data the thread pool.
 * \return NULL.
 */
static void* thr_worker(void* data)
{
  struct thread_pool* pool = data;

  if(!data)
  {
    return NULL;
  }

  while(pool->run >= 0)
  {
    struct thread_pool_task task;

    if(pool->run == 0)
    {
      /* stop case */
      pthread_mutex_lock(&pool->mutex_start);
      {
        while(pool->run == 0)
        {
          pthread_cond_wait(&pool->cond_start, &pool->mutex_start);
        }
      }
      pthread_mutex_unlock(&pool->mutex_start);
      continue;
    }
    else if(pool->run == -1)
    {
      /* free case */
      break;
    }
    else
    {
      /* running case */
      if(thread_pool_pop(pool, &task) == 0)
      {
        /* process task then cleanup */
        task.run(task.data);
        task.cleanup(task.data);
      }
      else
      {
        /* timeout */
      }
    }
  }

  return NULL;
}

thread_pool thread_pool_new(size_t nb)
{
  struct thread_pool* ret = NULL;
  pthread_mutexattr_t mutexattr;

  if(nb == 0)
  {
    return NULL;
  }

  ret = malloc(sizeof(struct thread_pool) + (sizeof(pthread_t) * nb));
  if(!ret)
  {
    return NULL;
  }

  ret->run = 0;
  list_head_init(&ret->tasks);
  /* memory is already reserved for threads member */
  ret->threads = (pthread_t*)(((char*)ret) + sizeof(struct thread_pool));
  memset(ret->threads, 0x00, sizeof(pthread_t));
  ret->nb_threads = 0;

  if((pthread_mutexattr_init(&mutexattr) != 0) ||
      (pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_NORMAL)))
  {
    free(ret);
    return NULL;
  }

  if(pthread_mutex_init(&ret->mutex_tasks, &mutexattr) != 0)
  {
    pthread_mutexattr_destroy(&mutexattr);
    free(ret);
    return NULL;
  }

  pthread_mutexattr_destroy(&mutexattr);

  if(pthread_cond_init(&ret->cond_tasks, NULL) != 0)
  {
    pthread_mutex_destroy(&ret->mutex_tasks);
    free(ret);
    return NULL;
  }

  if(pthread_mutex_init(&ret->mutex_start, NULL) != 0)
  {
    pthread_mutex_destroy(&ret->mutex_tasks);
    pthread_cond_destroy(&ret->cond_tasks);
    free(ret);
    return NULL;
  }

  if(pthread_cond_init(&ret->cond_start, NULL) != 0)
  {
    pthread_mutex_destroy(&ret->mutex_tasks);
    pthread_cond_destroy(&ret->cond_tasks);
    pthread_mutex_destroy(&ret->mutex_start);
    free(ret);
    return NULL;
  }

  for(size_t i = 0 ; i < nb ; i++)
  {
    if(pthread_create(&ret->threads[i], NULL, thr_worker, ret) != 0)
    {
      break;
    }
    else
    {
      ret->nb_threads++;
    }
  }

  if(ret->nb_threads != nb)
  {
    ret->run = -1;

    /* error creating all threads, cancel the created ones */
    for(size_t i = 0 ; i < ret->nb_threads ; i++)
    {
      pthread_mutex_lock(&ret->mutex_start);
      {
        /* tell the threads that they have to quit */
        pthread_cond_broadcast(&ret->cond_start);
      }
      pthread_mutex_unlock(&ret->mutex_start);

      //pthread_cancel(ret->threads[i]);
      pthread_join(ret->threads[i], NULL);
    }

    pthread_mutex_destroy(&ret->mutex_tasks);
    pthread_cond_destroy(&ret->cond_tasks);
    pthread_mutex_destroy(&ret->mutex_start);
    pthread_cond_destroy(&ret->cond_start);
    free(ret);
    return NULL;
  }

  return ret;
}

void thread_pool_free(thread_pool* obj)
{
  (*obj)->run = -1;

  /* unblock threads waiting tasks */
  pthread_mutex_lock(&(*obj)->mutex_tasks);
  {
    /* tell the threads that they have to quit */
    pthread_cond_broadcast(&(*obj)->cond_tasks);
  }
  pthread_mutex_unlock(&(*obj)->mutex_tasks);

  /* unblock threads in stop state */
  pthread_mutex_lock(&(*obj)->mutex_start);
  {
    /* tell the threads that they have to quit */
    (*obj)->run = -1;
    pthread_cond_broadcast(&(*obj)->cond_start);
  }
  pthread_mutex_unlock(&(*obj)->mutex_start);

  /* wait for the threads */
  for(size_t i = 0 ; i < (*obj)->nb_threads ; i++)
  {
    pthread_join((*obj)->threads[i], NULL);
  }

  /* cleanup the rest of tasks if any */
  thread_pool_clean(*obj);

  /* do not care about success or failure of these calls */
  pthread_mutex_destroy(&(*obj)->mutex_tasks);
  pthread_cond_destroy(&(*obj)->cond_tasks);
  pthread_mutex_destroy(&(*obj)->mutex_start);
  pthread_cond_destroy(&(*obj)->cond_start);

  free(*obj);
  *obj = NULL;
}

int thread_pool_push(thread_pool obj, struct thread_pool_task* task)
{
  struct thread_pool_task* t = NULL;

  t = malloc(sizeof(struct thread_pool_task));
  if(!t)
  {
    return -1;
  }

  t->data = task->data;
  t->run = task->run;
  t->cleanup = task->cleanup;
  list_head_init(&t->list);

  pthread_mutex_lock(&obj->mutex_tasks);
  {
    int first = list_head_is_empty(&obj->tasks);
    list_head_add_tail(&obj->tasks, &t->list);

    /* notify one of worker threads that wait about a task */
    if(first)
    {
      if(pthread_cond_signal(&obj->cond_tasks) != 0)
      {
        /* it should happen only if cond_tasks is
         * not initialized
         */
        pthread_mutex_unlock(&obj->mutex_tasks);
        return -1;
      }
    }
  }
  pthread_mutex_unlock(&obj->mutex_tasks);
  return 0;
}

int thread_pool_clean(thread_pool obj)
{
  /* do not clean while running */
  if(obj->run == 1)
  {
    return -1;
  }

  pthread_mutex_lock(&obj->mutex_tasks);
  {
    struct list_head* pos = NULL;
    struct list_head* tmp = NULL;

    list_head_iterate_safe(&obj->tasks, pos, tmp)
    {
      struct thread_pool_task* t = list_head_get(pos,
          struct thread_pool_task, list);
      list_head_remove(&obj->tasks, &t->list);
      free(t);
    }

    pthread_cond_broadcast(&obj->cond_tasks);
  }
  pthread_mutex_unlock(&obj->mutex_tasks);

  return 0;
}

void thread_pool_start(thread_pool obj)
{
  pthread_mutex_lock(&obj->mutex_start);
  {
    obj->run = 1;
    pthread_cond_broadcast(&obj->cond_start);
  }
  pthread_mutex_unlock(&obj->mutex_start);
}

void thread_pool_stop(thread_pool obj)
{
  pthread_mutex_lock(&obj->mutex_start);
  {
    obj->run = 0;
    pthread_cond_broadcast(&obj->cond_start);
  }
  pthread_mutex_unlock(&obj->mutex_start);
}

