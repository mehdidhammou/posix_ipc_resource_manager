#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

#include "types.h"
#include "consts.h"
#include "mq.h"
#include "ipc.h"
#include "display.h"

// array for requests that were not satisfied
ResourceList process_requests[PROCESS_NUM - 1];

// array for process states
Status process_statuses[PROCESS_NUM - 1];

// resources
ResourceList resources;

void init_stats()
{
    resources = (ResourceList){10, 10, 10};

    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        process_requests[i] = (ResourceList){0, 0, 0};
        process_statuses[i] = (Status){.state = 0, .time_blocked = 0, .resources = {0, 0, 0}};
    }
}

void cleanup_stats()
{
    memset(process_requests, 0, sizeof(process_requests));
    memset(process_statuses, 0, sizeof(process_statuses));

    resources = (ResourceList){10, 10, 10};
}

void init()
{
    init_stats();
    init_mutexes();
    init_semaphores();
    init_shared_memory();
    init_message_queues();

    printf("Initialized.\n");
    printf("Available resources : %d, %d, %d\n", resources.n_1, resources.n_2, resources.n_3);
    printf("-----------------------------------------------------------------------\n");
}

void cleanup()
{
    cleanup_mqs();
    cleanup_stats();
    cleanup_semaphores();
    cleanup_shared_memory();
    printf("Cleaned up.\n");
}

bool resources_match(ResourceList req, ResourceList res)
{
    return req.n_1 == res.n_1 && req.n_2 == res.n_2 && req.n_3 == res.n_3;
}
    
char *get_file_path(int choice, int i)
{
    char file_name[10];

    sprintf(file_name, "%d.txt", i);
    int len = strlen(test_paths[choice - 1].path) + strlen(file_name) + 2;
    char *path = (char *)malloc(len * sizeof(char));

    strcpy(path, test_paths[choice - 1].path);

    strcat(path, "/");
    strcat(path, file_name);

    return path;
}

bool check_availability(Operation op, ResourceList res)
{
    ResourceList gathered = res;

    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        if (process_statuses[i].state != 1 || i == op.id)
            continue;

        int needed, available, taken;

        if (op.resources.n_1 - gathered.n_1 > 0)
        {
            needed = op.resources.n_1 - gathered.n_1;
            available = process_statuses[i].resources.n_1;
            taken = min(needed, available);
            gathered.n_1 += taken;
        }

        if (op.resources.n_2 - gathered.n_2 > 0)
        {
            needed = op.resources.n_2 - gathered.n_2;
            available = process_statuses[i].resources.n_2;
            taken = min(needed, available);
            gathered.n_2 += taken;
        }

        if (op.resources.n_3 - gathered.n_3 > 0)
        {
            needed = op.resources.n_3 - gathered.n_3;
            available = process_statuses[i].resources.n_3;
            taken = min(needed, available);
            gathered.n_3 += taken;
        }

        if (resources_match(op.resources, gathered))
            return true;
    }

    return false;
}

void gather_resources(Operation op, ResourceList res)
{
    ResourceList gathered = res;

    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        if (process_statuses[i].state != 1 || i == op.id)
            continue;

        if (op.resources.n_1 - gathered.n_1 > 0)
        {
            int needed = op.resources.n_1 - gathered.n_1;
            int available = process_statuses[i].resources.n_1;
            int taken = min(needed, available);

            gathered.n_1 += taken;
            process_statuses[i].resources.n_1 -= taken;
            process_requests[i].n_1 += taken;
        }

        if (op.resources.n_2 - gathered.n_2 > 0)
        {
            int needed = op.resources.n_2 - gathered.n_2;
            int available = process_statuses[i].resources.n_2;
            int taken = min(needed, available);

            gathered.n_2 += taken;
            process_statuses[i].resources.n_2 -= taken;
            process_requests[i].n_2 += taken;
        }

        if (op.resources.n_3 - gathered.n_3 > 0)
        {
            int needed = op.resources.n_3 - gathered.n_3;
            int available = process_statuses[i].resources.n_3;
            int taken = min(needed, available);

            gathered.n_3 += taken;
            process_statuses[i].resources.n_3 -= taken;
            process_requests[i].n_3 += taken;
        }

        if (resources_match(op.resources, gathered))
            break;
    }
}

void activate_process(Operation op)
{
    process_statuses[op.id].state = 0;
    process_statuses[op.id].time_blocked = 0;

    process_statuses[op.id].resources.n_1 += op.resources.n_1;
    process_statuses[op.id].resources.n_2 += op.resources.n_2;
    process_statuses[op.id].resources.n_3 += op.resources.n_3;

    process_requests[op.id] = (ResourceList){0, 0, 0};
}

void block_process(Operation op)
{
    printf("M: cannot satisfy %d blocked\n", op.id + 1);
    process_statuses[op.id].state = 1;
    process_statuses[op.id].time_blocked = time(NULL);

    process_requests[op.id] = op.resources;
}

void finish_process(Operation op)
{
    process_statuses[op.id].resources = (ResourceList){0, 0, 0};
}

Response satisfy(Operation op)
{
    Response resp = {.id = op.id, .is_available = 0};
    ResourceList res = {0, 0, 0};

    res.n_1 = min(resources.n_1, op.resources.n_1);
    res.n_2 = min(resources.n_2, op.resources.n_2);
    res.n_3 = min(resources.n_3, op.resources.n_3);

    if (resources_match(op.resources, res))
    {
        resp.is_available = 1;
    }
    else if (check_availability(op, res))
    {
        resp.is_available = 1;
        gather_resources(op, res);
    }

    return resp;
}

void satisfy_other()
{
    int max_priority_index = -1;
    time_t max_priority = -1;

    for (long i = 0; i < PROCESS_NUM - 1; i++)
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
        Operation op = {.id = max_priority_index, .type = 2, .resources = process_requests[max_priority_index]};
        Response resp = satisfy(op);

        if (resp.is_available)
        {
            printf("M: activated blocked process %d, {%d,%d,%d}\n", max_priority_index + 1, op.resources.n_1, op.resources.n_2, op.resources.n_3);
            resources.n_1 -= min(op.resources.n_1,resources.n_1);
            resources.n_2 -= min(op.resources.n_2,resources.n_2);
            resources.n_3 -= min(op.resources.n_3,resources.n_3);

            activate_process(op);
            send_response(resp);
        }
    }
}

void sigint_handler(int sig)
{
    cleanup();
    exit(0);
}

#endif