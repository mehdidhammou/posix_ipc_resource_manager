#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include "utils.h"
#include <errno.h>
#include <math.h>
#include <signal.h>

void sigint_handler(int sig)
{
    cleanup();
    exit(0);
}

void blocked_time_handler(int sig)
{
    int active_processes = 0;
    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        if (process_statuses[i].state == 0)
        {
            process_statuses[i].time_blocked++;
        }
    }
    alarm(WAIT_TIME);
}

void process(int choice, int i)
{

    char *file_name = get_file_path(choice, i);
    FILE *f = fopen(file_name, "r");

    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    Request req;
    Response resp;
    Instruction inst;
    Liberation lib;

    while (fscanf(f, "%d,%d,%d,%d", &inst.type, &inst.resources.n_1, &inst.resources.n_2, &inst.resources.n_3) != EOF)
    {
        switch (inst.type)
        {
        case 1:
            sleep(WAIT_TIME);
            break;

        case 2:

            req = (Request){i, 2, inst.resources};
            send_request(req);
            do
            {
                resp = get_response(i, response_msgid);
            } while (!resp.is_available);

            break;

        case 3:
            printf("%d: \t\t\t liberation : %d, %d, %d\n", i + 1, inst.resources.n_1, inst.resources.n_2, inst.resources.n_3);
            lib = (Liberation){i, inst.resources};
            send_liberation(lib, liberations_msgid[i]);
            break;

        case 4:
            printf("\t%d: \t finish\n", i + 1);
            req = (Request){i, 4, inst.resources};
            send_request(req);
            break;
        }
    }

    fclose(f);
    free(file_name);
    exit(0);
}

void manager()
{
    sleep(WAIT_TIME);
    int active_processes = PROCESS_NUM - 1;
    while (active_processes > 0)
    {
        check_liberation_queues();

        int checked_procs[PROCESS_NUM - 1] = {0};
        for (int i = 0; i < PROCESS_NUM - 1; i++)
        {
            int max_priority_index = -1;
            int max_priority = -1;

            for (int j = 0; j < PROCESS_NUM - 1; j++)
            {
                if (process_statuses[j].state != 0)
                    continue;

                int priority = process_statuses[j].time_blocked + 5 * process_statuses[j].requistions_count;
                if (priority > max_priority && checked_procs[j] == 0)
                {
                    max_priority = priority;
                    max_priority_index = j;
                }
            }

            checked_procs[max_priority_index] = 1;

            Request req = {max_priority_index, 2, process_statuses[max_priority_index].resources};
            Response resp = get_resources(req);

            if (resp.is_available && max_priority_index != -1)
            {
                send_response(resp, response_msgid);
                activate_process(max_priority_index);
            }
        }

        Request req = get_request();

        // if the process is finished
        if (req.type == 4)
        {
            printf("M: \t\t %d finished\n", req.id + 1);

            process_statuses[req.id].state = -1;

            resources.n_1 += process_statuses[req.id].resources.n_1;
            resources.n_2 += process_statuses[req.id].resources.n_2;
            resources.n_3 += process_statuses[req.id].resources.n_3;

            active_processes--;
        }

        if (req.type == 2)
        {

            // check liberation queues for available resources
            printf("M: \t\t %d requests : %d, %d, %d\n", req.id + 1, req.resources.n_1, req.resources.n_2, req.resources.n_3);
            Response resp = get_resources(req);

            if (!resp.is_available)
            {
                block_process(req.id, req);

                printf("M: \t\t %d !S: %d, %d, %d\n", req.id + 1, req.resources.n_1, req.resources.n_2, req.resources.n_3);
            }
            else
            {
                activate_process(req.id);

                printf("M: \t\t %d S: %d, %d, %d\n", req.id + 1, req.resources.n_1, req.resources.n_2, req.resources.n_3);
            }

            send_response(resp, response_msgid);
        }
    }
    printf("\tM: \t finished\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigint_handler);
    signal(SIGALRM, blocked_time_handler);
    int choice;
    do
    {
        system("clear");
        printf("Choose a test:\n");
        printf("1. No resource request\n");
        printf("2. One resource request with one liberation\n");
        printf("3. One resource request with mulitple liberations\n");
        printf("4. mulitple resource requests with one liberation\n");
        printf("5. mulitple resource requests with mulitple liberations\n");
        printf("0. Exit\n");

        scanf("%d", &choice);

        if (choice == 0)
            break;

        if (choice < 0 || choice > 5)
        {
            printf("Invalid choice\n");
            continue;
        }

        init();

        alarm(WAIT_TIME);

        pid_t pids[PROCESS_NUM];

        printf("Available resources : %d, %d, %d\n", resources.n_1, resources.n_2, resources.n_3);

        for (int i = 0; i < PROCESS_NUM; i++)
        {
            pids[i] = fork();

            if (pids[i] == -1)
            {
                perror("fork");
                exit(1);
            }

            else if (pids[i] == 0)
            {
                signal(SIGALRM, SIG_IGN);
                signal(SIGINT, SIG_DFL);

                if (i == PROCESS_NUM - 1)
                {
                    manager();
                }
                else
                {
                    process(choice, i);
                }
            }
        }

        for (int i = 0; i < PROCESS_NUM; i++)
        {
            waitpid(pids[i], NULL, 0);
        }

        cleanup();

        printf("Done.\n\n");

    } while (choice != 0);
    return 0;
}