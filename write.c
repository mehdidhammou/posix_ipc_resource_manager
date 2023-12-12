#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define PROCESS_NUM 6
#define MAX_RESOURCES 3 // Example resource list size

typedef struct
{
    int n_1;
    int n_2;
    int n_3;
} ResourceList;

typedef struct
{
    long id;
    bool is_available;
} Response;

typedef struct
{
    long id;
    ResourceList resources; // Assuming ResourceList as an int array
} Liberation;

int main()
{
    int response_msgid; // Assuming 5 Liberation queues

    // Generate keys and get message queue IDs for Liberation and Response queues
    key_t key = ftok("/home/mahdi/Desktop/tp-algo-2/.gitignore", 5);
    response_msgid = msgget(key, 0644 | IPC_CREAT);
    if (response_msgid == -1)
    {
        perror("msgget response");
        exit(1);
    }

    // Send a Liberation message
    while (1)
    {
        long id = rand() % (PROCESS_NUM - 1) + 1;
        Liberation lib = {id, {rand() % 10, rand() % 10, rand() % 10}};
        printf("Sending Liberation message..., id %ld\n", lib.id);

        if (msgsnd(response_msgid, &lib, sizeof(Liberation) - sizeof(long), 0) == -1)
        {
            perror("msgsnd");
            exit(1);
        }
        sleep(1);
    }
}