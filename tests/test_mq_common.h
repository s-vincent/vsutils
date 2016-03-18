/**
 * \file test_mq_common.h
 * \brief Common functions for MQ tests.
 * \author Sebastien Vincent
 * \date 2016
 */

#ifndef TEST_MQ_COMMON_H
#define TEST_MQ_COMMON_H

#include "ipc_mq.h"

/**
 * \brief Print out the supported and not supported message queue types.
 */
void test_ipc_print_mq(void);

/**
 * \brief Tests IPC message queue.
 * \param type IPC message queue type.
 * \param arg argument for message queue.
 * \return 0 if success, -1 otherwise.
 */
int test_ipc_mq(enum ipc_mq_type type, void* arg);

#endif /* TEST_MQ_COMMON_H */

