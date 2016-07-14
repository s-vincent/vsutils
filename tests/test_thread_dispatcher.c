/**
 * \file test_thread_dispatcher.c
 * \brief Tests for thread dispatcher.
 * \author Sebastien Vincent
 * \date 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#include "thread_dispatcher.h"

/**
 * \brief Run function.
 * \param data user data.
 */
static void fcn_run(void* data)
{
  fprintf(stderr, "Task %p executed\n", data);
}
/**
 * \brief Cleanup function.
 * \param data user data.
 */
static void fcn_cleanup(void* data)
{
  fprintf(stderr, "Task %p cleanup\n", data);
}

/**
 * \brief Entry point of the program.
 * \param argc number of arguments.
 * \param argv array of arguments.
 * \return EXIT_SUCCESS or EXIT_FAILURE.
 */
int main(int argc, char** argv)
{
  const size_t tasks_size = 20;
  thread_dispatcher th = NULL;
  struct thread_task tasks[tasks_size];

  (void)argc;
  (void)argv;

  fprintf(stdout, "Begin\n");
  th = thread_dispatcher_new(10);
  fprintf(stdout, "Thread dispatcher: %p\n", (void*)th);

  if(!th)
  {
    fprintf(stderr, "Failed to create dispatcher errno=%d\n",
        errno);
    exit(EXIT_FAILURE);
  }

  for(unsigned int i = 0 ; i < tasks_size ; i++)
  {
    tasks[i].data = (void*)(uintptr_t)i;
    tasks[i].run = fcn_run;
    tasks[i].cleanup = fcn_cleanup;

    if(thread_dispatcher_push(th, &tasks[i]) != 0)
    {
      fprintf(stderr, "Failed to add task %u\n", i);
    }
  }

  thread_dispatcher_start(th);
  sleep(3);

  fprintf(stdout, "Stop stuff\n");
  thread_dispatcher_stop(th);
  fprintf(stdout, "Free stuff\n");
  thread_dispatcher_free(&th);
  fprintf(stdout, "OK\n");

  fprintf(stdout, "End\n");

  return EXIT_SUCCESS;
}

