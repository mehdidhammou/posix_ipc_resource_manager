#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#include "types.h"
#include "consts.h"
#include "mq.h"
#include "ipc.h"

// array for requests that were not satisfied
ResourceList process_requests[PROCESS_NUM - 1];

// array for the status of the processes
Status process_statuses[PROCESS_NUM - 1];

// resources
ResourceList resources = {10, 10, 10};

void init_arrays()
{
    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        process_requests[i] = (ResourceList){0, 0, 0};
        process_statuses[i] = (Status){.state = 0, .time_blocked = 0, .resources = {0, 0, 0}};
    }
}

void init()
{
    init_arrays();
    init_semaphores();
    init_mutexes();
    init_message_queues();
    share_memory();
}

void cleanup_info()
{
    // Clear the arrays
    memset(process_requests, 0, sizeof(process_requests));
    memset(process_statuses, 0, sizeof(process_statuses));

    // Reset the resources
    resources.n_1 = 10;
    resources.n_2 = 10;
    resources.n_3 = 10;

    printf("Arrays and resources cleared\n");
}

void cleanup()
{
    cleanup_info();
    cleanup_mqs();
    cleanup_semaphores();
    cleanup_shared_memory();
}

bool is_request_satisfied(ResourceList req, ResourceList res)
{
    return req.n_1 == res.n_1 && req.n_2 == res.n_2 && req.n_3 == res.n_3;
}

void check_liberation_queues(int msgids[PROCESS_NUM - 1])
{
    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        Liberation lib = get_liberation(msgids[i]);

        // if the message is empty
        if (lib.id == -1)
            continue;

        printf("M: \t\t %ld liberates : %d, %d, %d\n", lib.id + 1, lib.resources.n_1, lib.resources.n_2, lib.resources.n_3);

        resources.n_1 += lib.resources.n_1;
        resources.n_2 += lib.resources.n_2;
        resources.n_3 += lib.resources.n_3;

        printf("M: \t\t resources : %d, %d, %d\n", resources.n_1, resources.n_2, resources.n_3);

        process_statuses[lib.id].resources.n_1 -= lib.resources.n_1;
        process_statuses[lib.id].resources.n_2 -= lib.resources.n_2;
        process_statuses[lib.id].resources.n_3 -= lib.resources.n_3;
    }
}

char *get_file_path(int choice, int i)
{
    const char *paths[] = {TEST_1, TEST_2, TEST_3, TEST_4, TEST_5};
    char file_name[10];

    sprintf(file_name, "%d.txt", i);

    char *path = malloc((strlen(paths[choice - 1]) + strlen(file_name) + 2) * sizeof(char));

    strcpy(path, paths[choice - 1]);

    strcat(path, "/");
    strcat(path, file_name);

    return path;
}

bool check_availability(Request req, ResourceList res)
{
    ResourceList gathered = res;

    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        if (process_statuses[i].state != 0)
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
        if (process_statuses[i].state != 0)
            continue;

        bool has_taken = false;

        if (req.resources.n_1 - gathered.n_1 > 0)
        {
            has_taken = true;

            int needed = req.resources.n_1 - gathered.n_1;
            int available = process_statuses[i].resources.n_1;
            int taken = min(needed, available);

            gathered.n_1 += taken;
            process_statuses[i].resources.n_1 -= taken;
            process_requests[i].n_1 += taken;
        }

        if (req.resources.n_2 - gathered.n_2 > 0)
        {
            has_taken = true;

            int needed = req.resources.n_2 - gathered.n_2;
            int available = process_statuses[i].resources.n_2;
            int taken = min(needed, available);

            gathered.n_2 += taken;
            process_statuses[i].resources.n_2 -= taken;
            process_requests[i].n_2 += taken;
        }

        if (req.resources.n_3 - gathered.n_3 > 0)
        {
            has_taken = true;

            int needed = req.resources.n_3 - gathered.n_3;
            int available = process_statuses[i].resources.n_3;
            int taken = min(needed, available);

            gathered.n_3 += taken;
            process_statuses[i].resources.n_3 -= taken;
            process_requests[i].n_3 += taken;
        }

        if (has_taken)

            if (is_request_satisfied(req.resources, gathered))
                break;
    }
}

Response get_resources(Request req)
{
    Response resp;
    resp.id = req.id;
    resp.is_available = false;

    if (
        req.resources.n_1 <= resources.n_1 &&
        req.resources.n_2 <= resources.n_2 &&
        req.resources.n_3 <= resources.n_3)
    {
        resp.is_available = true;

        resources.n_1 -= req.resources.n_1;
        resources.n_2 -= req.resources.n_2;
        resources.n_3 -= req.resources.n_3;
    }
    else
    {
        ResourceList res = {0, 0, 0};

        res.n_1 = min(resources.n_1, req.resources.n_1);
        res.n_2 = min(resources.n_2, req.resources.n_2);
        res.n_3 = min(resources.n_3, req.resources.n_3);

        if (check_availability(req, res))
        {
            gather_resources(req, res);

            resp.is_available = true;

            resources.n_1 -= res.n_1;
            resources.n_2 -= res.n_2;
            resources.n_3 -= res.n_3;
        }
    }

    return resp;
}

void activate_process(int i)
{
    process_statuses[i].state = 1;
    process_statuses[i].time_blocked = 0;
    process_requests[i] = (ResourceList){0, 0, 0};
}

void block_process(int i, Request req)
{
    process_statuses[i].state = 0;
    process_statuses[i].time_blocked = time(NULL);
    process_statuses[i].resources.n_1 += req.resources.n_1;
    process_statuses[i].resources.n_2 += req.resources.n_2;
    process_statuses[i].resources.n_3 += req.resources.n_3;
}

void sigint_handler(int sig)
{
    cleanup();
    exit(0);
}

#endif