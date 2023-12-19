#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <errno.h>

// -----------------------------------------------------------------types

typedef struct
{
    long mtype;
    char mtext[100];
} Message;

typedef struct
{
    int n_1;
    int n_2;
    int n_3;
} ResourceList;

typedef struct
{
    int id;
    int type;
    ResourceList resources;
} Operation;

typedef struct
{
    int id;
    int is_available;
} Response;

typedef struct
{
    int state;
    time_t time_blocked;
    ResourceList resources;
} Status;

typedef struct
{
    const char *name;
    const char *path;
} Test;

typedef struct
{
    Operation *buffer;
    int *write_idx;
    int *read_idx;
    int *counter;
} SharedMemory;

// -----------------------------------------------------------------consts

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// test paths
const Test test_paths[] = {
    {.name = "No resource request",
     .path = "./tests/no_dem"},
    {.name = "One resource request with one liberation",
     .path = "./tests/one_dem_one_lib"},
    {.name = "One resource request with mulitple liberations",
     .path = "./tests/one_dem_multi_lib"},
    {.name = "mulitple resource requests with one liberation",
     .path = "./tests/multi_dem_one_lib"},
    {.name = "mulitple resource requests with mulitple liberations",
     .path = "./tests/multi_dem_multi_lib"}};

// constants
#define PROCESS_NUM 6
#define BUFFER_SIZE 3
#define WAIT_TIME 2
#define MSG_SIZE sizeof(Message) - sizeof(long)

// semaphore names
#define BUFFER_EMPTY_SEM "/buffer_empty"

// mutex names
#define COUNTER_MUTEX "/counter"
#define BUFFER_MUTEX "/mutex"

// semaphores and mutexes
int buffer_empty, buffer_mutex, counter_mutex;

// buffer for requests made by the processes
Operation *buffer;

// counter for number of requests in buffer
int *counter;

// indices
int *write_idx;
int *read_idx;

// shared memory ids
int buffer_id, write_idx_id, read_idx_id, counter_id;

// message queues
int liberation_msgids[PROCESS_NUM - 1];
int response_msgid;

// array for requests that were not satisfied
ResourceList process_requests[PROCESS_NUM - 1];

// array for process states
Status process_statuses[PROCESS_NUM - 1];

// resources
ResourceList resources;

// ----------------------------------------------------------------- ipc


void sem_p(int sem_id)
{
    struct sembuf p_op = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
    if (semop(sem_id, &p_op, 1) == -1)
    {
        perror("semop");
        exit(1);
    }
}

void sem_v(int sem_id)
{
    struct sembuf v_op = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
    if (semop(sem_id, &v_op, 1) == -1)
    {
        perror("semop");
        exit(1);
    }
}

void send_request(Operation op)
{
    sem_p(buffer_empty);
    sem_p(buffer_mutex);

    buffer[*write_idx] = op;

    (*write_idx) = ((*write_idx) + 1) % BUFFER_SIZE;

    sem_p(counter_mutex);
    (*counter)++;
    sem_v(counter_mutex);

    sem_v(buffer_mutex);
}

Operation get_request()
{
    // req.type = 3 means no request
    Operation op = {.type = 3};

    sem_p(counter_mutex);
    if (*counter == 0)
    {
        sem_v(counter_mutex);
        return op;
    }
    sem_v(counter_mutex);
    sem_p(buffer_mutex);

    op = buffer[*read_idx];

    (*read_idx) = ((*read_idx) + 1) % BUFFER_SIZE;

    sem_p(counter_mutex);
    (*counter)--;
    sem_v(counter_mutex);

    sem_v(buffer_mutex);
    sem_v(buffer_empty);

    return op;
}

void init_semaphores()
{
    buffer_empty = semget(IPC_PRIVATE, 1, IPC_CREAT | 0644);
    if (buffer_empty == -1)
    {
        perror("semget");
        exit(1);
    }

    if (semctl(buffer_empty, 0, SETVAL, BUFFER_SIZE) == -1)
    {
        perror("semctl");
        exit(1);
    }
}

void init_mutexes()
{
    // Initialize named mutexes, we use sem_open with initial value 1 to simulate mutex
    buffer_mutex = semget(IPC_PRIVATE, 1, IPC_CREAT | 0644);
    if (buffer_mutex == -1)
    {
        perror("semget");
        exit(1);
    }

    if (semctl(buffer_mutex, 0, SETVAL, 1) == -1)
    {
        perror("semctl");
        exit(1);
    }

    counter_mutex = semget(IPC_PRIVATE, 1, IPC_CREAT | 0644);
    if (counter_mutex == -1)
    {
        perror("semget");
        exit(1);
    }

    if (semctl(counter_mutex, 0, SETVAL, 1) == -1)
    {
        perror("semctl");
        exit(1);
    }
}

