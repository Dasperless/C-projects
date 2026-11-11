#ifndef NODE_H
#define NODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct Node {
    int data;
    struct Node *next;
} node;

/* Forward-compatible typedef matching LinkedList specifications */
typedef struct Node Node;

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
