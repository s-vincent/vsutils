/**
 * \file test_sem_common.h
 * \brief Common functions for semaphorei tests.
 * \author Sebastien Vincent
 * \date 2016
 */

#ifndef TEST_SEM_COMMON_H
#define TEST_SEM_COMMON_H

#include "ipc_sem.h"

/**
 * \brief Print out the supported and not supported message queue types.
 */
void test_ipc_print_sem(void);

/**
 * \brief Tests IPC message queue.
 * \param type IPC message queue type.
 * \param arg argument for message queue.
 * \return 0 if success, -1 otherwise.
 */
int test_ipc_sem(enum ipc_sem_type type, void* arg);

#endif /* TEST_SEM_COMMON_H */

