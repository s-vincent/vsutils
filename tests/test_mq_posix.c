/**
 * \file test_mq_posix.c
 * \brief Tests for POSIX MQ.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdio.h>
#include <stdlib.h>

#include "test_mq_common.h"

int main(int argc, char** argv)
{
    enum ipc_mq_type type = IPC_MQ_POSIX;

    (void)argc;
    (void)argv;

    test_ipc_print_mq();

    if(test_ipc_mq(type, "/test_mq") == 0)
    {
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

