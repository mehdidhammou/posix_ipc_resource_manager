#ifndef IPC_H
#define IPC_H

#include <fcntl.h>
#include <sys/shm.h>
#include <semaphore.h>

// semaphores and mutexes
sem_t *mutex;
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
    sem_wait(mutex);

    printf("-----------------------------------------------------------------------\n");
    printf("[Buffer]\n");

    buffer[*write_idx] = req;

    printf("New message : ");
    printf("[ id: %d, type: %d, resources: (%d, %d, %d) ]\n", req.id + 1, req.type, req.resources.n_1, req.resources.n_2, req.resources.n_3);

    (*write_idx) = ((*write_idx) + 1) % BUFFER_SIZE;

    printf("-----------------------------------------------------------------------\n");

    sem_post(mutex);
    sem_post(buffer_full);
}

Request get_request()
{
    Request req = {.id = -1, .type = -1, .resources = {0, 0, 0}};

    if (sem_trywait(buffer_full) == 0)
    {
        sem_wait(mutex);

        printf("-----------------------------------------------------------------------\n");
        printf("[Buffer]\n");

        req = buffer[*read_idx];

        printf("Read {id: %d, type: %d}\n", req.id + 1, req.type);

        (*read_idx) = ((*read_idx) + 1) % BUFFER_SIZE;

        printf("-----------------------------------------------------------------------\n");

        sem_post(mutex);
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
}

void init_mutexes()
{
    // Initialize named mutexes, we use sem_open with initial value 1 to simulate mutex
    mutex = sem_open(MUTEX_SEM, O_CREAT, 0644, 1);
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
}

void cleanup_semaphores()
{
    sem_close(buffer_empty);
    sem_close(buffer_full);
    sem_close(mutex);

    sem_unlink(BUFFER_EMPTY_SEM);
    sem_unlink(BUFFER_FULL_SEM);
    sem_unlink(MUTEX_SEM);
}

void cleanup_shared_memory()
{
    shmdt(buffer);
    shmdt(write_idx);
    shmdt(read_idx);
}

#endif