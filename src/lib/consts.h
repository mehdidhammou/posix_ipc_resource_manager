#ifndef CONSTS_H
#define CONSTS_H

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// test locations
#define TEST_1 "./tests/no_dem"
#define TEST_2 "./tests/one_dem_one_lib"
#define TEST_3 "./tests/one_dem_multi_lib"
#define TEST_4 "./tests/multi_dem_one_lib"
#define TEST_5 "./tests/multi_dem_multi_lib"

// constants
#define PROCESS_NUM 6
#define BUFFER_SIZE 3
#define WAIT_TIME 1
#define RESPONSE_SIZE sizeof(Response)
#define LIBERATION_SIZE sizeof(Liberation)

// semaphore names
#define BUFFER_EMPTY_SEM "/buffer_empty"
#define BUFFER_FULL_SEM "/buffer_full"
#define MUTEX_BUFFER_SEM "/mutex_buffer"

#endif