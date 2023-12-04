#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))


// test locations
#define TEST_1 "./tests/no_dem"
#define TEST_2 "./tests/one_dem_one_lib"
#define TEST_3 "./tests/one_dem_multi_lib"
#define TEST_4 "./tests/multi_dem_one_lib"
#define TEST_5 "./tests/multi_dem_multi_lib"

// constants
#define PROCESS_NUM 6
#define BUFFER_SIZE 3
#define WAIT_TIME 1
#define RESPONSE_SIZE sizeof(Response)
#define LIBERATION_SIZE sizeof(Liberation)

// semaphore names
#define BUFFER_EMPTY_SEM "/buffer_empty"
#define BUFFER_FULL_SEM "/buffer_full"
#define RESPONSE_EMPTY_SEM "/respone_empty"
#define MUTEX_BUFFER_SEM "/mutex_buffer"

// semaphores and mutexes
sem_t *mutex_buffer;
sem_t *buffer_empty;
sem_t *buffer_full;
sem_t *respone_empty;

// structs
typedef struct
{
    int n_1;
    int n_2;
    int n_3;
} ResourceList;

typedef struct
{
    int type;
    ResourceList resources;
} Instruction;

typedef struct
{
    int id;
    int type;
    ResourceList resources;
} Request;

typedef struct
{
    long id;
    bool is_available;
} Response;

typedef struct
{
    int id;
    ResourceList resources;
} Liberation;

typedef struct
{
    bool is_active;
    ResourceList resources;
    int requistions_count;
    int time_blocked;
} Status;

// buffer for requests made by the processes
Request *buffer;

// indices
int *next_request_index;

// array for requests that were not satisfied
ResourceList process_requests[PROCESS_NUM - 1];

// array for responses
Response responses[PROCESS_NUM - 1];

// array for the status of the processes
Status process_statuses[PROCESS_NUM - 1];

// resources
ResourceList resources = {10, 10, 10};

// message queues
int liberations_msgid[PROCESS_NUM - 1];

// message queue id of the response queue
int response_msgid;

void send_request(Request req)
{
    // !still not adhering to FIFO

    sem_wait(buffer_empty);
    sem_wait(mutex_buffer);

    // add to the buffer
    buffer[*next_request_index] = req;

    // increment the index
    (*next_request_index)++;

    sem_post(mutex_buffer);
    sem_post(buffer_full);
}

Request get_request()
{
    // !still not adhering to FIFO
    Request req;
    sem_wait(buffer_full);
    sem_wait(mutex_buffer);

    // read the request from the buffer
    (*next_request_index)--;
    req = buffer[*next_request_index];

    // clear the buffer slot
    memset(&buffer[*next_request_index], 0, sizeof(Request));

    sem_post(mutex_buffer);
    sem_post(buffer_empty);

    return req;
}

void send_response(Response resp, int response_msgid)
{
    // send the response to the process
    if (msgsnd(response_msgid, &resp, RESPONSE_SIZE, 0) == -1)
    {
        printf("Problem in sending response\n");
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

bool is_request_satisfied(ResourceList req, ResourceList res)
{
    return req.n_1 == res.n_1 && req.n_2 == res.n_2 && req.n_3 == res.n_3;
}

void send_liberation(Liberation lib, int liberations_msgid)
{
    // send the liberation to the manager
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

    return lib;
}

void init_semaphores()
{
    // Initialize named semaphores
    buffer_empty = sem_open(BUFFER_EMPTY_SEM, O_CREAT, 0644, BUFFER_SIZE);
    buffer_full = sem_open(BUFFER_FULL_SEM, O_CREAT, 0644, 0);
    respone_empty = sem_open(RESPONSE_EMPTY_SEM, O_CREAT, 0644, 1);

    if (buffer_empty == SEM_FAILED || buffer_full == SEM_FAILED || respone_empty == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    printf("semaphores initialized\n");
}

void init_mutexes()
{
    // Initialize named mutexes, we use sem_open with initial value 1 to simulate mutex
    mutex_buffer = sem_open(MUTEX_BUFFER_SEM, O_CREAT, 0644, 1);

    printf("mutexes initialized\n");
}

void init_message_queues()
{
    // create 6 message queues , 5 for the resource liberation and 1 for the response
    for (int i = 0; i < PROCESS_NUM; i++)
    {
        key_t key = ftok("main.c", i);
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
            liberations_msgid[i] = msgget(key, 0644 | IPC_CREAT);
            if (liberations_msgid[i] == -1)
            {
                perror("msgget liberations");
                exit(1);
            }
        }
    }

    printf("message queues initialized\n");
}

void share_memory()
{
    // share memory for the buffer
    int buffer_id = shmget(IPC_PRIVATE, sizeof(Request) * BUFFER_SIZE, IPC_CREAT | 0644);

    if (buffer_id == -1)
    {
        perror("shmget");
        exit(1);
    }

    buffer = (Request *)shmat(buffer_id, NULL, 0);

    if (buffer == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    // share memory for next_request_index
    int next_request_index_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644);

    if (next_request_index_id == -1)
    {
        perror("shmget");
        exit(1);
    }

    next_request_index = (int *)shmat(next_request_index_id, NULL, 0);

    if (next_request_index == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    *next_request_index = 0;

    printf("shared memory initialized\n");
}

void init()
{
    srand(time(NULL));
    init_semaphores();
    init_mutexes();
    init_message_queues();
    share_memory();
}

char *get_file_path(int choice, int i)
{
    char file_name[10];
    sprintf(file_name, "%d.txt", i);
    char *path = (char *)malloc(100 * sizeof(char));

    switch (choice)
    {
    case 1:
        strcpy(path, TEST_1);
        break;

    case 2:
        strcpy(path, TEST_2);
        break;

    case 3:
        strcpy(path, TEST_3);
        break;

    case 4:
        strcpy(path, TEST_4);
        break;

    case 5:
        strcpy(path, TEST_5);
        break;
    }

    // append /i.txt to the path
    strcat(path, "/");
    strcat(path, file_name);

    return path;
}

void cleanup()
{
    // Close all named semaphores
    sem_close(buffer_empty);
    sem_close(buffer_full);
    sem_close(respone_empty);
    sem_close(mutex_buffer);

    // Unlink named semaphores
    sem_unlink(BUFFER_EMPTY_SEM);
    sem_unlink(BUFFER_FULL_SEM);
    sem_unlink(RESPONSE_EMPTY_SEM);
    sem_unlink(MUTEX_BUFFER_SEM);

    // cleanup message queues
    for (int i = 0; i < PROCESS_NUM; i++)
    {
        if (i == PROCESS_NUM - 1)
        {
            msgctl(response_msgid, IPC_RMID, NULL);
        }
        else
        {
            msgctl(liberations_msgid[i], IPC_RMID, NULL);
        }
    }

    // Clear the buffer memory
    shmdt(buffer);

    // Clear the arrays
    memset(process_requests, 0, sizeof(process_requests));
    memset(responses, 0, sizeof(responses));
    memset(process_statuses, 0, sizeof(process_statuses));

    // Reset the resources
    resources.n_1 = 10;
    resources.n_2 = 10;
    resources.n_3 = 10;

    // Clear the indices
    shmdt(next_request_index);

    printf("Cleanup done.\n");
}

#endif