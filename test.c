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
    long int id;
    bool is_available;
} Response;

typedef struct
{
    long int id;
    ResourceList resources; // Assuming ResourceList as an int array
} Liberation;

void display_response(Response response)
{
    printf("Response: ID = %ld, Availability = %s\n", response.id, response.is_available ? "true" : "false");
}

void display_liberation(Liberation liberation)
{
    printf("Liberation: ID = %ld, Resources = [%d, %d, %d]\n",
           liberation.id, liberation.resources.n_1, liberation.resources.n_2, liberation.resources.n_3);
}

int main()
{
    int response_msgid; // Assuming 5 Liberation queues

    // Generate keys and get message queue IDs for Liberation and Response queues
    key_t key = ftok("./src/main.c", 5);
    response_msgid = msgget(key, 0644 | IPC_CREAT);
    if (response_msgid == -1)
    {
        perror("msgget response");
        exit(1);
    }

    // Continuously monitor Response and Liberation queues
    while (1)
    {
        // Monitor Response queue
        Liberation lib;
        if (msgrcv(response_msgid, &lib, sizeof(Liberation), 1, IPC_NOWAIT) != -1)
        {
            display_liberation(lib);
            if (msgsnd(response_msgid, &lib, sizeof(Liberation) - sizeof(long), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }
        }

        usleep(200000); // Sleep for 0.5 seconds
    }

    return 0;
}
