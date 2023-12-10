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
    init_mutexes();
    init_semaphores();
    init_shared_memory();
    init_message_queues();

    printf("Initialized.\n");
    printf("Available resources : %d, %d, %d\n", resources.n_1, resources.n_2, resources.n_3);
    printf("-----------------------------------------------------------------------\n");
}

void cleanup_info()
{
    memset(process_requests, 0, sizeof(process_requests));
    memset(process_statuses, 0, sizeof(process_statuses));

    resources.n_1 = 10;
    resources.n_2 = 10;
    resources.n_3 = 10;
}


void cleanup()
{
    cleanup_info();
    cleanup_mqs();
    cleanup_semaphores();
    cleanup_shared_memory();
    printf("Cleaned up.\n");
}


bool is_request_satisfied(ResourceList req, ResourceList res)
{
    return req.n_1 == res.n_1 && req.n_2 == res.n_2 && req.n_3 == res.n_3;
}

void check_liberation_queues()
{
    for (int i = 0; i < PROCESS_NUM - 1; i++)
    {
        Liberation lib = get_liberation(liberation_msgids[i]);

        // if the message is empty
        if (lib.id == -1)
            continue;

        resources.n_1 += lib.resources.n_1;
        resources.n_2 += lib.resources.n_2;
        resources.n_3 += lib.resources.n_3;

        printf("Liberation: %ld, {%d, %d, %d}\n", lib.id + 1, lib.resources.n_1, lib.resources.n_2, lib.resources.n_3);
        printf("Available resources : %d, %d, %d\n", resources.n_1, resources.n_2, resources.n_3);

        process_statuses[lib.id].resources.n_1 -= lib.resources.n_1;
        process_statuses[lib.id].resources.n_2 -= lib.resources.n_2;
        process_statuses[lib.id].resources.n_3 -= lib.resources.n_3;
    }
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
        if (process_statuses[i].state != 1)
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
        if (process_statuses[i].state != 1)
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

        if (has_taken && is_request_satisfied(req.resources, gathered))
            break;
    }
}

Response get_resources(Request req)
{
    Response resp = {.id = req.id, .is_available = false};
    ResourceList res = {0, 0, 0};

    res.n_1 = min(resources.n_1, req.resources.n_1);
    res.n_2 = min(resources.n_2, req.resources.n_2);
    res.n_3 = min(resources.n_3, req.resources.n_3);

    if (is_request_satisfied(req.resources, res))
    {
        resp.is_available = true;
    }
    else if (check_availability(req, res))
    {
        resp.is_available = true;
        gather_resources(req, res);
    }

    if (resp.is_available)
    {
        resources.n_1 -= res.n_1;
        resources.n_2 -= res.n_2;
        resources.n_3 -= res.n_3;
    }

    printf("%d, %d %s : {%d, %d, %d}\n", req.id + 1, req.type, (resp.is_available ? "Granted" : "Blocked"), req.resources.n_1, req.resources.n_2, req.resources.n_3);

    return resp;
}

void activate_process(Request req)
{
    process_statuses[req.id].state = 0;
    process_statuses[req.id].time_blocked = 0;
    process_statuses[req.id].resources.n_1 += req.resources.n_1;
    process_statuses[req.id].resources.n_2 += req.resources.n_2;
    process_statuses[req.id].resources.n_3 += req.resources.n_3;

    display_process_statuses();
}

void block_process(Request req)
{
    process_statuses[req.id].state = 1;
    process_statuses[req.id].time_blocked = time(NULL);
    process_requests[req.id].n_1 += req.resources.n_1;
    process_requests[req.id].n_2 += req.resources.n_2;
    process_requests[req.id].n_3 += req.resources.n_3;

    display_process_requests();
}

void finish_process(Request req)
{
    process_statuses[req.id].state = -1;

    resources.n_1 += process_statuses[req.id].resources.n_1;
    resources.n_2 += process_statuses[req.id].resources.n_2;
    resources.n_3 += process_statuses[req.id].resources.n_3;
}

void sigint_handler(int sig)
{
    cleanup();
    exit(0);
}

#endif