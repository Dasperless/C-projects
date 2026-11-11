#undef NDEBUG
#include "dll.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void test_dll_create_destroy(void) {
    dll_t *list = dll_create();
    assert(list != NULL);
    assert(dll_is_empty(list));
    assert(dll_size(list) == 0);
    assert(list->head == NULL);
    assert(list->tail == NULL);

    dll_destroy(list);
    dll_destroy(NULL);
    printf("PASS: test_dll_create_destroy\n");
}

static void test_dll_push_pop_ends(void) {
    dll_t *list = dll_create();
    assert(list != NULL);

    int out = 0;
    assert(!dll_pop_front(list, &out));
    assert(!dll_pop_back(list, &out));

    /* Push front 1: [10] */
    assert(dll_push_front(list, 10));
    assert(dll_size(list) == 1);
    assert(list->head != NULL && list->tail != NULL);
    assert(list->head == list->tail);
    assert(list->head->value == 10);
    assert(list->head->prev == NULL && list->head->next == NULL);

    /* Push back 2: [10, 20] */
    assert(dll_push_back(list, 20));
    assert(dll_size(list) == 2);
    assert(list->head->value == 10);
    assert(list->tail->value == 20);
    assert(list->head->next == list->tail);
    assert(list->tail->prev == list->head);

    /* Push front 0: [0, 10, 20] */
    assert(dll_push_front(list, 0));
    assert(dll_size(list) == 3);
    assert(list->head->value == 0);

    /* Pop back: pops 20 -> [0, 10] */
    assert(dll_pop_back(list, &out) && out == 20);
    assert(dll_size(list) == 2);
    assert(list->tail->value == 10);
    assert(list->tail->next == NULL);

    /* Pop front: pops 0 -> [10] */
    assert(dll_pop_front(list, &out) && out == 0);
    assert(dll_size(list) == 1);
    assert(list->head == list->tail);
    assert(list->head->value == 10);

    /* Pop front: pops 10 -> empty */
    assert(dll_pop_front(list, &out) && out == 10);
    assert(dll_is_empty(list));
    assert(list->head == NULL && list->tail == NULL);
    assert(!dll_pop_front(list, &out));

    dll_destroy(list);
    printf("PASS: test_dll_push_pop_ends\n");
}

static void test_dll_traversal_and_find(void) {
    dll_t *list = dll_create();
    assert(list != NULL);

    for (int i = 0; i < 5; i++) {
        assert(dll_push_back(list, i * 10)); /* 0, 10, 20, 30, 40 */
    }
    assert(dll_size(list) == 5);

    /* Forward traversal verification */
    dll_node_t *fwd = list->head;
    for (int i = 0; i < 5; i++) {
        assert(fwd != NULL);
        assert(fwd->value == i * 10);
        fwd = fwd->next;
    }
    assert(fwd == NULL);

    /* Reverse traversal verification */
    dll_node_t *rev = list->tail;
    for (int i = 4; i >= 0; i--) {
        assert(rev != NULL);
        assert(rev->value == i * 10);
        rev = rev->prev;
    }
    assert(rev == NULL);

    /* Find forward and reverse */
    dll_node_t *n20 = dll_find(list, 20);
    assert(n20 != NULL && n20->value == 20);

    dll_node_t *n20_rev = dll_find_reverse(list, 20);
    assert(n20_rev == n20);

    assert(dll_find(list, 999) == NULL);
    assert(dll_find_reverse(list, 999) == NULL);

    dll_destroy(list);
    printf("PASS: test_dll_traversal_and_find\n");
}

