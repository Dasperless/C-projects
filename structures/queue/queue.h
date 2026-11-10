#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Queue Queue;

struct Queue {
    int *data;
    size_t front;
    size_t size;
    size_t capacity;
};

/**
 * @brief Creates a FIFO circular ring buffer queue with the given initial capacity.
 *
 * @param initial_capacity Initial number of elements capacity (if 0, default 4 is used).
 * @return Pointer to new Queue, or NULL on memory allocation failure.
 */
Queue *queue_create(size_t initial_capacity);

/**
 * @brief Deallocates the queue and its internal circular buffer.
 *
 * @param queue Pointer to queue to destroy. Safe to pass NULL.
 */
void queue_destroy(Queue *queue);

/**
 * @brief Enqueues an element at the rear of the circular buffer in O(1).
 *
 * Automatically expands circular buffer geometrically (2x) when full, preserving FIFO order.
 *
 * @param queue Pointer to queue.
 * @param value Integer value to enqueue.
 * @return true on success, false on NULL queue or allocation failure.
 */
bool queue_enqueue(Queue *queue, int value);

/**
 * @brief Dequeues an element from the front of the circular buffer in O(1).
 *
 * Advances front pointer circularly without any memory shifting.
 *
 * @param queue Pointer to queue.
 * @param out_value Optional pointer to receive dequeued value (can be NULL).
 * @return true on success, false if queue is NULL or empty.
 */
bool queue_dequeue(Queue *queue, int *out_value);

/**
 * @brief Peeks at the front element without removing it in O(1).
 *
 * @param queue Pointer to queue.
 * @param out_value Pointer to receive front value (must not be NULL).
 * @return true on success, false if queue or out_value is NULL or queue is empty.
 */
bool queue_peek(const Queue *queue, int *out_value);

/**
 * @brief Checks if the queue contains no elements.
 *
 * @param queue Pointer to queue.
 * @return true if empty or queue is NULL, false otherwise.
 */
bool queue_is_empty(const Queue *queue);

/**
 * @brief Returns the number of elements currently stored in the queue.
 *
 * @param queue Pointer to queue.
 * @return Number of elements, or 0 if queue is NULL.
 */
size_t queue_size(const Queue *queue);

/**
 * @brief Returns the current allocated capacity of the circular buffer.
 *
 * @param queue Pointer to queue.
 * @return Buffer capacity in elements, or 0 if queue is NULL.
 */
size_t queue_capacity(const Queue *queue);

#ifdef __cplusplus
}
#endif

#endif /* QUEUE_H */
