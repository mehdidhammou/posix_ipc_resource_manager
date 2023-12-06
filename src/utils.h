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
#define RESPONSE_SIZE sizeof(Response) - sizeof(long)
#define LIBERATION_SIZE sizeof(Liberation) - sizeof(long)

// semaphore names
#define BUFFER_EMPTY_SEM "/buffer_empty"
#define BUFFER_FULL_SEM "/buffer_full"
#define MUTEX_BUFFER_SEM "/mutex_buffer"

// semaphores and mutexes
sem_t *mutex_buffer;
sem_t *buffer_empty;
sem_t *buffer_full;

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
    long id;
    ResourceList resources;
} Liberation;

typedef struct
{
    int state;
    ResourceList resources;
    int requistions_count;
    int time_blocked;
} Status;

// buffer for requests made by the processes
Request *buffer;

// indices
int *write_idx;
int *read_idx;

// array for requests that were not satisfied
ResourceList process_requests[PROCESS_NUM - 1];

// array for the status of the processes
Status process_statuses[PROCESS_NUM - 1];

// resources
ResourceList resources = {10, 10, 10};

// message queues
int liberations_msgid[PROCESS_NUM - 1];
int response_msgid;

void send_request(Request req)
{
    sem_wait(buffer_empty);
    sem_wait(mutex_buffer);

    // write the request to the buffer
    buffer[*write_idx] = req;

    // increment the write index and wrap around if needed
    (*write_idx) = ((*write_idx) + 1) % BUFFER_SIZE;

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
    req = buffer[*read_idx];

    // increment the read index and wrap around if needed
    (*read_idx) = ((*read_idx) + 1) % BUFFER_SIZE;

    sem_post(mutex_buffer);
    sem_post(buffer_empty);

    return req;
}

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

bool is_request_satisfied(ResourceList req, ResourceList res)
{
    return req.n_1 == res.n_1 && req.n_2 == res.n_2 && req.n_3 == res.n_3;
}

void send_liberation(Liberation lib, int liberations_msgid)
{
    // send the liberation to the manager
    if (msgsnd(liberations_msgid, &lib, LIBERATION_SIZE, 0) == -1)
    {
        printf("error from send_liberation, process %ld\n", lib.id);
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

void check_liberation_queues()
{
    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        Liberation lib = get_liberation(liberations_msgid[i]);

        // if the message is empty
        if (lib.id == -1)
            continue;

        printf("M: \t\t %ld liberates : %d, %d, %d\n", lib.id + 1, lib.resources.n_1, lib.resources.n_2, lib.resources.n_3);

        resources.n_1 += lib.resources.n_1;
        resources.n_2 += lib.resources.n_2;
        resources.n_3 += lib.resources.n_3;

        printf("M: \t\t resources : %d, %d, %d\n", resources.n_1, resources.n_2, resources.n_3);

        process_statuses[lib.id].resources.n_1 -= lib.resources.n_1;
        process_statuses[lib.id].resources.n_2 -= lib.resources.n_2;
        process_statuses[lib.id].resources.n_3 -= lib.resources.n_3;
    }
}

void init_arrays()
{
    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        process_requests[i] = (ResourceList){0, 0, 0};
        process_statuses[i] = (Status){1, (ResourceList){0, 0, 0}, 0, 0};
    }
}

void init_semaphores()
{
    // Initialize named semaphores
    buffer_empty = sem_open(BUFFER_EMPTY_SEM, O_CREAT, 0644, BUFFER_SIZE);
    buffer_full = sem_open(BUFFER_FULL_SEM, O_CREAT, 0644, 0);

    if (buffer_empty == SEM_FAILED || buffer_full == SEM_FAILED)
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

    // share memory for write_idx
    int write_idx_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644);

    if (write_idx_id == -1)
    {
        perror("shmget");
        exit(1);
    }

    write_idx = (int *)shmat(write_idx_id, NULL, 0);

    if (write_idx == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    *write_idx = 0;

    // share memory for read_idx
    int read_idx_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644);

    if (read_idx_id == -1)
    {
        perror("shmget");
        exit(1);
    }

    read_idx = (int *)shmat(read_idx_id, NULL, 0);

    if (read_idx == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    *read_idx = 0;

    printf("shared memory initialized\n");
}