static void test_dll_splicing(void) {
    dll_t *list = dll_create();
    assert(list != NULL);

    assert(dll_push_back(list, 10));
    assert(dll_push_back(list, 30)); /* [10, 30] */

    /* Insert after middle: insert 20 after 10 -> [10, 20, 30] */
    dll_node_t *n10 = dll_find(list, 10);
    assert(n10 != NULL);
    assert(dll_insert_after(list, n10, 20));
    assert(dll_size(list) == 3);

    /* Insert after tail: insert 40 after 30 -> [10, 20, 30, 40] */
    dll_node_t *n30 = dll_find(list, 30);
    assert(n30 != NULL);
    assert(dll_insert_after(list, n30, 40));
    assert(dll_size(list) == 4);
    assert(list->tail->value == 40);

    /* Insert before head: insert 0 before 10 -> [0, 10, 20, 30, 40] */
    assert(dll_insert_before(list, list->head, 0));
    assert(dll_size(list) == 5);
    assert(list->head->value == 0);

    /* Insert before middle: insert 25 before 30 -> [0, 10, 20, 25, 30, 40] */
    assert(dll_insert_before(list, n30, 25));
    assert(dll_size(list) == 6);

    /* Remove middle node 25 -> [0, 10, 20, 30, 40] */
    dll_node_t *n25 = dll_find(list, 25);
    assert(n25 != NULL);
    assert(dll_remove_node(list, n25));
    assert(dll_size(list) == 5);
    assert(dll_find(list, 25) == NULL);

    /* Remove head node 0 -> [10, 20, 30, 40] */
    assert(dll_remove_node(list, list->head));
    assert(dll_size(list) == 4);
    assert(list->head->value == 10);

    /* Remove tail node 40 -> [10, 20, 30] */
    assert(dll_remove_node(list, list->tail));
    assert(dll_size(list) == 3);
    assert(list->tail->value == 30);

    /* Remove remaining nodes one by one */
    dll_node_t *n20 = dll_find(list, 20);
    assert(dll_remove_node(list, n20));
    assert(dll_size(list) == 2);

    assert(dll_remove_node(list, list->head)); /* removes 10 */
    assert(dll_size(list) == 1);
    assert(list->head == list->tail && list->head->value == 30);

    assert(dll_remove_node(list, list->head)); /* removes 30 */
    assert(dll_is_empty(list));
    assert(list->head == NULL && list->tail == NULL);

    dll_destroy(list);
    printf("PASS: test_dll_splicing\n");
}

static void test_dll_large_scale(void) {
    const size_t count = 10000;
    dll_t *list = dll_create();
    assert(list != NULL);

    for (size_t i = 0; i < count; i++) {
        assert(dll_push_back(list, (int)i));
    }
    assert(dll_size(list) == count);

    /* Reverse pop all */
    for (size_t i = 0; i < count; i++) {
        int out = 0;
        assert(dll_pop_back(list, &out));
        assert(out == (int)(count - 1 - i));
    }
    assert(dll_is_empty(list));

    dll_destroy(list);
    printf("PASS: test_dll_large_scale (10,000 nodes)\n");
}

static void test_dll_null_safety(void) {
    int out = 0;
    assert(dll_size(NULL) == 0);
    assert(dll_is_empty(NULL) == true);
    assert(!dll_push_front(NULL, 10));
    assert(!dll_push_back(NULL, 10));
    assert(!dll_pop_front(NULL, &out));
    assert(!dll_pop_back(NULL, &out));
    assert(dll_find(NULL, 10) == NULL);
    assert(dll_find_reverse(NULL, 10) == NULL);
    assert(!dll_insert_after(NULL, NULL, 10));
    assert(!dll_insert_before(NULL, NULL, 10));
    assert(!dll_remove_node(NULL, NULL));
    dll_clear(NULL);
    dll_print_forward(NULL);
    dll_print_backward(NULL);
    printf("PASS: test_dll_null_safety\n");
}

int main(void) {
    printf("Running doubly linked list unit tests...\n");
    test_dll_create_destroy();
    test_dll_push_pop_ends();
    test_dll_traversal_and_find();
    test_dll_splicing();
    test_dll_large_scale();
    test_dll_null_safety();
    printf("All doubly linked list tests passed successfully!\n");
    return 0;
}
