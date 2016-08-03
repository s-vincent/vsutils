/**
 * \file thread_dispatcher.c
 * \brief Thread dispatcher for tasks.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
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
   * \brief Status of the dispatcher.
   */
  volatile sig_atomic_t run;

  /**
   * \brief Next thread number for "random" thread selection.
   */
  size_t next_select;

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

  pthread_mutex_lock(&worker->mutex_tasks);
  {
    struct list_head* pos = NULL;

    /* if arrived after condition signaled */
    if(worker->dispatcher->run <= 0)
    {
      pthread_mutex_unlock(&worker->mutex_tasks);
      return -1;
    }

    while(list_head_is_empty(&worker->tasks))
    {
      /* wait for a task */
      pthread_cond_wait(&worker->cond_tasks, &worker->mutex_tasks);

      /* condition signaled or spurious wake up, check if stop/exit */
      if(worker->dispatcher->run <= 0)
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

  assert(worker);

  dispatcher = worker->dispatcher;

  /*
   * block signals because this worker thread will not do signal handler
   * management
   */
  sigset_t mask;
  sigfillset(&mask);
  pthread_sigmask(SIG_BLOCK, &mask, NULL);

  while(dispatcher->run >= 0)
  {
    struct thread_dispatcher_task task;

    memset(&task, 0x00, sizeof(struct thread_dispatcher_task));

    if(dispatcher->run == 0)
    {
      /* stop case */
      pthread_mutex_lock(&dispatcher->mutex_start);
      {
        /* wait for start */
        while(dispatcher->run == 0)
        {
          pthread_cond_wait(&dispatcher->cond_start, &dispatcher->mutex_start);
        }
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

  ret = malloc(sizeof(struct thread_dispatcher) + (sizeof(struct thread_worker) * nb));
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

      /* join the threads and destroy private thread worker stuff */
      pthread_join(ret->threads[i].id, NULL);
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
    struct thread_worker* worker = &(*obj)->threads[i];

    pthread_mutex_lock(&worker->mutex_tasks);
    {
      /*
       * unblock worker waiting for tasks
       * worker will then check for run variable (-1) and exit
       */
      pthread_cond_signal(&worker->cond_tasks);;
    }
    pthread_mutex_unlock(&worker->mutex_tasks);

    pthread_join(worker->id, NULL);
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
  int color = 0;

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
  int select = 0;

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
  select = color % obj->nb_threads;

  printf("select: %d %zu\n", select, obj->nb_threads);
  worker = &obj->threads[select];

  /* enqueue in the selected thread worker queue */
  pthread_mutex_lock(&worker->mutex_tasks);
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
  }
  pthread_mutex_unlock(&worker->mutex_tasks);
  return 0;
}

int thread_dispatcher_clean(thread_dispatcher obj)
{
  struct list_head* pos = NULL;
  struct list_head* tmp = NULL;

  assert(obj);

  /* do not clean while running or destroyed */
  if(obj->run != 0)
  {
    return -1;
  }

  for(size_t i = 0 ; i < obj->nb_threads ; i++)
  {
    struct thread_worker* worker = &obj->threads[i];

    pthread_mutex_lock(&worker->mutex_tasks);
    {
      list_head_iterate_safe(&worker->tasks, pos, tmp)
      {
        struct thread_dispatcher_task* t = list_head_get(pos,
            struct thread_dispatcher_task, list);
        list_head_remove(&worker->tasks, &t->list);
        free(t);
      }
      pthread_cond_broadcast(&worker->cond_tasks);
    }
    pthread_mutex_unlock(&worker->mutex_tasks);
  }

  return 0;
}

void thread_dispatcher_start(thread_dispatcher obj)
{
  assert(obj);

  pthread_mutex_lock(&obj->mutex_start);
  {
    obj->run = 1;
    pthread_cond_broadcast(&obj->cond_start);
  }
  pthread_mutex_unlock(&obj->mutex_start);
}

void thread_dispatcher_stop(thread_dispatcher obj)
{
  assert(obj);

  pthread_mutex_lock(&obj->mutex_start);
  {
    obj->run = 0;
    pthread_cond_broadcast(&obj->cond_start);
  }
  pthread_mutex_unlock(&obj->mutex_start);
}

