#ifndef DISPLAY_H
#define DISPLAY_H

extern sem_t *buffer_mutex;
extern ResourceList resources;
extern Status process_statuses[PROCESS_NUM - 1];
extern ResourceList process_requests[PROCESS_NUM - 1];

void display_process_requests()
{
    printf("Process requests \n\n");
    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        printf("{id : %d, resources : (%d, %d, %d)}\n", i + 1, process_requests[i].n_1, process_requests[i].n_2, process_requests[i].n_3);
    }
}

void display_process_statuses()
{
    printf("Process statuses \n\n");
    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        printf("{id : %d, ", i + 1);
        switch (process_statuses[i].state)
        {
        case -1:
            printf("Finished}\n");
            continue;

        case 0:
            printf("Active, ");
            break;

        case 1:
            printf("Blocked, time blocked : %ld, ", process_statuses[i].time_blocked);
            break;
        }

        printf("resources : (%d, %d, %d)}\n", process_statuses[i].resources.n_1, process_statuses[i].resources.n_2, process_statuses[i].resources.n_3);
    }
}

void display_buffer()
{
    sem_wait(buffer_mutex);
    printf("Buffer\n\n");
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        printf("{id : %d, type: %s, resources : (%d, %d, %d)}\n", buffer[i].id + 1, (buffer[i].type == 2 ? "Request" : buffer[i].type == 4 ? "Finish"
                                                                                                                                           : ""),
               buffer[i].resources.n_1, buffer[i].resources.n_2, buffer[i].resources.n_3);
    }
    sem_post(buffer_mutex);
}

void display_resources()
{
    printf("Available resources : %d, %d, %d\n", resources.n_1, resources.n_2, resources.n_3);
}

void display_stats()
{
    system("clear");
    printf("-----------------------------------------------------------------------\n");
    display_buffer();
    printf("-----------------------------------------------------------------------\n");
    display_resources();
    printf("-----------------------------------------------------------------------\n");
    display_process_requests();
    printf("-----------------------------------------------------------------------\n");
    display_process_statuses();
    printf("-----------------------------------------------------------------------\n");
}

void display_choices()
{
    for (int i = 0; i < sizeof(test_paths) / sizeof(test_paths[0]); i++)
        printf("%d. %s\n", i + 1, test_paths[i].name);

    printf("0. Exit\n");
}

#endif