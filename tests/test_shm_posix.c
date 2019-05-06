/**
 * \file test_shm_posix.c
 * \brief Tests for POSIX shared memory.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdio.h>
#include <stdlib.h>

#include "test_shm_common.h"

int main(int argc, char** argv)
{
    enum ipc_shm_type type = IPC_SHM_POSIX;

    (void)argc;
    (void)argv;

    test_ipc_print_shm();

    if(test_ipc_shm(type, "/test_shm") == 0)
    {
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

