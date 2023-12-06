#include <sys/wait.h>
#include "lib/utils.h"

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
            resp = get_response(i, response_msgid);
            if (!resp.is_available)
                resp = get_response(i, response_msgid);

            break;

        case 3:
            printf("%d: \t\t\t liberation : %d, %d, %d\n", i + 1, inst.resources.n_1, inst.resources.n_2, inst.resources.n_3);
            lib = (Liberation){i, inst.resources};
            send_liberation(lib, liberation_msgids[i]);
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
    int active_processes = PROCESS_NUM - 1;
    while (active_processes > 0)
    {

        Request req = get_request();

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

        check_liberation_queues(liberation_msgids);

        if (req.type == -1)
        {
            int max_priority_index = -1;
            time_t max_priority = -1;

            for (int i = 0; i < PROCESS_NUM - 1; i++)
            {
                if (process_statuses[i].state != 0)
                    continue;

                time_t priority = time(NULL) - process_statuses[i].time_blocked;

                if (priority > max_priority)
                {
                    max_priority = priority;
                    max_priority_index = i;
                }
            }

            if (max_priority_index != -1)
            {
                Request req = {max_priority_index, 2, process_statuses[max_priority_index].resources};
                Response resp = get_resources(req);

                if (resp.is_available)
                {
                    send_response(resp, response_msgid);
                    activate_process(max_priority_index);
                }
            }
        }
    }

    printf("\tM: \t finished\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigint_handler);

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
                signal(SIGINT, SIG_DFL);
                (i == PROCESS_NUM - 1) ? manager() : process(choice, i);
            }
        }

        for (int i = 0; i < PROCESS_NUM; i++)
            waitpid(pids[i], NULL, 0);

        cleanup();

        printf("Done.\n\n");

    } while (choice != 0);

    return 0;
}