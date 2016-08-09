/**
 * \file pthread_osx.h
 * \brief POSIX threads functions that are not supported on vanilla OS X.
 * \author Sebastien Vincent
 * \date 2016
 */

#ifndef VSUTILS_PTHREAD_OSX
#define VSUTILS_PTHREAD_OSX

#ifdef __APPLE__

#include <libkern/OSAtomic.h>

/**
 * \typedef pthread_spinlock_t
 * \brief Typedef for pthread_spinlock_t to match OS X's OSSpinLock.
 */
typedef int32_t /*OSSpinLock*/ pthread_spinlock_t;

/**
 * \brief Implementation of pthread_spin_init for MacOS X.
 * \param spin spinlock to initialize.
 * \param pshared shared status (not used).
 * \return 0 if success, error value otherwise.
 */
int pthread_spin_init(pthread_spinlock_t* lock, int pshared);

/**
 * \brief Implementation of pthread_spin_destroy for MacOS X.
 * \param spin spinlock to destroy.
 * \return 0 if success, error value otherwise.
 */
int pthread_spin_destroy(pthread_spinlock_t* lock);

/**
 * \brief Implementation of pthread_spin_lock for MacOS X.
 * \param spin spinlock to lock.
 * \return 0 if success, error value otherwise.
 */
int pthread_spin_lock(pthread_spinlock_t* lock);

/**
 * \brief Implementation of pthread_spin_trylock for MacOS X.
 * \param spin spinlock to lock.
 * \return 0 if success, error value otherwise.
 */
int pthread_spin_trylock(pthread_spinlock_t* lock);

/**
 * \brief Implementation of pthread_spin_unlock for MacOS X.
 * \param spin spinlock to unlock.
 * \return 0 if success, error value otherwise.
 */
int pthread_spin_unlock(pthread_spinlock_t* lock);

/**
 * \brief Implementation of pthread_mutex_timedlock for MacOS X.
 * \param mutex the mutex.
 * \param abs_timeout the absolute timeout.
 * \return 0 if success, error value otherwise.
 */
int pthread_mutex_timedlock(pthread_mutex_t* mutex,
    const struct timespec* abs_timeout);

#endif

#endif /* VSUTILS_PTHREAD_OSX */

