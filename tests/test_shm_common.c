#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "test_shm_common.h"

void test_ipc_print_shm(void)
{
    fprintf(stdout, "SystemV SHM: %ssupported\n", 
            ipc_shm_is_supported(IPC_SHM_SYSV) ? "" : "not ");
    fprintf(stdout, "POSIX SHM: %ssupported\n", 
            ipc_shm_is_supported(IPC_SHM_POSIX) ? "" : "not ");
}

int test_ipc_shm(enum ipc_shm_type type, void* arg)
{
    ipc_shm shm = NULL;
    const size_t shm_size = 1024;
    pid_t p = -1;
  
    if(!ipc_shm_is_supported(type))
    {
        fprintf(stderr, "Shared memory type %d not supported!\n", type);
        return -1;
    }

    p = fork();

    if(p == -1)
    {
        perror("fork");
        return -1;
    }
    else if(p > 0)
    {
        /* father */
        int status = -1;
        shm = ipc_shm_new(type, arg, O_CREAT | O_RDWR, 0700, shm_size);
        char* mem = NULL;

        if(!shm)
        {
            perror("ipc_shm_new");
            return -1;
        }

        mem = ipc_shm_get_data(shm);
        mem[0] = 'T';
        mem[1] = 'E';
        mem[2] = 'S';
        mem[3] = 'T';
        mem[4] = 0x00;

        fprintf(stdout, "[FATHER] Wait process\n");
        wait(&status);
        fprintf(stdout, "[FATHER] Test OK!\n");
    }
    else
    {
        /* son */
        char* mem = NULL;

        shm = ipc_shm_new(type, arg, O_CREAT | O_RDWR, 0700, shm_size);
        
        if(!shm)
        {
            perror("ipc_shm_new");
            return -1;
        }

        mem = ipc_shm_get_data(shm);
        sleep(1);

        fprintf(stdout, "[SON] %s\n", mem);
    }

   
    ipc_shm_free(&shm, 1);
    return 0;
}

