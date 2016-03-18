/**
 * \file ipc_shm_impl.h
 * \brief IPC shared memory definition.
 * \author Sebastien Vincent.
 * \date 2016
 */

#ifndef IPC_SHM_IMPL_H
#define IPC_SHM_IMPL_H

#include "ipc_shm.h"

/**
 * \struct ipc_shm
 * \brief IPC shared memory.
 */
struct ipc_shm
{
    /**
     * \brief Shared memory type.
     */
    enum ipc_shm_type type;

    /**
     * \brief Shared memory data.
     */
    void* data;
   
    /**
     * \brief Shared memory size.
     */
    size_t data_size;

    /**
     * \brief Function to free the object.
     */
    void (*free)(ipc_shm*, int);

    /**
     * \brief Function to get shared data pointer.
     */
    void* (*get_data)(struct ipc_shm* obj);

    /**
     * \brief Function to get shared data size.
     */
    size_t (*get_data_size)(struct ipc_shm* obj);

    /**
     * \brief Private implementation.
     */
    char priv[];
};

#endif /* IPC_SHM_IMPL_H */

