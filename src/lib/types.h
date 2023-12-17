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
    int type;
    ResourceList resources;
} Instruction;

typedef struct
{
    int id;
    int type;
    ResourceList resources;
} Request;

typedef struct
{
    long id;
    int is_available;
} Response;

typedef struct
{
    long id;
    ResourceList resources;
} Liberation;

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

#endif