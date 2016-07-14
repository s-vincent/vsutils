/**
 * \file test_sem_sysv.c
 * \brief Tests for SystemV semaphore.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdio.h>
#include <stdlib.h>

#include "test_sem_common.h"

#define MAGIC_COOKIE_SEM (key_t)0xCAFE

int main(int argc, char** argv)
{
    enum ipc_sem_type type = IPC_SEM_SYSV;
    enum ipc_sem_type best_type = ipc_sem_get_best_type();

    (void)argc;
    (void)argv;

    test_ipc_print_sem();

    if(best_type != type)
    {
        fprintf(stdout, "SystemV semaphore is not the best type for this OS "
                "(best type is %d).\n", best_type);
    }

    if(test_ipc_sem(type, (void*)MAGIC_COOKIE_SEM) == 0)
    {
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

