#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#define THREAD_NUM 6
#define BUFFER_SIZE 3

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
    ResourceList resources;
} Request;

typedef struct
{
    int id;
    ResourceList resources;
} Response;

typedef struct
{
    bool is_active;
    ResourceList resources;
    int requistions_count;
    int time_blocked;
} Status;

// arrays

// buffer for requests made by the processes
Request buffer[BUFFER_SIZE];

// array for requests that were not satisfied
ResourceList requests[THREAD_NUM - 1];

// array for responses
Response responses[THREAD_NUM - 1];

// array for the status of the processes
Status process_statuses[THREAD_NUM - 1];

// resources
ResourceList resources = {10, 5, 7};

// semaphores and mutexes
pthread_mutex_t mutexBuffer;
sem_t *buffer_empty;
sem_t *buffer_full;
sem_t *respone_empty;

// indices
int next_request_index = 0;

void init_semaphores()
{
    // Initialize named semaphores
    buffer_empty = sem_open("/buffer_empty", O_CREAT, 0644, BUFFER_SIZE);
    buffer_full = sem_open("/buffer_full", O_CREAT, 0644, 0);
    respone_empty = sem_open("/respone_empty", O_CREAT, 0644, 1);

    if (buffer_empty == SEM_FAILED || buffer_full == SEM_FAILED || respone_empty == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
}

void create_request(Instruction inst, int i)
{
    Request req;

    req.id = i;
    req.resources = inst.resources;

    sem_wait(&buffer_empty);
    pthread_mutex_lock(&mutexBuffer);

    // add to the buffer
    buffer[next_request_index] = req;

    // increment the index
    next_request_index++;

    pthread_mutex_unlock(&mutexBuffer);
    sem_post(&buffer_full);
}

void *process(int i)
{
    char file_name[20];

    sprintf(file_name, "Fiche_%d.txt", i);

    FILE *f = fopen(file_name, "r");

    if (f == NULL)
    {
        perror(strcat("Error opening file :", file_name));
        exit(1);
    }

    Instruction inst;
    while (fscanf(f, "%d,%d,%d,%d", &inst.type, &inst.resources.n_1, &inst.resources.n_2, &inst.resources.n_3) != EOF)
    {
        switch (inst.type)
        {
        case 1:
            sleep(1);
            break;

        case 2:
            create_request(inst, i);

            // sem_wait

            break;
        }
    }

    // Add to the buffer

    fclose(f);
    exit(0);
}

bool is_request_satisfied(ResourceList req, ResourceList res)
{
    return req.n_1 == res.n_1 && req.n_2 == res.n_2 && req.n_3 == res.n_3;
}

void *manager()
{
    while (1)
    {
        // Remove from the buffer
        sem_wait(&buffer_full);
        pthread_mutex_lock(&mutexBuffer);
        // read the request from the buffer
        next_request_index--;
        Request req = buffer[next_request_index];

        // clear the buffer
        memset(&buffer[next_request_index], 0, sizeof(Request));

        pthread_mutex_unlock(&mutexBuffer);
        sem_post(&buffer_empty);

        // handle request
        sem_wait(&respone_empty);
        // if the resources are available
        bool is_available = false;
        if (req.resources.n_1 <= resources.n_1 && req.resources.n_2 <= resources.n_2 && req.resources.n_3 <= resources.n_3)
        {
            is_available = true;
            // decrement the resources
            resources.n_1 -= req.resources.n_1;
            resources.n_2 -= req.resources.n_2;
            resources.n_3 -= req.resources.n_3;

            // generate a response in FReponse
        }
        else
        {
            ResourceList available_resources = {0, 0, 0};
            // take the resources that are availble
            available_resources.n_1 = resources.n_1;
            resources.n_1 = 0;
            available_resources.n_2 = resources.n_2;
            resources.n_2 = 0;
            available_resources.n_3 = resources.n_3;
            resources.n_3 = 0;

            // take the resources that are needed from th blocked process and add them to the available resources
            for (int i = 0; i < THREAD_NUM - 1; i++)
            {
                if (!process_statuses[i].is_active)
                {
                    if (available_resources.n_1 < req.resources.n_1 && requests[i].n_1 > 0)
                    {
                        available_resources.n_1 += requests[i].n_1;
                        requests[i].n_1 += process_statuses[i].resources.n_1;
                        process_statuses[i].resources.n_1 = 0;
                    }
                    else if (available_resources.n_2 < requests[i].n_2)
                    {
                        available_resources.n_1 -= requests[i].n_1;
                        requests[i].n_1 = 0;
                    }
                    else if (available_resources.n_3 < requests[i].n_3)
                    {
                        available_resources.n_2 -= requests[i].n_2;
                        requests[i].n_2 = 0;
                    }
                    else
                    {
                        available_resources.n_3 -= requests[i].n_3;
                        requests[i].n_3 = 0;
                    }
                }
            }
        }

        if (is_available)
        {
            // generate a positive response
        }
        else
        {
            // add the request to the requests array
            requests[req.id] = req.resources;
            // increment the time_blocked
            process_statuses[req.id].time_blocked++;
        }
    }

    // sem_post

    // sem_wait

    exit(0);
}

void init_mutexes()
{
    pthread_mutex_init(&mutexBuffer, NULL);
}

void init_message_queues()
{
    // initialize the process statuses array
    for (int i = 0; i < THREAD_NUM - 1; i++)
    {
        process_statuses[i].is_active = true;
        process_statuses[i].requistions_count = 0;
        process_statuses[i].time_blocked = 0;
    }

    // initialize the requests array
    for (int i = 0; i < THREAD_NUM - 1; i++)
    {
        requests[i].n_1 = 0;
        requests[i].n_2 = 0;
        requests[i].n_3 = 0;
    }
}

void cleanup()
{
    pthread_mutex_destroy(&mutexBuffer);
    // Close named semaphores
    sem_close(buffer_empty);
    sem_close(buffer_full);
    sem_close(respone_empty);

    // Unlink named semaphores
    sem_unlink("/buffer_empty");
    sem_unlink("/buffer_full");
    sem_unlink("/respone_empty");
}

void init()
{
    init_semaphores();

    init_mutexes();

    init_message_queues();
}

int main(int argc, char *argv[])
{
    
    init();

    for (int i = 0; i < THREAD_NUM; i++)
    {
        if (fork() == 0)
        {
            if (i == THREAD_NUM - 1)
            {
                manager();
            }
            else
            {
                process(i);
            }
        }
    }

    for (int i = 0; i < THREAD_NUM; i++)
    {
        wait(NULL);
    }

    cleanup();

    return 0;
}