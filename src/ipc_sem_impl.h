/**
 * \file ipc_sem_impl.h
 * \brief IPC semaphore definition.
 * \author Sebastien Vincent.
 * \date 2016
 */

#ifndef IPC_SEM_IMPL_H
#define IPC_SEM_IMPL_H

#include "ipc_sem.h"

/**
 * \struct ipc_sem
 * \brief IPC semaphore.
 */
struct ipc_sem
{
    /**
     * \brief Semaphore type.
     */
    enum ipc_sem_type type;
    
    /**
     * \brief Function to free the object.
     */
    void (*free)(ipc_sem*, int);

    /**
     * \brief Function to lock a semaphore.
     */
    int (*lock)(ipc_sem);

    /**
     * \brief Function to unlock a semaphore.
     */
    int (*unlock)(ipc_sem);

    /**
     * \brief Private implementation.
     */
    char priv[];
};


#endif /* IPC_SEM_IMPL_H */

