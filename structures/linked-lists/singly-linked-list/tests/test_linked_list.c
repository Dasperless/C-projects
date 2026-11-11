#undef NDEBUG
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

/* ---- LinkedList container (issue #6) ---- */

static void check_invariants(const LinkedList *l) {
    size_t n = 0;
    const Node *last = NULL;
    for (const Node *c = l->head; c != NULL; c = c->next) {
        n++;
        last = c;
    }
    assert(n == l->size);
    assert(l->tail == last);
}

static LinkedList *make(const int *vals, size_t n) {
    LinkedList *l = list_create();
    assert(l != NULL);
    for (size_t i = 0; i < n; i++) {
        assert(list_append(l, vals[i]));
    }
    return l;
}

static void expect(const LinkedList *l, const int *vals, size_t n) {
    assert(list_size(l) == n);
    for (size_t i = 0; i < n; i++) {
        int v = 0;
        assert(list_get_at(l, i, &v) && v == vals[i]);
    }
    check_invariants(l);
}

static void test_container_prepend_append(void) {
    LinkedList *l = list_create();
    assert(list_is_empty(l));
    assert(list_prepend(l, 2));
    assert(l->head == l->tail && l->size == 1);
    assert(list_prepend(l, 1));
    assert(list_append(l, 3));
    const int want[] = {1, 2, 3};
    expect(l, want, 3);
    assert(!list_prepend(NULL, 1) && !list_append(NULL, 1));
    list_destroy(l);
    list_destroy(NULL);
}

static void test_container_insert_at(void) {
    LinkedList *l = list_create();
    assert(list_insert_at(l, 0, 10));
    assert(list_insert_at(l, 1, 30));
    assert(list_insert_at(l, 1, 20));
    assert(list_insert_at(l, 3, 40));
    assert(!list_insert_at(l, 5, 99));
    const int want[] = {10, 20, 30, 40};
    expect(l, want, 4);
    list_destroy(l);
}

static void test_container_pop_and_remove(void) {
    int out = 0;
    LinkedList *l = list_create();
    assert(!list_pop_front(l, &out) && !list_pop_back(l, &out));
    assert(!list_remove_at(l, 0, &out) && !list_remove_value(l, 1));

    assert(list_append(l, 5));
    assert(list_pop_back(l, &out) && out == 5);
    assert(l->head == NULL && l->tail == NULL && l->size == 0);

    const int vals[] = {1, 2, 3, 4, 5};
    for (size_t i = 0; i < 5; i++) {
        assert(list_append(l, vals[i]));
    }
    assert(list_pop_front(l, &out) && out == 1);
    assert(list_pop_back(l, &out) && out == 5);
    assert(list_remove_at(l, 1, &out) && out == 3);
    assert(!list_remove_at(l, 2, &out));
    assert(list_remove_value(l, 4));
    assert(!list_remove_value(l, 4));
    const int want[] = {2};
    expect(l, want, 1);
    assert(list_pop_front(l, NULL));
    assert(list_is_empty(l));
    list_destroy(l);
}

static void test_container_search(void) {
    const int vals[] = {4, 8, 15, 8};
    LinkedList *l = make(vals, 4);
    int out = 0;
    assert(list_index_of(l, 8) == 1);
    assert(list_index_of(l, 99) == -1);
    assert(list_contains(l, 15) && !list_contains(l, 16));
    assert(!list_get_at(l, 4, &out));
    assert(!list_get_at(l, 0, NULL));
    assert(list_index_of(NULL, 1) == -1 && !list_contains(NULL, 1));
    list_clear(l);
    assert(list_is_empty(l));
    check_invariants(l);
    list_destroy(l);
}

static void test_container_reverse(void) {
    for (size_t n = 0; n <= 6; n++) {
        int vals[6];
        int rev[6];
        for (size_t i = 0; i < n; i++) {
            vals[i] = (int)i;
            rev[n - 1 - i] = (int)i;
        }
        LinkedList *l = make(vals, n);
        list_reverse(l);
        expect(l, rev, n);
        list_destroy(l);
    }
    list_reverse(NULL);
}

static int cmp_int(const void *a, const void *b) {
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

static void test_container_sort(void) {
    const int sorted[] = {1, 2, 3, 4};
    const int reversed[] = {4, 3, 2, 1};
    const int dups[] = {3, 1, 3, 2, 1, 3};
    const int *cases[] = {sorted, reversed, dups};
    const size_t lens[] = {4, 4, 6};
    for (size_t c = 0; c < 3; c++) {
        int want[6];
        for (size_t i = 0; i < lens[c]; i++) {
            want[i] = cases[c][i];
        }
        qsort(want, lens[c], sizeof(int), cmp_int);
        LinkedList *l = make(cases[c], lens[c]);
        list_sort(l);
        expect(l, want, lens[c]);
        list_destroy(l);
    }

    /* random list, deterministic seed */
    int rnd[500];
    unsigned seed = 12345u;
    for (size_t i = 0; i < 500; i++) {
        seed = seed * 1103515245u + 12345u;
        rnd[i] = (int)((seed >> 16) % 1000u) - 500;
    }
    LinkedList *l = make(rnd, 500);
    list_sort(l);
    qsort(rnd, 500, sizeof(int), cmp_int);
    expect(l, rnd, 500);
    /* tail stays usable after sort */
    assert(list_append(l, 9999));
    check_invariants(l);
    list_destroy(l);

    LinkedList *e = list_create();
    list_sort(e);
    assert(list_is_empty(e));
    list_destroy(e);
    list_sort(NULL);
}

int main(void) {
    test_list_init_empty();
    test_list_append();
    test_list_min_max_positive();
    test_list_min_max_negative();
    test_list_min_max_empty();
    test_list_free_null();
    test_list_free_multiple();
    test_container_prepend_append();
    test_container_insert_at();
    test_container_pop_and_remove();
    test_container_search();
    test_container_reverse();
    test_container_sort();
    printf("All singly linked list tests passed.\n");
    return 0;
}
