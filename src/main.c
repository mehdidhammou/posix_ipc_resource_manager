#include <sys/wait.h>
#include "lib/utils.h"

void process(int choice, long i)
{
    char *file_name = get_file_path(choice, i);
    FILE *f = fopen(file_name, "r");

    if (f == NULL)
    {
        perror("fopen");
        exit(1);
    }

    Request req;
    Response resp;
    Instruction inst;
    Liberation lib;

    while (fscanf(f, "%d,%d,%d,%d", &inst.type, &inst.resources.n_1, &inst.resources.n_2, &inst.resources.n_3) != EOF)
    {
        sleep(WAIT_TIME);
        switch (inst.type)
        {
        case 2:
            req = (Request){i, inst.type, inst.resources};
            send_request(req);
            resp = get_response(i);
            if (!resp.is_available)
                resp = get_response(i);
            break;

        case 3:
            lib = (Liberation){i, inst.resources};
            send_liberation(lib, liberation_msgids[i]);
            break;

        case 4:
            req = (Request){i, inst.type, inst.resources};
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
    int next_lib_queue = 0;
    int active_processes = PROCESS_NUM - 1;
    int empty_queues = 0;
    int last_attempted_activation = -1;

    while (active_processes > 0 || empty_queues != PROCESS_NUM - 1)
    {
        display_stats();
        sleep(WAIT_TIME);

        Request req = get_request();

        if (req.type == 4)
        {
            finish_process(req);
            active_processes--;
            display_stats();
            continue;
        }

        if (req.type == 2)
        {
            Response resp = get_resources(req);
            (resp.is_available) ? activate_process(req) : block_process(req);
            send_response(resp);
            display_stats();
            continue;
        }

        if (req.type == -1)
        {
            next_lib_queue = check_liberation_queues(next_lib_queue, &empty_queues);

            int max_priority_index = -1;
            time_t max_priority = -1;

            for (long i = 0; i < PROCESS_NUM - 1; i++)
            {
                if (process_statuses[i].state != 1)
                    continue;

                time_t priority = time(NULL) - process_statuses[i].time_blocked;

                if (priority > max_priority && i != last_attempted_activation)
                {
                    max_priority = priority;
                    max_priority_index = i;
                }
            }

            if (max_priority_index != -1)
            {
                Request req = {max_priority_index, 2, process_requests[max_priority_index]};
                Response resp = get_resources(req);
                if (resp.is_available)
                {
                    last_attempted_activation = -1;
                    display_resources();
                    sleep(WAIT_TIME);
                    activate_process(req);
                    send_response(resp);
                    display_stats();
                }
                else
                {
                    last_attempted_activation = max_priority_index;
                }
            }
        }
    }

    printf("M: finished\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigint_handler);

    int choice;
    do
    {
        display_choices();

        scanf("%d", &choice);

        if (choice == 0)
            break;

        if (choice < 0 || choice > 5)
        {
            printf("Invalid choice\n");
            continue;
        }

        printf("path: %s.\n", test_paths[choice - 1].path);

        init();

        pid_t pids[PROCESS_NUM];

        system("clear");

        for (long i = 0; i < PROCESS_NUM; i++)
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
    } while (choice != 0);

    return 0;
}