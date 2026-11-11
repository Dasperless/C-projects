#include "stack.h"

#include <stdint.h>
#include <stdlib.h>

#define STACK_DEFAULT_INITIAL_CAPACITY 4

Stack *stack_create(size_t initial_capacity) {
    size_t cap = initial_capacity == 0 ? STACK_DEFAULT_INITIAL_CAPACITY : initial_capacity;
    if (cap > SIZE_MAX / sizeof(int)) {
        return NULL;
    }

    Stack *stack = (Stack *)malloc(sizeof(Stack));
    if (!stack) {
        return NULL;
    }

    stack->data = (int *)malloc(cap * sizeof(int));
    if (!stack->data) {
        free(stack);
        return NULL;
    }

    stack->size = 0;
    stack->capacity = cap;
    return stack;
}

void stack_destroy(Stack *stack) {
    if (!stack) {
        return;
    }
    free(stack->data);
    free(stack);
}

bool stack_push(Stack *stack, int value) {
    if (!stack) {
        return false;
    }

    if (stack->size == stack->capacity) {
        if (stack->capacity > SIZE_MAX / 2) {
            return false; /* Safeguard overflow */
        }
        size_t new_cap = stack->capacity * 2;
        if (new_cap > SIZE_MAX / sizeof(int)) {
            return false;
        }

        int *new_data = (int *)realloc(stack->data, new_cap * sizeof(int));
        if (!new_data) {
            return false;
        }

        stack->data = new_data;
        stack->capacity = new_cap;
    }

    stack->data[stack->size++] = value;
    return true;
}

bool stack_pop(Stack *stack, int *out_value) {
    if (!stack || stack->size == 0) {
        return false;
    }
    stack->size--;
    if (out_value) {
        *out_value = stack->data[stack->size];
    }
    return true;
}

bool stack_peek(const Stack *stack, int *out_value) {
    if (!stack || !out_value || stack->size == 0) {
        return false;
    }
    *out_value = stack->data[stack->size - 1];
    return true;
}

bool stack_is_empty(const Stack *stack) {
    return (!stack || stack->size == 0);
}

size_t stack_size(const Stack *stack) {
    return stack ? stack->size : 0;
}

size_t stack_capacity(const Stack *stack) {
    return stack ? stack->capacity : 0;
}
