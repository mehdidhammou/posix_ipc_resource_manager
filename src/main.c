#include <sys/wait.h>
#include "lib/utils.h"

void process(int choice, long int i)
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
            resp = get_response(i, response_msgid);
            if (!resp.is_available)
                resp = get_response(i, response_msgid);
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

char data[6][20][20] = {
    {"1,0,0,0",
     "1,0,0,0",
     "2,4,4,5",
     "1,0,0,0",
     "1,0,0,0",
     "3,4,4,5",
     "1,0,0,0",
     "2,3,2,0",
     "1,0,0,0",
     "3,3,2,0",
     "4,0,0,0",
     "END"},
    {"1,0,0,0",
     "1,0,0,0",
     "2,1,0,1",
     "1,0,0,0",
     "1,0,0,0",
     "3,1,0,1",
     "1,0,0,0",
     "2,4,4,10",
     "1,0,0,0",
     "3,4,4,10",
     "4,0,0,0",
     "END"},
    {"1,0,0,0",
     "1,0,0,0",
     "2,10,10,10",
     "1,0,0,0",
     "1,0,0,0",
     "3,10,10,10",
     "1,0,0,0",
     "2,4,4,0",
     "1,0,0,0",
     "3,4,4,0",
     "4,0,0,0",
     "END"},
    {"1,0,0,0",
     "1,0,0,0",
     "2,10,10,10",
     "1,0,0,0",
     "1,0,0,0",
     "3,10,10,10",
     "1,0,0,0",
     "2,4,4,0",
     "1,0,0,0",
     "3,4,4,0",
     "4,0,0,0",
     "END"},
    {"1,0,0,0",
     "1,0,0,0",
     "2,10,10,10",
     "1,0,0,0",
     "1,0,0,0",
     "3,10,10,10",
     "1,0,0,0",
     "2,4,4,0",
     "1,0,0,0",
     "3,4,4,0",
     "4,0,0,0",
     "END"}};

void process_sim(int choice, long int i)
{
    Request req;
    Response resp;
    Instruction inst;
    Liberation lib;

    int lastLine = 0;

    while (strcmp(data[choice - 1][lastLine], "END") != 0)
    {
        sleep(WAIT_TIME);
        sscanf(data[choice - 1][lastLine], "%d,%d,%d,%d", &inst.type, &inst.resources.n_1, &inst.resources.n_2, &inst.resources.n_3);
        switch (inst.type)
        {
        case 2:
            req = (Request){i, inst.type, inst.resources};
            send_request(req);
            resp = get_response(i, response_msgid);
            if (!resp.is_available)
                resp = get_response(i, response_msgid);
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
        lastLine++;
    }
}

void manager()
{
    int last_checked_lib = -1;
    int active_processes = PROCESS_NUM - 1;
    while (active_processes > 0)
    {
        sleep(WAIT_TIME);

        Request req = get_request();

        if (req.type == 4)
        {
            finish_process(req);
            active_processes--;
            continue;
        }

        if (req.type == 2)
        {
            Response resp = get_resources(req);
            (resp.is_available) ? activate_process(req) : block_process(req);
            send_response(resp, response_msgid);
            continue;
        }

        if (req.type == -1)
        {
            last_checked_lib = check_liberation_queues(last_checked_lib);

            int max_priority_index = -1;
            time_t max_priority = -1;

            for (long int i = 0; i < PROCESS_NUM - 1; i++)
            {
                if (process_statuses[i].state != 1)
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
                    activate_process(req);
                    send_response(resp, response_msgid);
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

        for (long int i = 0; i < PROCESS_NUM; i++)
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