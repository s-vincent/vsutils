/**
 * \file pthread_osx.h
 * \brief POSIX threads functions that are not supported on vanilla OS X.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "pthread_osx.h"

#ifdef __APPLE__

int pthread_spin_init(pthread_spinlock_t* lock, int pshared)
{
  (void)pshared;

  *lock = OS_SPINLOCK_INIT;
  return 0;
}

int pthread_spin_destroy(pthread_spinlock_t* lock)
{
  if(*lock != OS_SPINLOCK_INIT)
  {
    /* spinlock already locked, cannot destroy */
    return EBUSY;
  }

  *lock = OS_SPINLOCK_INIT;
  return 0;
}

int pthread_spin_lock(pthread_spinlock_t* lock)
{
  /* OSSpinLockLock does not have return value */
  OSSpinLockLock(lock);
  return 0;
}

int pthread_spin_trylock(pthread_spinlock_t* lock)
{
  if(OSSpinLockTry(lock))
  {
    return 0;
  }
  else
  {
    return EBUSY;
  }
}

int pthread_spin_unlock(pthread_spinlock_t* lock)
{
  /* OSSpinLockUnlock does not have return value */
  OSSpinLockUnlock(lock);
  return 0;
}

int priv_pthread_mutex_timedlock(pthread_mutex_t* mutex,
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

      /* do not act as a spinlock...*/
      sched_yield();
    }
  }
  while(ret == EBUSY);

  return ret;
}

#endif

