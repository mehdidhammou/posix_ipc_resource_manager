#ifndef MQ_H
#define MQ_H

#include <sys/msg.h>
#include <errno.h>

// message queues
int liberation_msgids[PROCESS_NUM - 1];
int response_msgid;

void init_message_queues()
{
    // create 6 message queues , 5 for the resource liberation and 1 for the response
    for (int i = 0; i < PROCESS_NUM; i++)
    {
        key_t key = ftok("/home/mahdi/Desktop/tp-algo-2/.gitignore", i);
        if (i == PROCESS_NUM - 1)
        {
            response_msgid = msgget(key, 0644 | IPC_CREAT);
            if (response_msgid == -1)
            {
                perror("msgget response");
                exit(1);
            }
        }
        else
        {
            liberation_msgids[i] = msgget(key, 0644 | IPC_CREAT);
            if (liberation_msgids[i] == -1)
            {
                perror("msgget liberations");
                exit(1);
            }
        }
    }
}

void send_response(Response resp, int response_msgid)
{
    resp.id++;
    if (msgsnd(response_msgid, &resp, RESPONSE_SIZE, 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }
}

Response get_response(long int i, int response_msgid)
{
    Response resp;
    if (msgrcv(response_msgid, &resp, RESPONSE_SIZE, i, 0) == -1)
    {
        perror("msgrcv");
        exit(1);
    }
    resp.id--;
    return resp;
}

void send_liberation(Liberation lib, int liberations_msgid)
{
    lib.id++;
    if (msgsnd(liberations_msgid, &lib, LIBERATION_SIZE, 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }
}

Liberation get_liberation(int liberations_msgid)
{
    Liberation lib;
    if (msgrcv(liberations_msgid, &lib, LIBERATION_SIZE, 0, IPC_NOWAIT) == -1)
    {
        if (errno != ENOMSG)
        {
            perror("msgrcv");
            exit(1);
        }
        else
        {
            lib.id = -1;
        }
    }
    else
    {
        lib.id--;
    }
    return lib;
}

void cleanup_mqs()
{
    // cleanup message queues
    for (int i = 0; i < PROCESS_NUM; i++)
    {
        if (i == PROCESS_NUM - 1)
        {
            msgctl(response_msgid, IPC_RMID, NULL);
        }
        else
        {
            msgctl(liberation_msgids[i], IPC_RMID, NULL);
        }
    }
}

#endif