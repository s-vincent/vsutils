/**
 * \file ipc_sem.c
 * \brief IPC semaphore.
 * \author Sebastien Vincent
 * \date 2016
 */

#if !defined(_WIN32) && !defined(_WIN64)
/* Unix variants */
#include <unistd.h>
#endif

#include "ipc_sem.h"
#include "ipc_sem_impl.h"
#include "ipc_sem_posix.h"
#include "ipc_sem_sysv.h"
/*#include "ipc_sem_win.h"*/

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

ipc_sem ipc_sem_new(enum ipc_sem_type type, void* value, int mode, int perm,
        unsigned int init)
{
    switch(type)
    {
        case IPC_SEM_SYSV:
            return ipc_sem_sysv_new(value, mode, perm, init);
            break;
        case IPC_SEM_POSIX:
            return ipc_sem_posix_new(value, mode, perm, init);
            break;
/*
        case IPC_SEM_WIN:
            return ipc_sem_win_new(value, mode, perm);
            break;
*/
        default:
            return NULL;
    }
}

void ipc_sem_free(ipc_sem* obj, int unlink)
{
    (*obj)->free(obj, unlink);
}

int ipc_sem_lock(ipc_sem obj)
{
    return obj->lock(obj);
}

int ipc_sem_unlock(ipc_sem obj)
{
    return obj->unlock(obj);
}

int ipc_sem_is_supported(enum ipc_sem_type type)
{
#if defined(_WIN32) || defined(_WIN64)
    return type == IPC_SEM_WIN;
#else
    switch(type)
    {
        case IPC_SEM_WIN:
            return 0;
            break;
        case IPC_SEM_SYSV:
            return 1;
            break;
        case IPC_SEM_POSIX:
#if defined(_POSIX_SEMAPHORES)
            return 1;
#else
            return 0;
#endif
            break;
        default:
            return 0;
            break;
    }
#endif
}

enum ipc_sem_type ipc_sem_get_best_type(void)
{
#if defined(_WIN32) || defined(_WIN64)
    return IPC_SEM_WIN;
#elif defined(_POSIX_SEMAPHORES)
    return IPC_SEM_POSIX;
#else
    return IPC_SEM_SYSV;
#endif
}

#ifdef __cplusplus
}
#endif
