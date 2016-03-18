/**
 * \file test_sem_common.c
 * \brief Common functions for semaphorei tests.
 * \author Sebastien Vincent
 * \date 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "test_sem_common.h"

void test_ipc_print_sem(void)
{
    fprintf(stdout, "SystemV semaphore: %ssupported\n", 
            ipc_sem_is_supported(IPC_SEM_SYSV) ? "" : "not ");
    fprintf(stdout, "POSIX semaphore: %ssupported\n", 
            ipc_sem_is_supported(IPC_SEM_POSIX) ? "" : "not ");
}

int test_ipc_sem(enum ipc_sem_type type, void* arg)
{
    ipc_sem sem = NULL;
    pid_t p = -1;
    struct tm* timeinfo = NULL;
    time_t timeo;
    char buf[256];
  
    if(!ipc_sem_is_supported(type))
    {
        fprintf(stderr, "Semaphore type %d not supported!\n", type);
        return -1;
    }
    
    sem = ipc_sem_new(type, arg, O_CREAT | O_RDWR, 0700, 1);
    if(!sem)
    {
        perror("ipc_sem_new");
        return -1;
    }

    p = fork();

    if(p == -1)
    {
        perror("fork");
        ipc_sem_free(&sem, 1);
        return -1;
    }
    else if(p > 0)
    {
        int status = -1;

        /* father */
        sleep(1);
        time(&timeo);
        timeinfo = localtime(&timeo); 
        sprintf(buf, "[%d %d %d %d:%d:%d]", timeinfo->tm_mday,
                timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        fprintf(stdout, "%s[FATHER] Try to lock semaphore\n", buf);
        ipc_sem_lock(sem);
        
        time(&timeo);
        timeinfo = localtime(&timeo); 
        sprintf(buf, "[%d %d %d %d:%d:%d]", timeinfo->tm_mday,
                timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        fprintf(stdout, "%s[FATHER] Semaphore locked\n", buf);
        ipc_sem_unlock(sem);

        time(&timeo);
        timeinfo = localtime(&timeo); 
        sprintf(buf, "[%d %d %d %d:%d:%d]", timeinfo->tm_mday,
                timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        fprintf(stdout, "%s[FATHER] Semaphore unlocked\n", buf);

        /* wait son termination */
        wait(&status);
        time(&timeo);
        timeinfo = localtime(&timeo); 
        sprintf(buf, "[%d %d %d %d:%d:%d]", timeinfo->tm_mday,
                timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        fprintf(stdout, "%s[FATHER] Son terminated\n", buf);
        fprintf(stdout, "Test OK!\n");
    }
    else
    {
        /* son */
        time(&timeo);
        timeinfo = localtime(&timeo); 
        sprintf(buf, "[%d %d %d %d:%d:%d]", timeinfo->tm_mday,
                timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        fprintf(stdout, "%s[SON] Try to lock semaphore\n", buf);
        ipc_sem_lock(sem);

        time(&timeo);
        timeinfo = localtime(&timeo); 
        sprintf(buf, "[%d %d %d %d:%d:%d]", timeinfo->tm_mday,
                timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        fprintf(stdout, "%s[SON] Semaphore locked\n", buf);
        sleep(3);
        ipc_sem_unlock(sem);

        time(&timeo);
        timeinfo = localtime(&timeo); 
        sprintf(buf, "[%d %d %d %d:%d:%d]", timeinfo->tm_mday,
                timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        fprintf(stdout, "%s[SON] Semaphore unlocked\n", buf);
    }
    
    ipc_sem_free(&sem, 1);
    return 0;
}

