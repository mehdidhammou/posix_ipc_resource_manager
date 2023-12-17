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
            printf("%d: request, {%d,%d,%d}\n", req.id + 1, req.resources.n_1, req.resources.n_2, req.resources.n_3);
            resp = get_response(i);
            if (!resp.is_available)
                resp = get_response(i);
            break;

        case 3:
            lib = (Liberation){i, inst.resources};
            printf("%ld: Liberation, {%d,%d,%d}\n", lib.id + 1, lib.resources.n_1, lib.resources.n_2, lib.resources.n_3);
            send_liberation(lib, liberation_msgids[i]);
            break;

        case 4:
            printf("%ld: Finish\n", i + 1);
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
    int queues_empty = 1;

    while (active_processes > 0 || queues_empty)
    {
        printf("M: active processes : %d, resources : {%d,%d,%d}\n", active_processes, resources.n_1, resources.n_2, resources.n_3);
        sleep(WAIT_TIME);

        Request req = get_request();

        switch (req.type)
        {
        case 2:
            printf("M:_______________________________Request from %d, {%d,%d,%d}\n", req.id + 1, req.resources.n_1, req.resources.n_2, req.resources.n_3);
            Response resp = satisfy(req);

            if (resp.is_available)
            {
                printf("M: %d's request satisfied\n", req.id + 1);
                resources.n_1 -= min(req.resources.n_1, resources.n_1);
                resources.n_2 -= min(req.resources.n_2, resources.n_2);
                resources.n_3 -= min(req.resources.n_3, resources.n_3);

                activate_process(req);
                send_response(resp);
            }
            else
            {
                block_process(req);
            }
            break;

        case 3:
            printf("M: _______________________________Queue Check\n");
            printf("M: checking queue : %d\n", next_lib_queue + 1);
            next_lib_queue = check_liberation_queues(next_lib_queue, &queues_empty);
            printf("M: next queue : %d\n", next_lib_queue + 1);
            satisfy_other();
            break;

        case 4:
            printf("M: _______________________________ %d Finished\n", req.id + 1);
            finish_process(req);
            active_processes--;
            satisfy_other();
            break;
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