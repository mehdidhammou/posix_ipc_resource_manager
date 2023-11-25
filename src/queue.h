// create a queue of integers

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

typedef struct queue Queue;

Queue *queue_create(int size);
void queue_destroy(Queue *q);
void queue_enqueue(Queue *q, int value);
int queue_dequeue(Queue *q);
bool queue_is_empty(Queue *q);
bool queue_is_full(Queue *q);

#endif
