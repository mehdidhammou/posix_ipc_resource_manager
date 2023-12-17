#ifndef CONSTS_H
#define CONSTS_H

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

#endif