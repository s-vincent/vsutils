/**
 * \file test_mq_sysv.c
 * \brief Tests for SystemV MQ.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdio.h>
#include <stdlib.h>

#include "test_mq_common.h"

#define MAGIC_COOKIE_MQ (key_t)0xCAFE

int main(int argc, char** argv)
{
    enum ipc_mq_type type = IPC_MQ_SYSV;
    enum ipc_mq_type best_type = ipc_mq_get_best_type();

    (void)argc;
    (void)argv;

    test_ipc_print_mq();

    if(best_type != type)
    {
        fprintf(stdout, "SystemV MQ is not the best type for this OS "
                "(best type is %d).\n", best_type);
    }

    if(test_ipc_mq(type, (void*)MAGIC_COOKIE_MQ) == 0)
    {
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

