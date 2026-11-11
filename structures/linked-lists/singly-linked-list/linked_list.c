#include "singly_linked_list.h"

#include <stdio.h>
#include <stdlib.h>

/* ========================================================================= */
/*                          Lifecycle Functions                              */
/* ========================================================================= */

LinkedList *list_create(void) {
    LinkedList *list = (LinkedList *)malloc(sizeof(LinkedList));
    if (list == NULL) {
        return NULL;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

void list_clear(LinkedList *list) {
    if (list == NULL) {
        return;
    }

    Node *curr = list->head;
    while (curr != NULL) {
        Node *next = curr->next;
        free(curr);
        curr = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void list_destroy(LinkedList *list) {
    if (list == NULL) {
        return;
    }
    list_clear(list);
    free(list);
}

/* ========================================================================= */
/*                          Insertion Functions                              */
/* ========================================================================= */

bool list_prepend(LinkedList *list, int data) {
    if (list == NULL) {
        return false;
    }

    Node *new_node = (Node *)malloc(sizeof(Node));
    if (new_node == NULL) {
        return false;
    }
    new_node->data = data;
    new_node->next = list->head;

    list->head = new_node;
    if (list->tail == NULL) {
        list->tail = new_node;
    }
    list->size++;
    return true;
}

bool list_append(LinkedList *list, int data) {
    if (list == NULL) {
        return false;
    }

    Node *new_node = (Node *)malloc(sizeof(Node));
    if (new_node == NULL) {
        return false;
    }
    new_node->data = data;
    new_node->next = NULL;

    if (list->tail == NULL) {
        list->head = new_node;
        list->tail = new_node;
    } else {
        list->tail->next = new_node;
        list->tail = new_node;
    }
    list->size++;
    return true;
}

bool list_insert_at(LinkedList *list, size_t index, int data) {
    if (list == NULL || index > list->size) {
        return false;
    }

    if (index == 0) {
        return list_prepend(list, data);
    }
    if (index == list->size) {
        return list_append(list, data);
    }

    Node *prev = list->head;
    for (size_t i = 0; i < index - 1; i++) {
        prev = prev->next;
    }

    Node *new_node = (Node *)malloc(sizeof(Node));
    if (new_node == NULL) {
        return false;
    }
    new_node->data = data;
    new_node->next = prev->next;
    prev->next = new_node;

    list->size++;
    return true;
}

/* ========================================================================= */
/*                          Deletion Functions                               */
/* ========================================================================= */

bool list_pop_front(LinkedList *list, int *out_data) {
    if (list == NULL || list->head == NULL) {
        return false;
    }

    Node *node_to_remove = list->head;
    if (out_data != NULL) {
        *out_data = node_to_remove->data;
    }

    list->head = node_to_remove->next;
    if (list->head == NULL) {
        list->tail = NULL;
    }
    free(node_to_remove);
    list->size--;
    return true;
}

bool list_pop_back(LinkedList *list, int *out_data) {
    if (list == NULL || list->head == NULL) {
        return false;
    }

    if (list->head == list->tail) {
        return list_pop_front(list, out_data);
    }

    Node *curr = list->head;
    while (curr->next != list->tail) {
        curr = curr->next;
    }

    Node *node_to_remove = list->tail;
    if (out_data != NULL) {
        *out_data = node_to_remove->data;
    }

    curr->next = NULL;
    list->tail = curr;
    free(node_to_remove);
    list->size--;
    return true;
}

bool list_remove_value(LinkedList *list, int data) {
    if (list == NULL || list->head == NULL) {
        return false;
    }

    if (list->head->data == data) {
        return list_pop_front(list, NULL);
    }

    Node *curr = list->head;
    while (curr->next != NULL && curr->next->data != data) {
        curr = curr->next;
    }

    if (curr->next == NULL) {
        return false;
    }

    Node *node_to_remove = curr->next;
    curr->next = node_to_remove->next;
    if (node_to_remove == list->tail) {
        list->tail = curr;
    }
    free(node_to_remove);
    list->size--;
    return true;
}

bool list_remove_at(LinkedList *list, size_t index, int *out_data) {
    if (list == NULL || index >= list->size) {
        return false;
    }

    if (index == 0) {
        return list_pop_front(list, out_data);
    }

    Node *prev = list->head;
    for (size_t i = 0; i < index - 1; i++) {
        prev = prev->next;
    }

    Node *node_to_remove = prev->next;
    if (out_data != NULL) {
        *out_data = node_to_remove->data;
    }

    prev->next = node_to_remove->next;
    if (node_to_remove == list->tail) {
        list->tail = prev;
    }
    free(node_to_remove);
    list->size--;
    return true;
}

/* ========================================================================= */
/*                          Query & Search                                   */
/* ========================================================================= */

bool list_get_at(const LinkedList *list, size_t index, int *out_data) {
    if (list == NULL || index >= list->size || out_data == NULL) {
        return false;
    }

    Node *curr = list->head;
    for (size_t i = 0; i < index; i++) {
        curr = curr->next;
    }

    *out_data = curr->data;
    return true;
}

ssize_t list_index_of(const LinkedList *list, int data) {
    if (list == NULL) {
        return -1;
    }

    Node *curr = list->head;
    ssize_t idx = 0;
    while (curr != NULL) {
        if (curr->data == data) {
            return idx;
        }
        curr = curr->next;
        idx++;
    }
    return -1;
}

bool list_contains(const LinkedList *list, int data) {
    return list_index_of(list, data) != -1;
}

size_t list_size(const LinkedList *list) {
    if (list == NULL) {
        return 0;
    }
    return list->size;
}

bool list_is_empty(const LinkedList *list) {
    return list == NULL || list->size == 0;
}

bool list_min(const LinkedList *list, int *out_min) {
    if (list == NULL || list->head == NULL || out_min == NULL) {
        return false;
    }
    return list_min_node(list->head, out_min);
}

bool list_max(const LinkedList *list, int *out_max) {
    if (list == NULL || list->head == NULL || out_max == NULL) {
        return false;
    }
    return list_max_node(list->head, out_max);
}

void list_print(const LinkedList *list) {
    if (list == NULL) {
        return;
    }
    print_list_nodes(list->head);
}

/* ========================================================================= */
/*                          Algorithms                                       */
/* ========================================================================= */

void list_reverse(LinkedList *list) {
    if (list == NULL || list->size <= 1) {
        return;
    }

    Node *prev = NULL;
    Node *curr = list->head;
    list->tail = list->head;

    while (curr != NULL) {
        Node *next = curr->next;
        curr->next = prev;
        prev = curr;
        curr = next;
    }

    list->head = prev;
}

static Node *merge_sorted_nodes(Node *a, Node *b) {
    Node dummy;
    dummy.next = NULL;
    Node *tail = &dummy;

    while (a != NULL && b != NULL) {
        if (a->data <= b->data) {
            tail->next = a;
            a = a->next;
        } else {
            tail->next = b;
            b = b->next;
        }
        tail = tail->next;
    }

    if (a != NULL) {
        tail->next = a;
    } else {
        tail->next = b;
    }

    return dummy.next;
}

static Node *mergesort_nodes(Node *head) {
    if (head == NULL || head->next == NULL) {
        return head;
    }

    /* Fast & slow pointer to bisect the list */
    Node *slow = head;
    Node *fast = head->next;

    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }

    Node *mid = slow->next;
    slow->next = NULL;

    Node *left = mergesort_nodes(head);
    Node *right = mergesort_nodes(mid);

    return merge_sorted_nodes(left, right);
}

void list_sort(LinkedList *list) {
    if (list == NULL || list->size <= 1) {
        return;
    }

    list->head = mergesort_nodes(list->head);

    /* Update tail pointer */
    Node *curr = list->head;
    while (curr->next != NULL) {
        curr = curr->next;
    }
    list->tail = curr;
}

/* ========================================================================= */
/*               Node-level primitives & Backward Compatibility              */
/* ========================================================================= */

bool list_append_node(Node **head, int data) {
    if (head == NULL) {
        return false;
    }

    Node *new_node = (Node *)malloc(sizeof(Node));
    if (new_node == NULL) {
        return false;
    }
    new_node->data = data;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
        return true;
    }

    Node *curr_node = *head;
    while (curr_node->next != NULL) {
        curr_node = curr_node->next;
    }

    curr_node->next = new_node;
    return true;
}

void list_free_nodes(Node **head) {
    if (head == NULL || *head == NULL) {
        return;
    }

    Node *curr = *head;
    while (curr != NULL) {
        Node *next = curr->next;
        free(curr);
        curr = next;
    }
    *head = NULL;
}

void list_free(Node **head) {
    list_free_nodes(head);
}

bool list_min_node(const Node *head, int *out_min) {
    if (head == NULL || out_min == NULL) {
        return false;
    }

    int min_val = head->data;
    const Node *curr = head;
    while (curr != NULL) {
        if (curr->data < min_val) {
            min_val = curr->data;
        }
        curr = curr->next;
    }
    *out_min = min_val;
    return true;
}

bool list_max_node(const Node *head, int *out_max) {
    if (head == NULL || out_max == NULL) {
        return false;
    }

    int max_val = head->data;
    const Node *curr = head;
    while (curr != NULL) {
        if (curr->data > max_val) {
            max_val = curr->data;
        }
        curr = curr->next;
    }
    *out_max = max_val;
    return true;
}

void print_list_nodes(const Node *head) {
    while (head != NULL) {
        printf("[%d] -> %p\n", head->data, (void *)head->next);
        head = head->next;
    }
}

void print_list(const Node *head) {
    print_list_nodes(head);
}

void append(int data, Node **head) {
    (void)list_append_node(head, data);
}

int min(Node *head) {
    int val = -1;
    if (list_min_node(head, &val)) {
        return val;
    }
    return -1;
}

int max(Node *head) {
    int val = -1;
    if (list_max_node(head, &val)) {
        return val;
    }
    return -1;
}

void free_list(Node **head) {
    list_free_nodes(head);
}
