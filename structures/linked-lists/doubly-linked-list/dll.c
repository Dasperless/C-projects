#include "dll.h"

#include <stdio.h>
#include <stdlib.h>

dll_t *dll_create(void) {
    dll_t *list = (dll_t *)malloc(sizeof(dll_t));
    if (!list) {
        return NULL;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

void dll_clear(dll_t *list) {
    if (!list) {
        return;
    }
    dll_node_t *curr = list->head;
    while (curr) {
        dll_node_t *next = curr->next;
        free(curr);
        curr = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void dll_destroy(dll_t *list) {
    if (!list) {
        return;
    }
    dll_clear(list);
    free(list);
}

bool dll_push_front(dll_t *list, int value) {
    if (!list) {
        return false;
    }

    dll_node_t *node = (dll_node_t *)malloc(sizeof(dll_node_t));
    if (!node) {
        return false;
    }

    node->value = value;
    node->prev = NULL;
    node->next = list->head;

    if (list->head) {
        list->head->prev = node;
    } else {
        list->tail = node;
    }
    list->head = node;
    list->size++;
    return true;
}

bool dll_push_back(dll_t *list, int value) {
    if (!list) {
        return false;
    }

    dll_node_t *node = (dll_node_t *)malloc(sizeof(dll_node_t));
    if (!node) {
        return false;
    }

    node->value = value;
    node->next = NULL;
    node->prev = list->tail;

    if (list->tail) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->tail = node;
    list->size++;
    return true;
}

bool dll_pop_front(dll_t *list, int *out_value) {
    if (!list || !list->head) {
        return false;
    }

    dll_node_t *node = list->head;
    if (out_value) {
        *out_value = node->value;
    }

    list->head = node->next;
    if (list->head) {
        list->head->prev = NULL;
    } else {
        list->tail = NULL;
    }

    free(node);
    list->size--;
    return true;
}

bool dll_pop_back(dll_t *list, int *out_value) {
    if (!list || !list->tail) {
        return false;
    }

    dll_node_t *node = list->tail;
    if (out_value) {
        *out_value = node->value;
    }

    list->tail = node->prev;
    if (list->tail) {
        list->tail->next = NULL;
    } else {
        list->head = NULL;
    }

    free(node);
    list->size--;
    return true;
}

dll_node_t *dll_find(const dll_t *list, int value) {
    if (!list) {
        return NULL;
    }
    dll_node_t *curr = list->head;
    while (curr) {
        if (curr->value == value) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

dll_node_t *dll_find_reverse(const dll_t *list, int value) {
    if (!list) {
        return NULL;
    }
    dll_node_t *curr = list->tail;
    while (curr) {
        if (curr->value == value) {
            return curr;
        }
        curr = curr->prev;
    }
    return NULL;
}

bool dll_insert_after(dll_t *list, dll_node_t *node, int value) {
    if (!list || !node) {
        return false;
    }

    if (node == list->tail) {
        return dll_push_back(list, value);
    }

    dll_node_t *new_node = (dll_node_t *)malloc(sizeof(dll_node_t));
    if (!new_node) {
        return false;
    }

    new_node->value = value;
    new_node->prev = node;
    new_node->next = node->next;

    node->next->prev = new_node;
    node->next = new_node;
    list->size++;
    return true;
}

bool dll_insert_before(dll_t *list, dll_node_t *node, int value) {
    if (!list || !node) {
        return false;
    }

    if (node == list->head) {
        return dll_push_front(list, value);
    }

    dll_node_t *new_node = (dll_node_t *)malloc(sizeof(dll_node_t));
    if (!new_node) {
        return false;
    }

    new_node->value = value;
    new_node->next = node;
    new_node->prev = node->prev;

    node->prev->next = new_node;
    node->prev = new_node;
    list->size++;
    return true;
}

bool dll_remove_node(dll_t *list, dll_node_t *node) {
    if (!list || !node) {
        return false;
    }

    if (node == list->head) {
        return dll_pop_front(list, NULL);
    }

    if (node == list->tail) {
        return dll_pop_back(list, NULL);
    }

    node->prev->next = node->next;
    node->next->prev = node->prev;
    free(node);
    list->size--;
    return true;
}

size_t dll_size(const dll_t *list) {
    return list ? list->size : 0;
}

bool dll_is_empty(const dll_t *list) {
    return (!list || list->size == 0);
}

void dll_print_forward(const dll_t *list) {
    if (!list) {
        return;
    }
    printf("[Head -> Tail]: ");
    dll_node_t *curr = list->head;
    while (curr) {
        printf("%d%s", curr->value, curr->next ? " <-> " : "");
        curr = curr->next;
    }
    printf("\n");
}

void dll_print_backward(const dll_t *list) {
    if (!list) {
        return;
    }
    printf("[Tail -> Head]: ");
    dll_node_t *curr = list->tail;
    while (curr) {
        printf("%d%s", curr->value, curr->prev ? " <-> " : "");
        curr = curr->prev;
    }
    printf("\n");
}
