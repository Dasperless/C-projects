#ifndef SINGLY_LINKED_LIST_H
#define SINGLY_LINKED_LIST_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

typedef struct Node {
    int data;
    struct Node *next;
} Node;

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
bool list_prepend(LinkedList *list, int data);                 /* O(1) */
bool list_append(LinkedList *list, int data);                  /* O(1) */
bool list_insert_at(LinkedList *list, size_t index, int data); /* O(N) */

/* Deletion */
bool list_pop_front(LinkedList *list, int *out_data);               /* O(1) */
bool list_pop_back(LinkedList *list, int *out_data);                /* O(N) */
bool list_remove_value(LinkedList *list, int data);                 /* O(N) */
bool list_remove_at(LinkedList *list, size_t index, int *out_data); /* O(N) */

/* Query & Search */
bool list_get_at(const LinkedList *list, size_t index, int *out_data);
ssize_t list_index_of(const LinkedList *list, int data);
bool list_contains(const LinkedList *list, int data);
size_t list_size(const LinkedList *list);
bool list_is_empty(const LinkedList *list);
bool list_min(const LinkedList *list, int *out_min);
bool list_max(const LinkedList *list, int *out_max);
void list_print(const LinkedList *list);

/* Algorithms */
void list_reverse(LinkedList *list); /* O(N) time, O(1) space */
void list_sort(LinkedList *list);    /* O(N log N) MergeSort */

/* Node-level primitives & backward compatibility */
typedef struct Node node;
void list_free_nodes(Node **head);
void list_free(Node **head);
bool list_append_node(Node **head, int data);
bool list_min_node(const Node *head, int *out_min);
bool list_max_node(const Node *head, int *out_max);
void print_list_nodes(const Node *head);
void print_list(const Node *head);

/* Backward compatibility wrappers */
void append(int data, Node **head);
int min(Node *head);
int max(Node *head);
void free_list(Node **head);

#endif /* SINGLY_LINKED_LIST_H */
