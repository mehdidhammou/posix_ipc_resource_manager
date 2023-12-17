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

bool is_request_satisfied(ResourceList req, ResourceList res)
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

bool check_availability(Request req, ResourceList res)
{
    ResourceList gathered = res;

    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        if (process_statuses[i].state != 1 || i == req.id)
            continue;

        int needed, available, taken;

        if (req.resources.n_1 - gathered.n_1 > 0)
        {
            needed = req.resources.n_1 - gathered.n_1;
            available = process_statuses[i].resources.n_1;
            taken = min(needed, available);
            gathered.n_1 += taken;
        }

        if (req.resources.n_2 - gathered.n_2 > 0)
        {
            needed = req.resources.n_2 - gathered.n_2;
            available = process_statuses[i].resources.n_2;
            taken = min(needed, available);
            gathered.n_2 += taken;
        }

        if (req.resources.n_3 - gathered.n_3 > 0)
        {
            needed = req.resources.n_3 - gathered.n_3;
            available = process_statuses[i].resources.n_3;
            taken = min(needed, available);
            gathered.n_3 += taken;
        }

        if (is_request_satisfied(req.resources, gathered))
            return true;
    }

    return false;
}

void gather_resources(Request req, ResourceList res)
{
    ResourceList gathered = res;

    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        if (process_statuses[i].state != 1 || i == req.id)
            continue;

        if (req.resources.n_1 - gathered.n_1 > 0)
        {
            int needed = req.resources.n_1 - gathered.n_1;
            int available = process_statuses[i].resources.n_1;
            int taken = min(needed, available);

            gathered.n_1 += taken;
            process_statuses[i].resources.n_1 -= taken;
            process_requests[i].n_1 += taken;
        }

        if (req.resources.n_2 - gathered.n_2 > 0)
        {
            int needed = req.resources.n_2 - gathered.n_2;
            int available = process_statuses[i].resources.n_2;
            int taken = min(needed, available);

            gathered.n_2 += taken;
            process_statuses[i].resources.n_2 -= taken;
            process_requests[i].n_2 += taken;
        }

        if (req.resources.n_3 - gathered.n_3 > 0)
        {
            int needed = req.resources.n_3 - gathered.n_3;
            int available = process_statuses[i].resources.n_3;
            int taken = min(needed, available);

            gathered.n_3 += taken;
            process_statuses[i].resources.n_3 -= taken;
            process_requests[i].n_3 += taken;
        }

        if (is_request_satisfied(req.resources, gathered))
            break;
    }
}

void activate_process(Request req)
{
    process_statuses[req.id].state = 0;
    process_statuses[req.id].time_blocked = 0;

    process_statuses[req.id].resources.n_1 += req.resources.n_1;
    process_statuses[req.id].resources.n_2 += req.resources.n_2;
    process_statuses[req.id].resources.n_3 += req.resources.n_3;

    process_requests[req.id] = (ResourceList){0, 0, 0};
}

void block_process(Request req)
{
    printf("M: cannot satisfy %d blocked\n", req.id + 1);
    process_statuses[req.id].state = 1;
    process_statuses[req.id].time_blocked = time(NULL);

    process_requests[req.id] = req.resources;
}

void finish_process(Request req)
{
    printf("%d finished\n", req.id + 1);

    process_statuses[req.id].resources = (ResourceList){0, 0, 0};
}

Response satisfy(Request req)
{
    Response resp = {.id = req.id, .is_available = 0};
    ResourceList res = {0, 0, 0};

    res.n_1 = min(resources.n_1, req.resources.n_1);
    res.n_2 = min(resources.n_2, req.resources.n_2);
    res.n_3 = min(resources.n_3, req.resources.n_3);

    if (is_request_satisfied(req.resources, res))
    {
        resp.is_available = 1;
    }
    else if (check_availability(req, res))
    {
        resp.is_available = 1;
        gather_resources(req, res);
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
        Request req = {max_priority_index, 2, process_requests[max_priority_index]};
        Response resp = satisfy(req);

        if (resp.is_available)
        {
            printf("M: activated blocked process %d\n", max_priority_index + 1);
            resources.n_1 -= min(req.resources.n_1,resources.n_1);
            resources.n_2 -= min(req.resources.n_2,resources.n_2);
            resources.n_3 -= min(req.resources.n_3,resources.n_3);

            activate_process(req);
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