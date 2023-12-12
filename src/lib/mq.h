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
        key_t key = ftok("./src/main.c", i);
        if (key == -1)
        {
            perror("ftok");
            exit(1);
        }

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

void send_response(Response resp)
{
    Message msg = {.mtype = resp.id + 1};
    sprintf(msg.mtext, "%d", resp.is_available);

    if (msgsnd(response_msgid, &msg, MSG_SIZE, 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }
}

Response get_response(long i)
{
    Message msg;
    Response resp = {.id = i};

    if (msgrcv(response_msgid, &msg, MSG_SIZE, i + 1, 0) == -1)
    {
        perror("msgrcv");
        exit(1);
    }

    sscanf(msg.mtext, "%d", &resp.is_available);
    return resp;
}

void send_liberation(Liberation lib, int liberations_msgid)
{
    Message msg = {.mtype = lib.id + 1};
    sprintf(msg.mtext, "%d,%d,%d", lib.resources.n_1, lib.resources.n_2, lib.resources.n_3);

    if (msgsnd(liberations_msgid, &msg, MSG_SIZE, 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }
}

Liberation get_liberation(int liberations_msgid)
{
    Liberation lib = {.id = -1};
    Message msg;
    if (msgrcv(liberations_msgid, &msg, MSG_SIZE, 0, IPC_NOWAIT) == -1)
    {
        if (errno != ENOMSG)
        {
            perror("msgrcv");
            exit(1);
        }
    }
    else
    {
        lib.id = msg.mtype - 1;
        sscanf(msg.mtext, "%d,%d,%d", &lib.resources.n_1, &lib.resources.n_2, &lib.resources.n_3);
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