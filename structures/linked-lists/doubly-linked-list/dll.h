#ifndef DLL_H
#define DLL_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dll_node {
    union {
        int value;
        int data;
    };
    struct dll_node *prev;
    struct dll_node *next;
} dll_node_t, DNode;

typedef struct dll {
    dll_node_t *head;
    dll_node_t *tail;
    size_t size;
} dll_t, DoublyLinkedList;

/**
 * @brief Creates an empty doubly linked list.
 *
 * @return Pointer to new dll_t, or NULL on allocation failure.
 */
dll_t *dll_create(void);

/**
 * @brief Deallocates all nodes and the doubly linked list structure.
 *
 * @param list Pointer to list. Safe to pass NULL.
 */
void dll_destroy(dll_t *list);

/**
 * @brief Prepends a value to the front of the list in O(1).
 *
 * @param list Pointer to list.
 * @param value Integer value to push.
 * @return true on success, false on NULL list or allocation failure.
 */
bool dll_push_front(dll_t *list, int value);

/**
 * @brief Appends a value to the back of the list in O(1).
 *
 * @param list Pointer to list.
 * @param value Integer value to push.
 * @return true on success, false on NULL list or allocation failure.
 */
bool dll_push_back(dll_t *list, int value);

/**
 * @brief Removes and returns the value at the front of the list in O(1).
 *
 * @param list Pointer to list.
 * @param out_value Optional pointer to receive popped value (can be NULL).
 * @return true on success, false if list is NULL or empty.
 */
bool dll_pop_front(dll_t *list, int *out_value);

/**
 * @brief Removes and returns the value at the back of the list in O(1).
 *
 * @param list Pointer to list.
 * @param out_value Optional pointer to receive popped value (can be NULL).
 * @return true on success, false if list is NULL or empty.
 */
bool dll_pop_back(dll_t *list, int *out_value);

/**
 * @brief Finds the first node with the specified value via forward traversal.
 *
 * @param list Pointer to list.
 * @param value Value to search for.
 * @return Pointer to matching node, or NULL if not found or list is NULL.
 */
dll_node_t *dll_find(const dll_t *list, int value);

/**
 * @brief Finds the first node with the specified value via reverse traversal from tail.
 *
 * @param list Pointer to list.
 * @param value Value to search for.
 * @return Pointer to matching node, or NULL if not found or list is NULL.
 */
dll_node_t *dll_find_reverse(const dll_t *list, int value);

/**
 * @brief Inserts a new node with value immediately after the specified node in O(1).
 *
 * @param list Pointer to list.
 * @param node Node after which to insert (must be non-NULL).
 * @param value Integer value to insert.
 * @return true on success, false on NULL list, NULL node, or allocation failure.
 */
bool dll_insert_after(dll_t *list, dll_node_t *node, int value);

/**
 * @brief Inserts a new node with value immediately before the specified node in O(1).
 *
 * @param list Pointer to list.
 * @param node Node before which to insert (must be non-NULL).
 * @param value Integer value to insert.
 * @return true on success, false on NULL list, NULL node, or allocation failure.
 */
bool dll_insert_before(dll_t *list, dll_node_t *node, int value);

/**
 * @brief Splices and deallocates the specified node from the list in O(1).
 *
 * @param list Pointer to list.
 * @param node Node to remove and free (must be non-NULL).
 * @return true on success, false if list or node is NULL.
 */
bool dll_remove_node(dll_t *list, dll_node_t *node);

/**
 * @brief Returns the number of elements in the list.
 *
 * @param list Pointer to list.
 * @return Size of list, or 0 if list is NULL.
 */
size_t dll_size(const dll_t *list);

/**
 * @brief Checks if the list contains no elements.
 *
 * @param list Pointer to list.
 * @return true if empty or list is NULL, false otherwise.
 */
bool dll_is_empty(const dll_t *list);

/**
 * @brief Removes and frees all nodes from the list.
 *
 * @param list Pointer to list. Safe to pass NULL.
 */
void dll_clear(dll_t *list);

/**
 * @brief Prints list elements from head to tail.
 *
 * @param list Pointer to list.
 */
void dll_print_forward(const dll_t *list);

/**
 * @brief Prints list elements from tail to head.
 *
 * @param list Pointer to list.
 */
void dll_print_backward(const dll_t *list);

#ifdef __cplusplus
}
#endif

#endif /* DLL_H */
