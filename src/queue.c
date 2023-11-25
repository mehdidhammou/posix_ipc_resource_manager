#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
    int *data;
    int head;
    int tail;
    int size;
} Queue;

Queue *queue_create(int size)
{
    Queue *q = malloc(sizeof(Queue));
    q->data = malloc(size * sizeof(int));
    q->head = 0;
    q->tail = 0;
    q->size = size;
    return q;
}

void queue_destroy(Queue *q)
{
    free(q->data);
    free(q);
}

bool queue_is_empty(Queue *q)
{
    return q->head == q->tail;
}

bool queue_is_full(Queue *q)
{
    return (q->tail + 1) % q->size == q->head;
}

void queue_enqueue(Queue *q, int value)
{
    if (queue_is_full(q))
    {
        printf("Queue is full\n");
        return;
    }
    q->data[q->tail] = value;
    q->tail = (q->tail + 1) % q->size;
    q->size++;
}

int queue_dequeue(Queue *q)
{
    if (queue_is_empty(q))
    {
        printf("Queue is empty\n");
        return -1;
    }
    int value = q->data[q->head];
    q->head = (q->head + 1) % q->size;
    q->size--;
    return value;
}
