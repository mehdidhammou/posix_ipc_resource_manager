#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <signal.h>

#define n 10

typedef struct
{
    int id;
    int type;
} Test;

Test T[10];



int main(){

    for(int i = 0; i < n; i++){
        printf("%d, %d\n", T[i].id, T[i].type);
    }
}