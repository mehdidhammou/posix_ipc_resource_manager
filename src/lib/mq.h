#ifndef MQ_H
#define MQ_H

#include <sys/msg.h>
#include <errno.h>

// message queues
int liberation_msgids[PROCESS_NUM - 1];
int response_msgid;

extern ResourceList resources;
extern Status process_statuses[PROCESS_NUM - 1];

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

    printf("M: Response : %s to %ld\n", resp.is_available ? "available" : "not available", resp.id + 1);
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
    printf("%ld: Response, %s\n", resp.id + 1, resp.is_available ? "available, continuing..." : "not available, blocked");
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
        printf("M: Liberation from %ld, {%d,%d,%d}\n", lib.id + 1, lib.resources.n_1, lib.resources.n_2, lib.resources.n_3);
    }

    return lib;
}

int check_liberation_queues(int next_lib_queue, int *queues_empty)
{
    Liberation lib;

    int i;
    for (i = next_lib_queue; i < PROCESS_NUM - 1; i++)
    {
        lib = get_liberation(liberation_msgids[i]);

        if (lib.id != -1)
        {
            (*queues_empty) = 0;

            resources.n_1 += lib.resources.n_1;
            resources.n_2 += lib.resources.n_2;
            resources.n_3 += lib.resources.n_3;

            printf("M: new resources : {%d,%d,%d}\n", resources.n_1, resources.n_2, resources.n_3);

            process_statuses[lib.id].resources.n_1 -= lib.resources.n_1;
            process_statuses[lib.id].resources.n_2 -= lib.resources.n_2;
            process_statuses[lib.id].resources.n_3 -= lib.resources.n_3;

            break;
        }
        else
        {
            (*queues_empty) == 1;
        }
    }

    int next_id = i == PROCESS_NUM - 1 ? 0 : (i + 1) % (PROCESS_NUM - 1);

    return next_id;
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