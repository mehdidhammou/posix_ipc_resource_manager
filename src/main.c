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

void process(FILE *f, int i)
{

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
            printf("%d: \t\t\t normal\n", i + 1);
            break;

        case 2:

            printf("%d: \t\t\t demand : %d, %d, %d\n", i + 1, inst.resources.n_1, inst.resources.n_2, inst.resources.n_3);
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
            printf("%d: \t\t\t finish\n", i + 1);
            req = (Request){i, 4, inst.resources};
            send_request(req);
            break;
        }
    }

    fclose(f);
    exit(0);
}

void manager()
{
    while (1)
    {
        sleep(WAIT_TIME);
        // liberate resources
        for (int i = 0; i < PROCESS_NUM - 1; i++)
        {
            Liberation lib = get_liberation(liberations_msgid[i]);

            // if the message is empty
            if (lib.id == -1)
            {
                continue;
            }

            printf("M: \t\t %d liberates : %d, %d, %d\n", lib.id, lib.resources.n_1, lib.resources.n_2, lib.resources.n_3);

            resources.n_1 += lib.resources.n_1;
            resources.n_2 += lib.resources.n_2;
            resources.n_3 += lib.resources.n_3;

            printf("M: \t\t resources : %d, %d, %d\n", resources.n_1, resources.n_2, resources.n_3);

            process_statuses[lib.id].resources.n_1 -= lib.resources.n_1;
            process_statuses[lib.id].resources.n_2 -= lib.resources.n_2;
            process_statuses[lib.id].resources.n_3 -= lib.resources.n_3;
        }

        Request req = get_request();

        // if the process is finished
        if (req.type == 4)
        {
            process_statuses[req.id].is_active = false;

            bool still_active = false;

            for (int i = 0; i < PROCESS_NUM - 1; i++)
            {
                if (process_statuses[i].is_active)
                {
                    still_active = true;
                }
            }

            if (!still_active)
            {
                break;
            }
        }

        if (req.type == 2)
        {
            // !this is yours ahmed.
        }
    }

    printf("M: \t\t finished\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    int choice;
    do
    {

        // clear the terminal
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

        printf("Available resources : %d, %d, %d\n", resources.n_1, resources.n_2, resources.n_3);

        pid_t pids[PROCESS_NUM];

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
                if (i == PROCESS_NUM - 1)
                {
                    manager();
                }
                else
                {
                    char *file_name = get_file_path(choice, i);
                    FILE *f = fopen(file_name, "r");

                    if (f == NULL)
                    {
                        printf("Error opening file!\n");
                        exit(1);
                    }

                    process(f, i);

                    free(file_name);
                }
            }
        }

        for (int i = 0; i < PROCESS_NUM; i++)
        {
            waitpid(pids[i], NULL, 0);
            printf("process %d finished\n", i + 1);
        }

        cleanup();

        printf("Done.\n\n");

    } while (choice != 0);
    return 0;
}