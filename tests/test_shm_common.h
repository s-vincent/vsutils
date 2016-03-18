/**
 * \file test_shm_common.h
 * \brief Common functions for shared memory tests.
 * \author Sebastien Vincent
 * \date 2016
 */

#ifndef TEST_SHM_COMMON_H
#define TEST_SHM_COMMON_H

#include "ipc_shm.h"

/**
 * \brief Print out the supported and not supported shared memory types.
 */
void test_ipc_print_shm(void);

/**
 * \brief Tests IPC shared memory.
 * \param type IPC shared memory type.
 * \param arg argument for shared memory.
 * \return 0 if success, -1 otherwise.
 */
int test_ipc_shm(enum ipc_shm_type type, void* arg);

#endif /* TEST_SHM_COMMON_H */

