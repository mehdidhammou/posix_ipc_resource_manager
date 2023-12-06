#ifndef IPC_H
#define IPC_H

#include <fcntl.h>
#include <semaphore.h>
#include <sys/shm.h>

// semaphores and mutexes
sem_t *mutex_buffer;
sem_t *buffer_empty;
sem_t *buffer_full;

// buffer for requests made by the processes
Request *buffer;

// indices
int *write_idx;
int *read_idx;

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
    Request req = {.id = -1, .type = -1, .resources = {0, 0, 0}};

    if (sem_trywait(buffer_full) == 0)
    {
        sem_wait(mutex_buffer);

        // read the request from the buffer
        req = buffer[*read_idx];

        // increment the read index and wrap around if needed
        (*read_idx) = ((*read_idx) + 1) % BUFFER_SIZE;

        sem_post(mutex_buffer);
        sem_post(buffer_empty);
    }

    return req;
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

void cleanup_semaphores()
{
    sem_close(buffer_empty);
    sem_close(buffer_full);
    sem_close(mutex_buffer);

    sem_unlink(BUFFER_EMPTY_SEM);
    sem_unlink(BUFFER_FULL_SEM);
    sem_unlink(MUTEX_BUFFER_SEM);

    printf("Semaphores cleared");
}

void cleanup_shared_memory()
{
    shmdt(buffer);
    shmdt(write_idx);
    shmdt(read_idx);

    printf("Shared memory cleared");
}

#endif