void init()
{
    srand(time(NULL));
    init_semaphores();
    init_mutexes();
    init_arrays();
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
    sem_close(mutex_buffer);

    // Unlink named semaphores
    sem_unlink(BUFFER_EMPTY_SEM);
    sem_unlink(BUFFER_FULL_SEM);
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

    shmdt(write_idx);
    shmdt(read_idx);

    // Clear the arrays
    memset(process_requests, 0, sizeof(process_requests));
    memset(process_statuses, 0, sizeof(process_statuses));

    // Reset the resources
    resources.n_1 = 10;
    resources.n_2 = 10;
    resources.n_3 = 10;

    printf("Cleanup done.\n");
}

bool check_availability(Request req, ResourceList res)
{
    ResourceList gathered = res;

    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        if (process_statuses[i].state != 0)
            continue;

        if (req.resources.n_1 - gathered.n_1 > 0)
        {
            int needed = req.resources.n_1 - gathered.n_1;
            int available = process_statuses[i].resources.n_1;
            int taken = min(needed, available);
            gathered.n_1 += taken;
        }

        if (req.resources.n_2 - gathered.n_2 > 0)
        {
            int needed = req.resources.n_2 - gathered.n_2;
            int available = process_statuses[i].resources.n_2;
            int taken = min(needed, available);
            gathered.n_2 += taken;
        }

        if (req.resources.n_3 - gathered.n_3 > 0)
        {
            int needed = req.resources.n_3 - gathered.n_3;
            int available = process_statuses[i].resources.n_3;
            int taken = min(needed, available);
            gathered.n_3 += taken;
        }

        if (is_request_satisfied(req.resources, gathered))
            return true;
    }

    return false;
}

void gather_resources(Request req, ResourceList res)
{
    ResourceList gathered = res;

    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        if (process_statuses[i].state != 0)
            continue;

        bool has_taken = false;

        if (req.resources.n_1 - gathered.n_1 > 0)
        {
            has_taken = true;

            int needed = req.resources.n_1 - gathered.n_1;
            int available = process_statuses[i].resources.n_1;
            int taken = min(needed, available);

            gathered.n_1 += taken;
            process_statuses[i].resources.n_1 -= taken;
            process_requests[i].n_1 += taken;
        }

        if (req.resources.n_2 - gathered.n_2 > 0)
        {
            has_taken = true;

            int needed = req.resources.n_2 - gathered.n_2;
            int available = process_statuses[i].resources.n_2;
            int taken = min(needed, available);

            gathered.n_2 += taken;
            process_statuses[i].resources.n_2 -= taken;
            process_requests[i].n_2 += taken;
        }

        if (req.resources.n_3 - gathered.n_3 > 0)
        {
            has_taken = true;

            int needed = req.resources.n_3 - gathered.n_3;
            int available = process_statuses[i].resources.n_3;
            int taken = min(needed, available);

            gathered.n_3 += taken;
            process_statuses[i].resources.n_3 -= taken;
            process_requests[i].n_3 += taken;
        }

        if (has_taken)
            process_statuses[i].requistions_count++;

        if (is_request_satisfied(req.resources, gathered))
            break;
    }
}

Response get_resources(Request req)
{
    Response resp;
    resp.id = req.id;
    resp.is_available = false;

    if (req.resources.n_1 <= resources.n_1 && req.resources.n_2 <= resources.n_2 <= req.resources.n_3 <= resources.n_3)
    {
        resp.is_available = true;

        resources.n_1 -= req.resources.n_1;
        resources.n_2 -= req.resources.n_2;
        resources.n_3 -= req.resources.n_3;
    }
    else
    {
        ResourceList res = {0, 0, 0};

        res.n_1 = min(resources.n_1, req.resources.n_1);
        res.n_2 = min(resources.n_2, req.resources.n_2);
        res.n_3 = min(resources.n_3, req.resources.n_3);

        if (check_availability(req, res))
        {
            gather_resources(req, res);

            resp.is_available = true;

            resources.n_1 -= res.n_1;
            resources.n_2 -= res.n_2;
            resources.n_3 -= res.n_3;
        }
    }

    return resp;
}

void activate_process(int i)
{
    process_statuses[i].state = 1;
    process_statuses[i].time_blocked = 0;
    process_statuses[i].requistions_count = 0;
    process_requests[i] = (ResourceList){0, 0, 0};
}

void block_process(int i, Request req)
{
    process_statuses[i].state = 0;
    process_statuses[i].resources.n_1 += req.resources.n_1;
    process_statuses[i].resources.n_2 += req.resources.n_2;
    process_statuses[i].resources.n_3 += req.resources.n_3;
}

#endif