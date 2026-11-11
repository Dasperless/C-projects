#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Stack Stack;

struct Stack {
    int *data;
    size_t size;
    size_t capacity;
};

/**
 * @brief Creates a dynamic LIFO stack with the specified initial capacity.
 *
 * @param initial_capacity Initial number of elements capacity (if 0, default 4 is used).
 * @return Pointer to new Stack, or NULL on memory allocation failure.
 */
Stack *stack_create(size_t initial_capacity);

/**
 * @brief Deallocates the stack and its internal buffer.
 *
 * @param stack Pointer to stack to destroy. Safe to pass NULL.
 */
void stack_destroy(Stack *stack);

/**
 * @brief Pushes a value onto the top of the stack.
 *
 * Expands capacity geometrically (2x) when full.
 *
 * @param stack Pointer to stack.
 * @param value Integer value to push.
 * @return true on success, false on NULL stack or allocation failure.
 */
bool stack_push(Stack *stack, int value);

/**
 * @brief Pops the top value from the stack.
 *
 * @param stack Pointer to stack.
 * @param out_value Optional pointer to receive popped value (can be NULL).
 * @return true on success, false if stack is NULL or empty.
 */
bool stack_pop(Stack *stack, int *out_value);

/**
 * @brief Peeks at the top value without removing it.
 *
 * @param stack Pointer to stack.
 * @param out_value Pointer to receive top value (must not be NULL).
 * @return true on success, false if stack or out_value is NULL or stack is empty.
 */
bool stack_peek(const Stack *stack, int *out_value);

/**
 * @brief Checks if the stack contains no elements.
 *
 * @param stack Pointer to stack.
 * @return true if empty or stack is NULL, false otherwise.
 */
bool stack_is_empty(const Stack *stack);

/**
 * @brief Returns the number of elements currently stored in the stack.
 *
 * @param stack Pointer to stack.
 * @return Number of elements, or 0 if stack is NULL.
 */
size_t stack_size(const Stack *stack);

/**
 * @brief Returns the current allocated capacity of the stack.
 *
 * @param stack Pointer to stack.
 * @return Capacity in elements, or 0 if stack is NULL.
 */
size_t stack_capacity(const Stack *stack);

#ifdef __cplusplus
}
#endif

#endif /* STACK_H */
