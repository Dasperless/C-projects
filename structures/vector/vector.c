#include "vector.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VECTOR_DEFAULT_INITIAL_CAPACITY 4

static bool vector_ensure_capacity(Vector *vec, size_t min_capacity) {
    if (!vec) {
        return false;
    }
    if (vec->capacity >= min_capacity) {
        return true;
    }

    size_t new_cap = vec->capacity;
    if (new_cap == 0) {
        new_cap = (min_capacity > VECTOR_DEFAULT_INITIAL_CAPACITY)
                      ? min_capacity
                      : VECTOR_DEFAULT_INITIAL_CAPACITY;
    } else {
        while (new_cap < min_capacity) {
            if (new_cap > SIZE_MAX / 2) {
                return false; /* Overflow safeguard */
            }
            new_cap *= 2;
        }
    }

    if (new_cap > SIZE_MAX / sizeof(int)) {
        return false;
    }

    int *new_data = (int *)realloc(vec->data, new_cap * sizeof(int));
    if (!new_data) {
        return false;
    }

    vec->data = new_data;
    vec->capacity = new_cap;
    return true;
}

Vector *vector_create(size_t initial_capacity) {
    Vector *vec = (Vector *)malloc(sizeof(Vector));
    if (!vec) {
        return NULL;
    }

    vec->size = 0;
    vec->capacity = 0;
    vec->data = NULL;

    if (initial_capacity > 0) {
        if (initial_capacity > SIZE_MAX / sizeof(int)) {
            free(vec);
            return NULL;
        }
        vec->data = (int *)malloc(initial_capacity * sizeof(int));
        if (!vec->data) {
            free(vec);
            return NULL;
        }
        vec->capacity = initial_capacity;
    }

    return vec;
}

void vector_destroy(Vector *vec) {
    if (!vec) {
        return;
    }
    free(vec->data);
    free(vec);
}

bool vector_push_back(Vector *vec, int value) {
    if (!vec) {
        return false;
    }
    if (vec->size == vec->capacity) {
        if (!vector_ensure_capacity(vec, vec->size + 1)) {
            return false;
        }
    }
    vec->data[vec->size] = value;
    vec->size++;
    return true;
}

bool vector_pop_back(Vector *vec, int *out_value) {
    if (!vec || vec->size == 0) {
        return false;
    }
    vec->size--;
    if (out_value) {
        *out_value = vec->data[vec->size];
    }
    return true;
}

bool vector_get(const Vector *vec, size_t index, int *out_value) {
    if (!vec || !out_value || index >= vec->size) {
        return false;
    }
    *out_value = vec->data[index];
    return true;
}

bool vector_set(Vector *vec, size_t index, int value) {
    if (!vec || index >= vec->size) {
        return false;
    }
    vec->data[index] = value;
    return true;
}

bool vector_insert(Vector *vec, size_t index, int value) {
    if (!vec || index > vec->size) {
        return false;
    }
    if (vec->size == vec->capacity) {
        if (!vector_ensure_capacity(vec, vec->size + 1)) {
            return false;
        }
    }

    if (index < vec->size) {
        memmove(&vec->data[index + 1], &vec->data[index], (vec->size - index) * sizeof(int));
    }

    vec->data[index] = value;
    vec->size++;
    return true;
}

bool vector_erase(Vector *vec, size_t index) {
    if (!vec || index >= vec->size) {
        return false;
    }

    if (index + 1 < vec->size) {
        memmove(&vec->data[index], &vec->data[index + 1], (vec->size - index - 1) * sizeof(int));
    }

    vec->size--;
    return true;
}

void vector_clear(Vector *vec) {
    if (!vec) {
        return;
    }
    vec->size = 0;
}

bool vector_shrink_to_fit(Vector *vec) {
    if (!vec) {
        return false;
    }
    if (vec->capacity == vec->size) {
        return true;
    }
    if (vec->size == 0) {
        free(vec->data);
        vec->data = NULL;
        vec->capacity = 0;
        return true;
    }

    int *new_data = (int *)realloc(vec->data, vec->size * sizeof(int));
    if (!new_data) {
        return false;
    }

    vec->data = new_data;
    vec->capacity = vec->size;
    return true;
}

size_t vector_size(const Vector *vec) {
    return vec ? vec->size : 0;
}

size_t vector_capacity(const Vector *vec) {
    return vec ? vec->capacity : 0;
}

bool vector_is_empty(const Vector *vec) {
    return (!vec || vec->size == 0);
}
