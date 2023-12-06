#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <signal.h>

typedef struct
{
    long id;
    int type;
    int result;
} Response;

#define RESPONSE_SIZE sizeof(Response)
void send_response(Response resp, int response_msgid)
{
    // send the response to the process
    printf("arguments : %ld, %ld\n", resp.id, RESPONSE_SIZE);
    if (msgsnd(response_msgid, &resp, RESPONSE_SIZE, 0) == -1)
    {
        printf("error from send_response, process %ld\n", resp.id);
        perror("msgsnd");
        exit(1);
    }
}

Response get_response(int i, int response_msgid)
{
    Response resp;
    if (msgrcv(response_msgid, &resp, RESPONSE_SIZE, i, 0) == -1)
    {
        perror("msgrcv");
        exit(1);
    }

    return resp;
}

int main()
{
    // fork 5 processes and send a message, and 1 process to receive the message
    printf("start time : %ld\n", time(NULL) - time(NULL));
    
    return 0;
}