#include "node.h"

#include <stdio.h>
#include <stdlib.h>

bool list_append_node(node **head, int data) {
    if (head == NULL) {
        return false;
    }

    node *new_node = (node *)malloc(sizeof(node));
    if (new_node == NULL) {
        return false;
    }
    new_node->data = data;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
        return true;
    }

    node *curr_node = *head;
    while (curr_node->next != NULL) {
        curr_node = curr_node->next;
    }

    curr_node->next = new_node;
    return true;
}

void append(int data, node **head) {
    (void)list_append_node(head, data);
}

void print_list(const node *head) {
    while (head != NULL) {
        printf("[%d] -> %p\n", head->data, (void *)head->next);
        head = head->next;
    }
}

bool list_min(const node *head, int *min_out) {
    if (head == NULL || min_out == NULL) {
        return false;
    }

    int min_value = head->data;
    const node *curr_node = head;
    while (curr_node != NULL) {
        if (curr_node->data < min_value) {
            min_value = curr_node->data;
        }
        curr_node = curr_node->next;
    }
    *min_out = min_value;
    return true;
}

int min(node *head) {
    int val = -1;
    if (list_min(head, &val)) {
        return val;
    }
    return -1;
}

bool list_max(const node *head, int *max_out) {
    if (head == NULL || max_out == NULL) {
        return false;
    }

    int max_value = head->data;
    const node *curr_node = head;
    while (curr_node != NULL) {
        if (curr_node->data > max_value) {
            max_value = curr_node->data;
        }
        curr_node = curr_node->next;
    }
    *max_out = max_value;
    return true;
}

int max(node *head) {
    int val = -1;
    if (list_max(head, &val)) {
        return val;
    }
    return -1;
}

void list_free(node **head) {
    if (head == NULL || *head == NULL) {
        return;
    }

    node *curr = *head;
    while (curr != NULL) {
        node *next = curr->next;
        free(curr);
        curr = next;
    }
    *head = NULL;
}

void free_list(node **head) {
    list_free(head);
}
