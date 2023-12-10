#ifndef DISPLAY_H
#define DISPLAY_H

extern ResourceList process_requests[PROCESS_NUM - 1];
extern Status process_statuses[PROCESS_NUM - 1];
extern sem_t *mutex;

void display_process_requests()
{
    sem_wait(mutex);
    printf("Process requests : [\n");
    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        if (process_statuses[i].state != 1)
            continue;

        printf("{id : %d, resources : (%d, %d, %d)}\n", i + 1, process_requests[i].n_1, process_requests[i].n_2, process_requests[i].n_3);
    }
    sem_post(mutex);
}

void display_process_statuses()
{
    sem_wait(mutex);
    printf("Process states : [\n");
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
    sem_post(mutex);
}

void display_choices()
{
    for (int i = 0; i < sizeof(test_paths) / sizeof(test_paths[0]); i++)
        printf("%d. %s\n", i + 1, test_paths[i].name);

    printf("0. Exit\n");
}

#endif