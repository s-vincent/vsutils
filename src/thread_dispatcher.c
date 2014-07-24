/**
 * \file thread_dispatcher.c
 * \brief Thread dispatcher for tasks.
 * \author Sebastien Vincent
 * \date 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#if !defined(_WIN32) && !defined(_WIN64)
#include <unistd.h>
#endif

#include "thread_dispatcher.h"

#ifdef __MACH__
/* MacOS X does not have pthread_mutex_timedlock */

/**
 * \brief Implementation of pthread_mutex_timedlock for MacOS X.
 * \param mutex the mutex.
 * \param abs_timeout the absolute timeout.
 * \return 0 if success, error value otherwise.
 */
static int priv_pthread_mutex_timedlock(pthread_mutex_t* mutex,
    const struct timespec* abs_timeout)
{
  int ret = 0;
  struct timeval tv;
  struct timespec ts;

  ts.tv_sec = 0;
  ts.tv_nsec = 10000000;

  do
  {
    ret = pthread_mutex_trylock(mutex);
  
    if(ret == EBUSY)
    {
      gettimeofday(&tv, NULL);
      if(tv.tv_sec >= abs_timeout->tv_sec &&
          (tv.tv_usec * 1000) >= abs_timeout->tv_nsec)
      {
        return ETIMEDOUT;
      }
    }
  }
  while(ret == EBUSY);

  return ret;
}

#define pthread_mutex_timedlock priv_pthread_mutex_timedlock
#endif

/**
 * \def DISPATCHER_CONDITION_TIMEOUT
 * \brief Timeout to wait for an item to pop from the list.
 */
#define DISPATCHER_CONDITION_TIMEOUT 5

/**
 * \struct thread_dispatcher.
 * \brief Thread dispatcher.
 */
struct thread_dispatcher
{
  struct list_head tasks; /**< List of tasks. */
  pthread_mutex_t mutex_tasks; /**< Mutex to protect tasks list. */
  pthread_cond_t cond_tasks; /**< Condition for push/pop tasks. */
  pthread_mutex_t mutex_start; /**< Mutex to protect the start condition. */
  pthread_cond_t cond_start; /**< Condition to notify the start. */
  volatile sig_atomic_t run; /**< Status of the dispatcher. */
  unsigned int nb_threads; /**< Number of threads. */
  pthread_t* threads; /**< Array of worker threads. */
};

/**
 * \brief Worker thread function that wait for a task to execute.
 * \param data the thread dispatcher.
 * \return NULL.
 */
