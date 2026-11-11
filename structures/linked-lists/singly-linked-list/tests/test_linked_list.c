#include "singly_linked_list.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_tests_run = 0;
static int g_tests_failed = 0;

#define ASSERT_TRUE(expr)                                                                          \
    do {                                                                                           \
        g_tests_run++;                                                                             \
        if (!(expr)) {                                                                             \
            fprintf(stderr, "FAIL: %s:%d: ASSERT_TRUE(%s)\n", __FILE__, __LINE__, #expr);          \
            g_tests_failed++;                                                                      \
        }                                                                                          \
    } while (0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))

/* ========================================================================= */
/*                          Node-level Primitives Tests                      */
/* ========================================================================= */

static void test_node_primitives(void) {
    Node *head = NULL;
    int val = 0;

    ASSERT_FALSE(list_min_node(head, &val));
    ASSERT_FALSE(list_max_node(head, &val));
    ASSERT_FALSE(list_append_node(NULL, 10));

    ASSERT_TRUE(list_append_node(&head, 15));
    ASSERT_TRUE(list_append_node(&head, -5));
    ASSERT_TRUE(list_append_node(&head, 30));

    ASSERT_TRUE(list_min_node(head, &val));
    ASSERT_EQ(val, -5);
    ASSERT_TRUE(list_max_node(head, &val));
    ASSERT_EQ(val, 30);

    /* Test backward compatibility wrappers */
    ASSERT_EQ(min(head), -5);
    ASSERT_EQ(max(head), 30);

    list_free_nodes(&head);
    ASSERT_TRUE(head == NULL);
    list_free_nodes(&head); /* double free safety */
}

/* ========================================================================= */
/*                          LinkedList CRUD Tests                            */
/* ========================================================================= */

static void test_create_destroy_clear(void) {
    LinkedList *list = list_create();
    ASSERT_TRUE(list != NULL);
    ASSERT_TRUE(list->head == NULL);
    ASSERT_TRUE(list->tail == NULL);
    ASSERT_EQ(list_size(list), (size_t)0);
    ASSERT_TRUE(list_is_empty(list));

    list_clear(list);
    ASSERT_TRUE(list_is_empty(list));

    list_destroy(list);
    list_destroy(NULL); /* NULL safety */
}

static void test_prepend_and_append(void) {
    LinkedList *list = list_create();

    /* Prepend on empty */
    ASSERT_TRUE(list_prepend(list, 20));
    ASSERT_EQ(list_size(list), (size_t)1);
    ASSERT_TRUE(list->head != NULL);
    ASSERT_TRUE(list->tail == list->head);
    ASSERT_EQ(list->head->data, 20);

    /* Prepend second */
    ASSERT_TRUE(list_prepend(list, 10));
    ASSERT_EQ(list_size(list), (size_t)2);
    ASSERT_EQ(list->head->data, 10);
    ASSERT_EQ(list->tail->data, 20);

    /* Append */
    ASSERT_TRUE(list_append(list, 30));
    ASSERT_EQ(list_size(list), (size_t)3);
    ASSERT_EQ(list->tail->data, 30);
    ASSERT_EQ(list->head->next->data, 20);
    ASSERT_EQ(list->head->next->next->data, 30);

    list_destroy(list);
}

static void test_insert_at(void) {
    LinkedList *list = list_create();

    /* Out-of-bounds on empty */
    ASSERT_FALSE(list_insert_at(list, 1, 999));

    /* Insert at 0 (empty) */
    ASSERT_TRUE(list_insert_at(list, 0, 100));
    ASSERT_EQ(list_size(list), (size_t)1);
    ASSERT_EQ(list->head->data, 100);
    ASSERT_EQ(list->tail->data, 100);

    /* Insert at index 1 (end) */
    ASSERT_TRUE(list_insert_at(list, 1, 300));
    ASSERT_EQ(list_size(list), (size_t)2);
    ASSERT_EQ(list->tail->data, 300);

    /* Insert at index 1 (middle) */
    ASSERT_TRUE(list_insert_at(list, 1, 200));
    ASSERT_EQ(list_size(list), (size_t)3);

    int val = 0;
    ASSERT_TRUE(list_get_at(list, 0, &val));
    ASSERT_EQ(val, 100);
    ASSERT_TRUE(list_get_at(list, 1, &val));
    ASSERT_EQ(val, 200);
    ASSERT_TRUE(list_get_at(list, 2, &val));
    ASSERT_EQ(val, 300);

    /* Out of bounds */
    ASSERT_FALSE(list_insert_at(list, 4, 400));
    ASSERT_FALSE(list_insert_at(NULL, 0, 10));

    list_destroy(list);
}

