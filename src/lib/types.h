#ifndef TYPES_H
#define TYPES_H

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


#endif