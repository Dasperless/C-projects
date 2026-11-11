#include "node.h"

#include <stdlib.h>

static Node *new_node(int data) {
    Node *n = (Node *)malloc(sizeof(Node));
    if (n != NULL) {
        n->data = data;
        n->next = NULL;
    }
    return n;
}

LinkedList *list_create(void) {
    return (LinkedList *)calloc(1, sizeof(LinkedList));
}

void list_clear(LinkedList *list) {
    if (list == NULL) {
        return;
    }
    Node *cur = list->head;
    while (cur != NULL) {
        Node *next = cur->next;
        free(cur);
        cur = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void list_destroy(LinkedList *list) {
    list_clear(list);
    free(list);
}

bool list_prepend(LinkedList *list, int data) {
    if (list == NULL) {
        return false;
    }
    Node *n = new_node(data);
    if (n == NULL) {
        return false;
    }
    n->next = list->head;
    list->head = n;
    if (list->tail == NULL) {
        list->tail = n;
    }
    list->size++;
    return true;
}

bool list_append(LinkedList *list, int data) {
    if (list == NULL) {
        return false;
    }
    Node *n = new_node(data);
    if (n == NULL) {
        return false;
    }
    if (list->tail == NULL) {
        list->head = n;
    } else {
        list->tail->next = n;
    }
    list->tail = n;
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
    for (size_t i = 0; i + 1 < index; i++) {
        prev = prev->next;
    }
    Node *n = new_node(data);
    if (n == NULL) {
        return false;
    }
    n->next = prev->next;
    prev->next = n;
    list->size++;
    return true;
}

bool list_pop_front(LinkedList *list, int *out_data) {
    if (list == NULL || list->head == NULL) {
        return false;
    }
    Node *n = list->head;
    if (out_data != NULL) {
        *out_data = n->data;
    }
    list->head = n->next;
    if (list->head == NULL) {
        list->tail = NULL;
    }
    list->size--;
    free(n);
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
    for (size_t i = 0; i + 1 < index; i++) {
        prev = prev->next;
    }
    Node *victim = prev->next;
    if (out_data != NULL) {
        *out_data = victim->data;
    }
    prev->next = victim->next;
    if (victim == list->tail) {
        list->tail = prev;
    }
    list->size--;
    free(victim);
    return true;
}

bool list_pop_back(LinkedList *list, int *out_data) {
    if (list == NULL || list->size == 0) {
        return false;
    }
    return list_remove_at(list, list->size - 1, out_data);
}

bool list_remove_value(LinkedList *list, int data) {
    ssize_t idx = list_index_of(list, data);
    if (idx < 0) {
        return false;
    }
    return list_remove_at(list, (size_t)idx, NULL);
}

bool list_get_at(const LinkedList *list, size_t index, int *out_data) {
    if (list == NULL || out_data == NULL || index >= list->size) {
        return false;
    }
    const Node *cur = list->head;
    for (size_t i = 0; i < index; i++) {
        cur = cur->next;
    }
    *out_data = cur->data;
    return true;
}

ssize_t list_index_of(const LinkedList *list, int data) {
    if (list == NULL) {
        return -1;
    }
    ssize_t i = 0;
    for (const Node *cur = list->head; cur != NULL; cur = cur->next, i++) {
        if (cur->data == data) {
            return i;
        }
    }
    return -1;
}

bool list_contains(const LinkedList *list, int data) {
    return list_index_of(list, data) >= 0;
}

size_t list_size(const LinkedList *list) {
    return list == NULL ? 0 : list->size;
}

bool list_is_empty(const LinkedList *list) {
    return list_size(list) == 0;
}

void list_reverse(LinkedList *list) {
    if (list == NULL || list->size < 2) {
        return;
    }
    Node *prev = NULL;
    Node *cur = list->head;
    list->tail = list->head;
    while (cur != NULL) {
        Node *next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
    }
    list->head = prev;
}

/* Merge two sorted chains; stable (ties favour `a`). */
static Node *merge_sorted(Node *a, Node *b) {
    Node dummy = {0, NULL};
    Node *tail = &dummy;
    while (a != NULL && b != NULL) {
        if (b->data < a->data) {
            tail->next = b;
            b = b->next;
        } else {
            tail->next = a;
            a = a->next;
        }
        tail = tail->next;
    }
    tail->next = (a != NULL) ? a : b;
    return dummy.next;
}

static Node *merge_sort(Node *head) {
    if (head == NULL || head->next == NULL) {
        return head;
    }
    /* Split using slow/fast pointers */
    Node *slow = head;
    Node *fast = head->next;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }
    Node *second = slow->next;
    slow->next = NULL;
    return merge_sorted(merge_sort(head), merge_sort(second));
}

void list_sort(LinkedList *list) {
    if (list == NULL || list->size < 2) {
        return;
    }
    list->head = merge_sort(list->head);
    Node *t = list->head;
    while (t->next != NULL) {
        t = t->next;
    }
    list->tail = t;
}