static void test_pop_front_and_pop_back(void) {
    LinkedList *list = list_create();
    int val = 0;

    ASSERT_FALSE(list_pop_front(list, &val));
    ASSERT_FALSE(list_pop_back(list, &val));

    list_append(list, 1);
    list_append(list, 2);
    list_append(list, 3);

    /* Pop front */
    ASSERT_TRUE(list_pop_front(list, &val));
    ASSERT_EQ(val, 1);
    ASSERT_EQ(list_size(list), (size_t)2);
    ASSERT_EQ(list->head->data, 2);

    /* Pop back */
    ASSERT_TRUE(list_pop_back(list, &val));
    ASSERT_EQ(val, 3);
    ASSERT_EQ(list_size(list), (size_t)1);
    ASSERT_EQ(list->tail->data, 2);
    ASSERT_TRUE(list->head == list->tail);

    /* Pop back single remaining item */
    ASSERT_TRUE(list_pop_back(list, &val));
    ASSERT_EQ(val, 2);
    ASSERT_EQ(list_size(list), (size_t)0);
    ASSERT_TRUE(list->head == NULL);
    ASSERT_TRUE(list->tail == NULL);

    /* Pop front with NULL out_data */
    list_append(list, 42);
    ASSERT_TRUE(list_pop_front(list, NULL));
    ASSERT_TRUE(list_is_empty(list));

    list_destroy(list);
}

static void test_remove_at_and_remove_value(void) {
    LinkedList *list = list_create();
    int val = 0;

    for (int i = 0; i < 5; i++) {
        list_append(list, i * 10); /* 0, 10, 20, 30, 40 */
    }

    /* Out of bounds removal */
    ASSERT_FALSE(list_remove_at(list, 5, &val));

    /* Remove middle index 2 (value 20) */
    ASSERT_TRUE(list_remove_at(list, 2, &val));
    ASSERT_EQ(val, 20);
    ASSERT_EQ(list_size(list), (size_t)4);

    /* Remove by value (non-existent) */
    ASSERT_FALSE(list_remove_value(list, 999));

    /* Remove head by value (0) */
    ASSERT_TRUE(list_remove_value(list, 0));
    ASSERT_EQ(list_size(list), (size_t)3);
    ASSERT_EQ(list->head->data, 10);

    /* Remove tail by value (40) */
    ASSERT_TRUE(list_remove_value(list, 40));
    ASSERT_EQ(list_size(list), (size_t)2);
    ASSERT_EQ(list->tail->data, 30);

    /* Remaining elements are 10, 30 */
    ASSERT_TRUE(list_get_at(list, 0, &val));
    ASSERT_EQ(val, 10);
    ASSERT_TRUE(list_get_at(list, 1, &val));
    ASSERT_EQ(val, 30);

    list_destroy(list);
}

static void test_query_and_search(void) {
    LinkedList *list = list_create();
    int val = 0;

    ASSERT_FALSE(list_get_at(list, 0, &val));
    ASSERT_EQ(list_index_of(list, 10), (ssize_t)-1);
    ASSERT_FALSE(list_contains(list, 10));

    list_append(list, 10);
    list_append(list, 20);
    list_append(list, 30);

    ASSERT_EQ(list_index_of(list, 10), (ssize_t)0);
    ASSERT_EQ(list_index_of(list, 20), (ssize_t)1);
    ASSERT_EQ(list_index_of(list, 30), (ssize_t)2);
    ASSERT_EQ(list_index_of(list, 40), (ssize_t)-1);

    ASSERT_TRUE(list_contains(list, 20));
    ASSERT_FALSE(list_contains(list, 99));

    ASSERT_TRUE(list_min(list, &val));
    ASSERT_EQ(val, 10);
    ASSERT_TRUE(list_max(list, &val));
    ASSERT_EQ(val, 30);

    list_destroy(list);
}