static void* thr_worker(void* data)
{
  struct thread_dispatcher* dispatcher = data;

  if(!data)
  {
    return NULL;
  }

  while(dispatcher->run >= 0)
  {
    struct thread_task task;

    if(dispatcher->run == 0)
    {
      /* stop case */
      pthread_mutex_lock(&dispatcher->mutex_start);
      {
        pthread_cond_wait(&dispatcher->cond_start, &dispatcher->mutex_start);
      }
      pthread_mutex_unlock(&dispatcher->mutex_start);
      continue;
    }
    else if(dispatcher->run == -1)
    {
      /* free case */
      break;
    }
    else
    {
      /* running case */

      if(thread_dispatcher_pop(dispatcher, &task) == 0)
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

thread_dispatcher thread_dispatcher_new(unsigned nb)
{
  struct thread_dispatcher* ret = NULL;
  pthread_mutexattr_t mutexattr;

  if(nb == 0)
  {
    return NULL;
  }

  ret = malloc(sizeof(struct thread_dispatcher) + (sizeof(pthread_t) * nb));
  if(!ret)
  {
    return NULL;
  }

  ret->run = 0;
  list_head_init(&ret->tasks);
  /* memory is already reserved for threads member */
  ret->threads = (pthread_t*)(((char*)ret) + sizeof(struct thread_dispatcher));
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

  for(unsigned int i = 0 ; i < nb ; i++)
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
    for(unsigned int i = 0 ; i < ret->nb_threads ; i++)
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

void thread_dispatcher_free(thread_dispatcher* obj)
{
  pthread_mutex_lock(&(*obj)->mutex_start);
  {
    /* tell the threads that they have to quit */
    (*obj)->run = -1;
    pthread_cond_broadcast(&(*obj)->cond_start);
  } 
  pthread_mutex_unlock(&(*obj)->mutex_start);

  /* wait for the threads */
  for(unsigned int i = 0 ; i < (*obj)->nb_threads ; i++)
  {
    pthread_join((*obj)->threads[i], NULL);
  }

  /* cleanup the rest of tasks if any */
  thread_dispatcher_clean(*obj);

  /* do not care about success or failure of these calls */
  pthread_mutex_destroy(&(*obj)->mutex_tasks);
  pthread_cond_destroy(&(*obj)->cond_tasks);
  pthread_mutex_destroy(&(*obj)->mutex_start);
  pthread_cond_destroy(&(*obj)->cond_start); 

  free(*obj);
  *obj = NULL;
}

int thread_dispatcher_push(thread_dispatcher obj, struct thread_task* task)
{
  struct thread_task* t = NULL;

  t = malloc(sizeof(struct thread_task));
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

    /* notify threads that wait about a task */
    if(first)
    {
      if(pthread_cond_broadcast(&obj->cond_tasks) != 0)
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

int thread_dispatcher_pop(thread_dispatcher obj, struct thread_task* task)
{
  struct thread_task* t = NULL;
  struct list_head* pos;
  struct timespec ts;
  int ret = 0;

#if defined(_POSIX_TIMERS) && !defined(__MACH__)
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += DISPATCHER_CONDITION_TIMEOUT;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  ts.tv_sec = tv.tv_sec + DISPATCHER_CONDITION_TIMEOUT;
  ts.tv_nsec = 0;
#endif

  pthread_mutex_timedlock(&obj->mutex_tasks, &ts);
  {
    if(list_head_is_empty(&obj->tasks))
    {
      /* wait for a task */
      if(pthread_cond_timedwait(&obj->cond_tasks,
            &obj->mutex_tasks, &ts) != 0)
      {
        pthread_mutex_unlock(&obj->mutex_tasks);
        return -1;
      }
    }

    if(list_head_is_empty(&obj->tasks))
    {
      pthread_mutex_unlock(&obj->mutex_tasks);
      return -1;
    }

    ret = 0;

    pos = obj->tasks.next;
    t = list_head_get(pos, struct thread_task, list);

    /* fill task with thread_task data from list */
    task->data = t->data;
    task->run = t->run;
    task->cleanup = t->cleanup;

    /* remove task from list */
    list_head_remove(&obj->tasks, &t->list);
  }
  pthread_mutex_unlock(&obj->mutex_tasks);

  if(ret == 0)
  {
    free(t);
  }

  return ret;
}

int thread_dispatcher_clean(thread_dispatcher obj)
{
  struct list_head* pos = NULL;
  struct list_head* tmp = NULL;

  pthread_mutex_lock(&obj->mutex_tasks);
  {
    list_head_iterate_safe(&obj->tasks, pos, tmp)
    {
      struct thread_task* t = list_head_get(pos,
          struct thread_task, list);
      list_head_remove(&obj->tasks, &t->list);
      free(t);
    }

    pthread_cond_broadcast(&obj->cond_tasks);
  }
  pthread_mutex_unlock(&obj->mutex_tasks);

  return 0;
}

void thread_dispatcher_start(thread_dispatcher obj)
{
  pthread_mutex_lock(&obj->mutex_start);
  {
    obj->run = 1;
    pthread_cond_broadcast(&obj->cond_start);
  }
  pthread_mutex_unlock(&obj->mutex_start);
}

void thread_dispatcher_stop(thread_dispatcher obj)
{
  pthread_mutex_lock(&obj->mutex_start);
  {
    obj->run = 0;
    pthread_cond_broadcast(&obj->cond_start);
  }
  pthread_mutex_unlock(&obj->mutex_start);
}

