#include "bst.h"

#include <assert.h>
#include <stdio.h>

static int seen[64];
static size_t seen_n;

static void record(int key, int value) {
    assert(value == key * 10);
    seen[seen_n++] = key;
}

static void expect_order(void (*walk)(const BST *, bst_visit_fn), const BST *t, const int *want,
                         size_t n) {
    seen_n = 0;
    walk(t, record);
    assert(seen_n == n);
    for (size_t i = 0; i < n; i++) {
        assert(seen[i] == want[i]);
    }
}

/* Shape:        50
 *            30    70
 *          20 40  60 80
 */
static BST *build(void) {
    BST *t = bst_create();
    assert(t != NULL);
    const int keys[] = {50, 30, 70, 20, 40, 60, 80};
    for (size_t i = 0; i < 7; i++) {
        assert(bst_insert(t, keys[i], keys[i] * 10));
    }
    return t;
}

static void test_empty_and_null(void) {
    BST *t = bst_create();
    int v = 0;
    assert(bst_size(t) == 0 && bst_height(t) == 0);
    assert(!bst_search(t, 1, &v));
    assert(!bst_delete(t, 1));
    seen_n = 0;
    bst_inorder(t, record);
    assert(seen_n == 0);
    bst_destroy(t);
    bst_destroy(NULL);
    assert(!bst_insert(NULL, 1, 1) && !bst_search(NULL, 1, &v) && !bst_delete(NULL, 1));
    assert(bst_height(NULL) == 0 && bst_size(NULL) == 0);
    bst_inorder(NULL, record);
}

static void test_insert_search_update(void) {
    BST *t = build();
    int v = 0;
    assert(bst_size(t) == 7 && bst_height(t) == 3);
    assert(bst_search(t, 60, &v) && v == 600);
    assert(bst_search(t, 60, NULL));
    assert(!bst_search(t, 65, &v));
    assert(bst_insert(t, 60, 1));
    assert(bst_size(t) == 7);
    assert(bst_search(t, 60, &v) && v == 1);
    bst_destroy(t);
}

static void test_traversals(void) {
    BST *t = build();
    const int in[] = {20, 30, 40, 50, 60, 70, 80};
    const int pre[] = {50, 30, 20, 40, 70, 60, 80};
    const int post[] = {20, 40, 30, 60, 80, 70, 50};
    expect_order(bst_inorder, t, in, 7);
    expect_order(bst_preorder, t, pre, 7);
    expect_order(bst_postorder, t, post, 7);
    bst_destroy(t);
}

static void test_delete_cases(void) {
    BST *t = build();
    /* leaf */
    assert(bst_delete(t, 20));
    const int a[] = {30, 40, 50, 60, 70, 80};
    expect_order(bst_inorder, t, a, 6);
    /* one child (30 now has only right child 40) */
    assert(bst_delete(t, 30));
    const int b[] = {40, 50, 60, 70, 80};
    expect_order(bst_inorder, t, b, 5);
    /* two children (70 has 60 and 80) */
    assert(bst_delete(t, 70));
    const int c[] = {40, 50, 60, 80};
    expect_order(bst_inorder, t, c, 4);
    /* root with two children */
    assert(bst_delete(t, 50));
    const int d[] = {40, 60, 80};
    expect_order(bst_inorder, t, d, 3);
    assert(!bst_delete(t, 50));
    assert(bst_size(t) == 3);
    assert(bst_delete(t, 40) && bst_delete(t, 60) && bst_delete(t, 80));
    assert(bst_size(t) == 0 && t->root == NULL);
    bst_destroy(t);
}

static void test_sorted_insert_height(void) {
    BST *t = bst_create();
    for (int i = 0; i < 20; i++) {
        assert(bst_insert(t, i, i * 10));
    }
    assert(bst_height(t) == 20); /* degenerate (unbalanced) tree */
    seen_n = 0;
    bst_inorder(t, record);
    assert(seen_n == 20);
    for (int i = 0; i < 20; i++) {
        assert(seen[i] == i);
    }
    bst_destroy(t);
}

int main(void) {
    test_empty_and_null();
    test_insert_search_update();
    test_traversals();
    test_delete_cases();
    test_sorted_insert_height();
    printf("All BST tests passed.\n");
    return 0;
}