/* ========================================================================= */
/*                          Algorithms Tests                                 */
/* ========================================================================= */

static void test_reverse_in_place(void) {
    LinkedList *list = list_create();

    /* Reverse empty list */
    list_reverse(list);
    ASSERT_TRUE(list_is_empty(list));

    /* Reverse 1-element list */
    list_append(list, 100);
    list_reverse(list);
    ASSERT_EQ(list->head->data, 100);
    ASSERT_EQ(list->tail->data, 100);
    ASSERT_TRUE(list->head->next == NULL);

    /* Reverse 2-element list */
    list_append(list, 200); /* [100, 200] */
    list_reverse(list);     /* [200, 100] */
    ASSERT_EQ(list->head->data, 200);
    ASSERT_EQ(list->tail->data, 100);
    ASSERT_TRUE(list->tail->next == NULL);

    /* Reverse 100 elements */
    list_clear(list);
    for (int i = 0; i < 100; i++) {
        list_append(list, i);
    }
    list_reverse(list);
    ASSERT_EQ(list_size(list), (size_t)100);
    ASSERT_EQ(list->head->data, 99);
    ASSERT_EQ(list->tail->data, 0);

    int val = 0;
    for (size_t i = 0; i < 100; i++) {
        ASSERT_TRUE(list_get_at(list, i, &val));
        ASSERT_EQ(val, 99 - (int)i);
    }

    list_destroy(list);
}

static void test_mergesort(void) {
    LinkedList *list = list_create();

    /* Sort empty and 1-element list */
    list_sort(list);
    ASSERT_TRUE(list_is_empty(list));

    list_append(list, 42);
    list_sort(list);
    ASSERT_EQ(list->head->data, 42);
    ASSERT_EQ(list->tail->data, 42);

    /* Sort reverse sorted list */
    list_clear(list);
    for (int i = 10; i >= 1; i--) {
        list_append(list, i);
    }
    list_sort(list);
    ASSERT_EQ(list_size(list), (size_t)10);
    ASSERT_EQ(list->head->data, 1);
    ASSERT_EQ(list->tail->data, 10);
    for (size_t i = 0; i < 10; i++) {
        int val = 0;
        ASSERT_TRUE(list_get_at(list, i, &val));
        ASSERT_EQ(val, (int)i + 1);
    }

    /* Sort list with duplicates */
    list_clear(list);
    list_append(list, 5);
    list_append(list, 2);
    list_append(list, 5);
    list_append(list, 1);
    list_append(list, 2);
    list_append(list, 5);
    list_sort(list);
    int expected[] = {1, 2, 2, 5, 5, 5};
    for (size_t i = 0; i < 6; i++) {
        int val = 0;
        ASSERT_TRUE(list_get_at(list, i, &val));
        ASSERT_EQ(val, expected[i]);
    }
    ASSERT_EQ(list->head->data, 1);
    ASSERT_EQ(list->tail->data, 5);

    /* Sort 1000 pseudo-random numbers */
    list_clear(list);
    srand(12345);
    for (int i = 0; i < 1000; i++) {
        list_append(list, (rand() % 2000) - 1000);
    }
    list_sort(list);
    ASSERT_EQ(list_size(list), (size_t)1000);

    /* Verify strictly non-decreasing order and tail invariant */
    Node *curr = list->head;
    Node *prev = NULL;
    size_t count = 0;
    while (curr != NULL) {
        if (prev != NULL) {
            ASSERT_TRUE(prev->data <= curr->data);
        }
        prev = curr;
        curr = curr->next;
        count++;
    }
    ASSERT_EQ(count, (size_t)1000);
    ASSERT_TRUE(list->tail == prev);
    ASSERT_TRUE(list->tail->next == NULL);

    list_destroy(list);
}

int main(void) {
    printf("Running singly linked list comprehensive unit tests...\n");

    test_node_primitives();
    test_create_destroy_clear();
    test_prepend_and_append();
    test_insert_at();
    test_pop_front_and_pop_back();
    test_remove_at_and_remove_value();
    test_query_and_search();
    test_reverse_in_place();
    test_mergesort();

    printf("Tests run: %d, Failures: %d\n", g_tests_run, g_tests_failed);
    return (g_tests_failed == 0) ? 0 : 1;
}
