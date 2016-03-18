/**
 * \file test_sem_posix.c
 * \brief Tests for POSIX semaphore.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdio.h>
#include <stdlib.h>

#include "test_sem_common.h"

int main(int argc, char** argv)
{
    enum ipc_sem_type type = IPC_SEM_POSIX;

    (void)argc;
    (void)argv;

    test_ipc_print_sem();

    if(test_ipc_sem(type, "/test_sem_posix") == 0)
    {
        return EXIT_SUCCESS;
    }
    
    return EXIT_FAILURE;
}

