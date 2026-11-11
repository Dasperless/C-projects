#ifndef NODE_H
#define NODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

typedef struct Node {
    int data;
    struct Node *next;
} node;

/* Forward-compatible typedef matching LinkedList specifications */
typedef struct Node Node;

/* Container that tracks head, tail and size for O(1) prepend/append/size */
typedef struct LinkedList {
    Node *head;
    Node *tail;
    size_t size;
} LinkedList;

/* Lifecycle */
LinkedList *list_create(void);
void list_destroy(LinkedList *list);
void list_clear(LinkedList *list);

/* Insertion */
bool list_prepend(LinkedList *list, int data);
bool list_append(LinkedList *list, int data);
bool list_insert_at(LinkedList *list, size_t index, int data);

/* Deletion (out_data may be NULL) */
bool list_pop_front(LinkedList *list, int *out_data);
bool list_pop_back(LinkedList *list, int *out_data);
bool list_remove_value(LinkedList *list, int data);
bool list_remove_at(LinkedList *list, size_t index, int *out_data);

/* Search and access */
bool list_get_at(const LinkedList *list, size_t index, int *out_data);
ssize_t list_index_of(const LinkedList *list, int data);
bool list_contains(const LinkedList *list, int data);
size_t list_size(const LinkedList *list);
bool list_is_empty(const LinkedList *list);

/* Algorithms */
void list_reverse(LinkedList *list);
void list_sort(LinkedList *list);

/* Node-level manipulation functions */
bool list_append_node(node **head, int data);
void list_free(node **head);
bool list_min(const node *head, int *min_out);
bool list_max(const node *head, int *max_out);
void print_list(const node *head);

/* Backward compatibility wrappers */
void append(int data, node **head);
int min(node *head);
int max(node *head);
void free_list(node **head);

#endif /* NODE_H */
