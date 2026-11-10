#include "queue.h"

#include <stdint.h>
#include <stdlib.h>

#define QUEUE_DEFAULT_INITIAL_CAPACITY 4

Queue *queue_create(size_t initial_capacity) {
    size_t cap = initial_capacity == 0 ? QUEUE_DEFAULT_INITIAL_CAPACITY : initial_capacity;
    if (cap > SIZE_MAX / sizeof(int)) {
        return NULL;
    }

    Queue *queue = (Queue *)malloc(sizeof(Queue));
    if (!queue) {
        return NULL;
    }

    queue->data = (int *)malloc(cap * sizeof(int));
    if (!queue->data) {
        free(queue);
        return NULL;
    }

    queue->front = 0;
    queue->size = 0;
    queue->capacity = cap;
    return queue;
}

void queue_destroy(Queue *queue) {
    if (!queue) {
        return;
    }
    free(queue->data);
    free(queue);
}

bool queue_enqueue(Queue *queue, int value) {
    if (!queue) {
        return false;
    }

    if (queue->size == queue->capacity) {
        if (queue->capacity > SIZE_MAX / 2) {
            return false; /* Overflow safeguard */
        }
        size_t new_cap = queue->capacity * 2;
        if (new_cap > SIZE_MAX / sizeof(int)) {
            return false;
        }

        int *new_data = (int *)malloc(new_cap * sizeof(int));
        if (!new_data) {
            return false;
        }

        /* Unroll circular buffer elements in order into new buffer */
        for (size_t i = 0; i < queue->size; i++) {
            new_data[i] = queue->data[(queue->front + i) % queue->capacity];
        }

        free(queue->data);
        queue->data = new_data;
        queue->front = 0;
        queue->capacity = new_cap;
    }

    size_t rear = (queue->front + queue->size) % queue->capacity;
    queue->data[rear] = value;
    queue->size++;
    return true;
}

bool queue_dequeue(Queue *queue, int *out_value) {
    if (!queue || queue->size == 0) {
        return false;
    }

    if (out_value) {
        *out_value = queue->data[queue->front];
    }

    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;
    return true;
}

bool queue_peek(const Queue *queue, int *out_value) {
    if (!queue || !out_value || queue->size == 0) {
        return false;
    }
    *out_value = queue->data[queue->front];
    return true;
}

bool queue_is_empty(const Queue *queue) {
    return (!queue || queue->size == 0);
}

size_t queue_size(const Queue *queue) {
    return queue ? queue->size : 0;
}

size_t queue_capacity(const Queue *queue) {
    return queue ? queue->capacity : 0;
}
