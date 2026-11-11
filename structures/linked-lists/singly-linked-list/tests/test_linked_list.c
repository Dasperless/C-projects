#include "node.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static node *build(const int *vals, size_t n) {
    node *head = NULL;
    for (size_t i = 0; i < n; i++) {
        assert(list_append_node(&head, vals[i]));
    }
    return head;
}

static void test_list_init_empty(void) {
    node *head = NULL;
    int out = 0;
    assert(!list_min(head, &out));
    assert(!list_max(head, &out));
    assert(!list_append_node(NULL, 1));
}

static void test_list_append(void) {
    const int vals[] = {3, 1, 4, 1, 5};
    node *head = build(vals, 5);
    const node *cur = head;
    for (size_t i = 0; i < 5; i++) {
        assert(cur != NULL && cur->data == vals[i]);
        cur = cur->next;
    }
    assert(cur == NULL);
    list_free(&head);
    assert(head == NULL);
}

static void test_list_min_max_positive(void) {
    const int vals[] = {7, 2, 9, 4};
    node *head = build(vals, 4);
    int mn = 0;
    int mx = 0;
    assert(list_min(head, &mn) && mn == 2);
    assert(list_max(head, &mx) && mx == 9);
    assert(min(head) == 2 && max(head) == 9);
    list_free(&head);
}

static void test_list_min_max_negative(void) {
    const int vals[] = {-5, 0, -20, 3};
    node *head = build(vals, 4);
    int mn = 0;
    int mx = 0;
    assert(list_min(head, &mn) && mn == -20);
    assert(list_max(head, &mx) && mx == 3);
    list_free(&head);
}

static void test_list_min_max_empty(void) {
    int out = 123;
    assert(!list_min(NULL, &out));
    assert(!list_max(NULL, &out));
    assert(out == 123);
    node *one = NULL;
    assert(list_append_node(&one, 1));
    assert(!list_min(one, NULL));
    assert(!list_max(one, NULL));
    list_free(&one);
}

static void test_list_free_null(void) {
    list_free(NULL);
    node *head = NULL;
    list_free(&head);
    free_list(&head);
    assert(head == NULL);
}

static void test_list_free_multiple(void) {
    node *head = NULL;
    for (int i = 0; i < 1000; i++) {
        assert(list_append_node(&head, i));
    }
    list_free(&head);
    assert(head == NULL);
}

int main(void) {
    test_list_init_empty();
    test_list_append();
    test_list_min_max_positive();
    test_list_min_max_negative();
    test_list_min_max_empty();
    test_list_free_null();
    test_list_free_multiple();
    printf("All singly linked list tests passed.\n");
    return 0;
}
