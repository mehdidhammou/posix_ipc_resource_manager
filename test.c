#include <signal.h>
#include <sys/wait.h>
// #include "src/lib/utils.h"
#include <stdbool.h>
#include <stdio.h>
typedef struct
{
    int n_1;
    int n_2;
    int n_3;
} ResourceList;

void process()
{
    while (true)
    {
    }
}

void manager()
{
    while (true)
    {
    }
}

int main()
{
    printf("size of long %ld, size of long %ld\n", sizeof(ResourceList), sizeof(long));
    return 0;
}
