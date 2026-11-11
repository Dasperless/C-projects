#ifndef VECTOR_H
#define VECTOR_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Vector Vector;

struct Vector {
    int *data;
    size_t size;
    size_t capacity;
};

/**
 * @brief Creates a new dynamic vector with the specified initial capacity.
 *
 * @param initial_capacity Initial number of elements to allocate storage for.
 *                         If 0, no immediate allocation is performed until first push.
 * @return Pointer to new Vector, or NULL on memory allocation failure.
 */
Vector *vector_create(size_t initial_capacity);

/**
 * @brief Deallocates the vector and its underlying buffer.
 *
 * @param vec Pointer to vector to destroy. Safe to pass NULL.
 */
void vector_destroy(Vector *vec);

/**
 * @brief Appends an element to the end of the vector.
 *
 * Expands capacity geometrically (2x) when full.
 *
 * @param vec Pointer to vector.
 * @param value Integer value to append.
 * @return true on success, false on NULL pointer or allocation failure.
 */
bool vector_push_back(Vector *vec, int value);

/**
 * @brief Removes and returns the last element of the vector.
 *
 * @param vec Pointer to vector.
 * @param out_value Optional pointer to receive the popped value (can be NULL).
 * @return true on success, false if vec is NULL or vector is empty.
 */
bool vector_pop_back(Vector *vec, int *out_value);

/**
 * @brief Retrieves the element at the specified index.
 *
 * @param vec Pointer to vector.
 * @param index 0-based index to access.
 * @param out_value Pointer to receive the retrieved value (must not be NULL).
 * @return true if index is in bounds, false if vec or out_value is NULL or index >= size.
 */
bool vector_get(const Vector *vec, size_t index, int *out_value);

/**
 * @brief Updates the element at the specified index.
 *
 * @param vec Pointer to vector.
 * @param index 0-based index to update.
 * @param value New value to store.
 * @return true if index is in bounds, false if vec is NULL or index >= size.
 */
bool vector_set(Vector *vec, size_t index, int value);

/**
 * @brief Inserts an element at the specified index, shifting trailing elements right.
 *
 * Index equal to size performs an append.
 *
 * @param vec Pointer to vector.
 * @param index 0-based index at which to insert (must be <= size).
 * @param value Integer value to insert.
 * @return true on success, false if vec is NULL, index > size, or allocation fails.
 */
bool vector_insert(Vector *vec, size_t index, int value);

/**
 * @brief Erases the element at the specified index, shifting trailing elements left.
 *
 * @param vec Pointer to vector.
 * @param index 0-based index of element to remove (must be < size).
 * @return true on success, false if vec is NULL or index >= size.
 */
bool vector_erase(Vector *vec, size_t index);

/**
 * @brief Clears all elements from the vector without deallocating capacity.
 *
 * @param vec Pointer to vector. Safe to pass NULL.
 */
void vector_clear(Vector *vec);

/**
 * @brief Shrinks allocated buffer capacity to match the current size.
 *
 * If size is 0, deallocates buffer and resets capacity to 0.
 *
 * @param vec Pointer to vector.
 * @return true on success, false on NULL pointer or realloc failure.
 */
bool vector_shrink_to_fit(Vector *vec);

/**
 * @brief Returns the current number of elements in the vector.
 *
 * @param vec Pointer to vector.
 * @return Number of elements, or 0 if vec is NULL.
 */
size_t vector_size(const Vector *vec);

/**
 * @brief Returns the current allocated capacity of the vector.
 *
 * @param vec Pointer to vector.
 * @return Current capacity in elements, or 0 if vec is NULL.
 */
size_t vector_capacity(const Vector *vec);

/**
 * @brief Checks whether the vector contains no elements.
 *
 * @param vec Pointer to vector.
 * @return true if empty or vec is NULL, false otherwise.
 */
bool vector_is_empty(const Vector *vec);

#ifdef __cplusplus
}
#endif

#endif /* VECTOR_H */
