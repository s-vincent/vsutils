#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "test_mq_common.h"

void test_ipc_print_mq(void)
{
    fprintf(stdout, "SystemV MQ: %ssupported\n", 
            ipc_mq_is_supported(IPC_MQ_SYSV) ? "" : "not ");
    fprintf(stdout, "POSIX MQ: %ssupported\n", 
            ipc_mq_is_supported(IPC_MQ_POSIX) ? "" : "not ");
}

int test_ipc_mq(enum ipc_mq_type type, void* arg)
{
    ipc_mq mq = NULL;
    struct ipc_mq_data* input = NULL;
    struct ipc_mq_data* output = NULL;
    size_t msg_size = 0;
  
    if(!ipc_mq_is_supported(type))
    {
        fprintf(stderr, "MQ type %d not supported!\n", type);
        return -1;
    }

    mq = ipc_mq_new(type, arg, O_CREAT | O_RDWR, 0700);

    if(!mq)
    {
        perror("ipc_mq_new");
        free(input);
        free(output);
        return -1;
    }
   
    msg_size = ipc_mq_get_max_msg_size(mq);
    fprintf(stdout, "Size of message: %lu\n", msg_size);
    input = malloc(sizeof(struct ipc_mq_data) + msg_size);
    output = malloc(sizeof(struct ipc_mq_data) + msg_size);
    
    if(!input || !output)
    {
        free(input);
        free(output);
        ipc_mq_free(&mq, 1);
        fprintf(stderr, "Error allocating buffer!\n");
        return -1;
    }

    input->priv = 1;
    memset(input->data, 0x00, msg_size);
    output->priv = 1;
    memset(output->data, 0x00, msg_size);

    strncpy(input->data, "TEST", sizeof("TEST"));

    if(ipc_mq_send(mq, input, msg_size) != -1)
    {
        fprintf(stdout, "MQ send success\n");

        if(ipc_mq_recv(mq, output, msg_size) == -1)
        {
            perror("MQ receive failed");
        }
        else
        {
            fprintf(stdout, "Message received: %s\n", output->data);
        }
    }
    else
    {
        perror("MQ send failed");
    }

    free(input);
    free(output);
    ipc_mq_free(&mq, 1);
    fprintf(stdout, "Test OK!\n");
    return 0;
}

