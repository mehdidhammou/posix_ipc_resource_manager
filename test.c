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
    // fork 5 processes that send a message , and 1 process that prints them
    int response_msgid;

    key_t key = ftok("test.c", 1);

    response_msgid = msgget(key, 0644 | IPC_CREAT);
    if (response_msgid == -1)
    {
        perror("msgget response");
        exit(1);
    }

    pid_t pids[5];
    for (int i = 0; i < 5; i++)
    {
        sleep(1);
        pids[i] = fork();
        if (pids[i] == 0)
        {
            Response resp = {i, 1, 0};
            send_response(resp, response_msgid);
            exit(0);
        }
    }

    // wait for all processes to finish
    for (int i = 0; i < 5; i++)
    {
        wait(NULL);
    }

    Response resp;
    for (int i = 0; i < 5; i++)
    {
        double id = (double)i;

        resp = get_response(id, response_msgid);
        printf("response from process %ld\n", resp.id);
    }

    // destroy message queues
    if (msgctl(response_msgid, IPC_RMID, NULL) == -1)
    {
        perror("msgctl");
        exit(1);
    }
    return 0;
}