void init_shared_memory()
{
    buffer_id = shmget(IPC_PRIVATE, sizeof(Operation) * BUFFER_SIZE, IPC_CREAT | 0644);
    if (buffer_id == -1)
    {
        perror("shmget");
        exit(1);
    }
    buffer = (Operation *)shmat(buffer_id, NULL, 0);
    if (buffer == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    write_idx_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644);
    if (write_idx_id == -1)
    {
        perror("shmget");
        exit(1);
    }
    write_idx = (int *)shmat(write_idx_id, NULL, 0);
    if (write_idx == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }
    *write_idx = 0;
    
    read_idx_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644);
    if (read_idx_id == -1)
    {
        perror("shmget");
        exit(1);
    }
    read_idx = (int *)shmat(read_idx_id, NULL, 0);
    if (read_idx == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }
    *read_idx = 0;

    counter_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644);
    if (counter_id == -1)
    {
        perror("shmget");
        exit(1);
    }
    counter = (int *)shmat(counter_id, NULL, 0);
    if (counter == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }
    *counter = 0;
}

void cleanup_semaphores()
{
    if (semctl(buffer_empty, 0, IPC_RMID, 0) == -1)
    {
        perror("semctl");
        exit(1);
    }

    if (semctl(buffer_mutex, 0, IPC_RMID, 0) == -1)
    {
        perror("semctl");
        exit(1);
    }

    if (semctl(counter_mutex, 0, IPC_RMID, 0) == -1)
    {
        perror("semctl");
        exit(1);
    }
}

void cleanup_shared_memory()
{
    shmdt(buffer);
    shmdt(write_idx);
    shmdt(read_idx);
    shmdt(counter);

    shmctl(buffer_id, IPC_RMID, NULL);
    shmctl(write_idx_id, IPC_RMID, NULL);
    shmctl(read_idx_id, IPC_RMID, NULL);
    shmctl(counter_id, IPC_RMID, NULL);
}

// ----------------------------------------------------------------- mq


void init_message_queues()
{
    // create 6 message queues , 5 for the resource liberation and 1 for the response
    for (int i = 0; i < PROCESS_NUM; i++)
    {
        key_t key = ftok("./src/main.c", i);
        if (key == -1)
        {
            perror("ftok");
            exit(1);
        }

        if (i == PROCESS_NUM - 1)
        {
            response_msgid = msgget(key, 0644 | IPC_CREAT);
            if (response_msgid == -1)
            {
                perror("msgget response");
                exit(1);
            }
        }
        else
        {
            liberation_msgids[i] = msgget(key, 0644 | IPC_CREAT);
            if (liberation_msgids[i] == -1)
            {
                perror("msgget liberations");
                exit(1);
            }
        }
    }
}

