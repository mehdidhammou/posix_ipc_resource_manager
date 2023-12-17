#ifndef IPC_H
#define IPC_H

#include <fcntl.h>
#include <sys/shm.h>
#include <semaphore.h>

// semaphores
sem_t *buffer_empty;

// mutexes
sem_t *buffer_mutex;
sem_t *counter_mutex;

// buffer for requests made by the processes
Request *buffer;

// counter for number of requests in buffer
int *counter;

// indices
int *write_idx;
int *read_idx;

void send_request(Request req)
{
    sem_wait(buffer_empty);
    sem_wait(buffer_mutex);


    buffer[*write_idx] = req;

    (*write_idx) = ((*write_idx) + 1) % BUFFER_SIZE;

    sem_wait(counter_mutex);
    (*counter)++;
    sem_post(counter_mutex);

    sem_post(buffer_mutex);
}

Request get_request()
{
    // req.type = 3 means no request
    Request req = {.id = -1, .type = 3, .resources = {0, 0, 0}};

    sem_wait(counter_mutex);
    if (*counter == 0)
    {
        sem_post(counter_mutex);
        return req;
    }
    sem_post(counter_mutex);
    sem_wait(buffer_mutex);

    req = buffer[*read_idx];

    (*read_idx) = ((*read_idx) + 1) % BUFFER_SIZE;

    sem_wait(counter_mutex);
    (*counter)--;
    sem_post(counter_mutex);

    sem_post(buffer_mutex);
    sem_post(buffer_empty);

    return req;
}

void init_semaphores()
{
    // Initialize named semaphores
    buffer_empty = sem_open(BUFFER_EMPTY_SEM, O_CREAT, 0644, BUFFER_SIZE);
    if (buffer_empty == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
}

void init_mutexes()
{
    // Initialize named mutexes, we use sem_open with initial value 1 to simulate mutex
    buffer_mutex = sem_open(BUFFER_MUTEX, O_CREAT, 0644, 1);
    counter_mutex = sem_open(COUNTER_MUTEX, O_CREAT, 0644, 1);

    if (buffer_mutex == SEM_FAILED || counter_mutex == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
}

void init_shared_memory()
{
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

    int counter_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644);
    if (counter_id == -1)
    {
        perror("shmget");
        exit(1);
    }

    counter = (int *)shmat(counter_id, NULL, 0);
    if (counter == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    *counter = 0;
}

void cleanup_semaphores()
{
    sem_close(buffer_empty);
    sem_close(buffer_mutex);
    sem_close(counter_mutex);

    sem_unlink(BUFFER_EMPTY_SEM);
    sem_unlink(COUNTER_MUTEX);
    sem_unlink(BUFFER_MUTEX);
}

void cleanup_shared_memory()
{
    shmdt(buffer);
    shmdt(write_idx);
    shmdt(read_idx);
    shmdt(counter);
}

#endif