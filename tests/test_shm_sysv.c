/**
 * \file test_shm_sysv.c
 * \brief Tests for SystemV shared memory.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>

#include "test_shm_common.h"

#define MAGIC_COOKIE_SHM (key_t)0xCAFE

int main(int argc, char** argv)
{
    enum ipc_shm_type type = IPC_SHM_SYSV;
    enum ipc_shm_type best_type = ipc_shm_get_best_type();

    (void)argc;
    (void)argv;

    test_ipc_print_shm();

    if(best_type != type)
    {
        fprintf(stdout, "SystemV SHM is not the best type for this OS "
                "(best type is %d).\n", best_type);
    }

    if(test_ipc_shm(type, (void*)MAGIC_COOKIE_SHM) == 0)
    {
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