void send_response(Response resp)
{
    Message msg = {.mtype = resp.id + 1};
    sprintf(msg.mtext, "%d", resp.is_available);

    if (msgsnd(response_msgid, &msg, MSG_SIZE, 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }

    printf("M: Response : %s to %d\n", resp.is_available ? "available" : "not available", resp.id + 1);
}

Response get_response(int i)
{
    Message msg;
    Response resp = {.id = i};

    if (msgrcv(response_msgid, &msg, MSG_SIZE, i + 1, 0) == -1)
    {
        perror("msgrcv");
        exit(1);
    }

    sscanf(msg.mtext, "%d", &resp.is_available);
    printf("%d: Response, %s\n", resp.id + 1, resp.is_available ? "available, continuing..." : "not available, blocked");
    return resp;
}

void send_liberation(Operation op, int liberations_msgid)
{
    Message msg = {.mtype = op.id + 1};
    sprintf(msg.mtext, "%d,%d,%d", op.resources.n_1, op.resources.n_2, op.resources.n_3);

    if (msgsnd(liberations_msgid, &msg, MSG_SIZE, 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }
}

Operation get_liberation(int liberations_msgid)
{
    Operation op = {.id = -1, .type = 3};
    Message msg;
    if (msgrcv(liberations_msgid, &msg, MSG_SIZE, 0, IPC_NOWAIT) == -1)
    {
        if (errno != ENOMSG)
        {
            perror("msgrcv");
            exit(1);
        }
    }
    else
    {
        op.id = msg.mtype - 1;
        sscanf(msg.mtext, "%d,%d,%d", &op.resources.n_1, &op.resources.n_2, &op.resources.n_3);
        printf("M: Liberation from %d, {%d,%d,%d}\n", op.id + 1, op.resources.n_1, op.resources.n_2, op.resources.n_3);
    }

    return op;
}

int check_liberation_queues(int next_lib_queue)
{
    Operation op;

    int i;
    for (i = next_lib_queue; i < PROCESS_NUM - 1; i++)
    {
        op = get_liberation(liberation_msgids[i]);

        if (op.id != -1)
        {
            resources.n_1 += op.resources.n_1;
            resources.n_2 += op.resources.n_2;
            resources.n_3 += op.resources.n_3;

            printf("M: new resources : {%d,%d,%d}\n", resources.n_1, resources.n_2, resources.n_3);

            process_statuses[op.id].resources.n_1 -= op.resources.n_1;
            process_statuses[op.id].resources.n_2 -= op.resources.n_2;
            process_statuses[op.id].resources.n_3 -= op.resources.n_3;

            break;
        }
    }

    int next_id = i == (PROCESS_NUM - 1) ? 0 : (i + 1) % (PROCESS_NUM - 1);

    return next_id;
}

void cleanup_mqs()
{
    // cleanup message queues
    for (int i = 0; i < PROCESS_NUM; i++)
    {
        if (i == PROCESS_NUM - 1)
        {
            msgctl(response_msgid, IPC_RMID, NULL);
        }
        else
        {
            msgctl(liberation_msgids[i], IPC_RMID, NULL);
        }
    }
}

// ----------------------------------------------------------------- utils

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

// ----------------------------------------------------------------- main

void display_choices()
{
    for (int i = 0; i < sizeof(test_paths) / sizeof(test_paths[0]); i++)
        printf("%d. %s\n", i + 1, test_paths[i].name);

    printf("0. Exit\n");
}


void process(int choice, int i)
{
    char *file_name = get_file_path(choice, i);
    FILE *f = fopen(file_name, "r");

    if (f == NULL)
    {
        perror("fopen");
        exit(1);
    }

    Operation inst = {.id = i};
    Response resp;

    while (fscanf(f, "%d,%d,%d,%d", &inst.type, &inst.resources.n_1, &inst.resources.n_2, &inst.resources.n_3) != EOF)
    {
        sleep(WAIT_TIME);
        switch (inst.type)
        {
        case 2:
            send_request(inst);
            printf("%d: request, {%d,%d,%d}\n", inst.id + 1, inst.resources.n_1, inst.resources.n_2, inst.resources.n_3);
            resp = get_response(i);
            if (!resp.is_available)
                resp = get_response(i);
            break;

        case 3:
            printf("%d: Liberation, {%d,%d,%d}\n", inst.id + 1, inst.resources.n_1, inst.resources.n_2, inst.resources.n_3);
            send_liberation(inst, liberation_msgids[i]);
            break;

        case 4:
            printf("%d: Finish\n", i + 1);
            send_request(inst);
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
    ResourceList initial_resources = resources;

    while (active_processes > 0 || !resources_match(resources, initial_resources))
    {
        printf("M: active processes : %d, resources : {%d,%d,%d}\n", active_processes, resources.n_1, resources.n_2, resources.n_3);
        sleep(WAIT_TIME);

        Operation op = get_request();

        switch (op.type)
        {
        case 2:
            printf("M:_______________________________Request from %d, {%d,%d,%d}\n", op.id + 1, op.resources.n_1, op.resources.n_2, op.resources.n_3);
            Response resp = satisfy(op);

            if (resp.is_available)
            {
                printf("M: %d's request satisfied\n", op.id + 1);
                resources.n_1 -= min(op.resources.n_1, resources.n_1);
                resources.n_2 -= min(op.resources.n_2, resources.n_2);
                resources.n_3 -= min(op.resources.n_3, resources.n_3);

                activate_process(op);
                send_response(resp);
            }
            else
            {
                block_process(op);
            }
            break;

        case 3:
            printf("M: _______________________________Queue Check\n");
            printf("M: checking queue : %d\n", next_lib_queue + 1);
            next_lib_queue = check_liberation_queues(next_lib_queue);
            printf("M: next queue : %d\n", next_lib_queue + 1);
            satisfy_other();
            break;

        case 4:
            printf("M: _______________________________ %d Finished\n", op.id + 1);
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
    } while (choice != 0);

    return 0;
}