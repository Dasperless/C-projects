#include "node.h"

#include <stdbool.h>
#include <stdio.h>
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

static void test_empty_list_operations(void) {
    node *head = NULL;
    int val = 42;

    /* Querying empty list should return false and leave output untouched */
    ASSERT_FALSE(list_min(head, &val));
    ASSERT_EQ(val, 42);

    ASSERT_FALSE(list_max(head, &val));
    ASSERT_EQ(val, 42);

    /* min and max legacy return -1 */
    ASSERT_EQ(min(head), -1);
    ASSERT_EQ(max(head), -1);

    /* list_free on NULL pointer or NULL *head should be safe */
    list_free(NULL);
    list_free(&head);
    free_list(NULL);
    free_list(&head);
    ASSERT_TRUE(head == NULL);
}

static void test_sequential_append(void) {
    node *head = NULL;

    ASSERT_TRUE(list_append_node(&head, 10));
    ASSERT_TRUE(list_append_node(&head, 20));
    ASSERT_TRUE(list_append_node(&head, 30));

    ASSERT_TRUE(head != NULL);
    ASSERT_EQ(head->data, 10);
    ASSERT_TRUE(head->next != NULL);
    ASSERT_EQ(head->next->data, 20);
    ASSERT_TRUE(head->next->next != NULL);
    ASSERT_EQ(head->next->next->data, 30);
    ASSERT_TRUE(head->next->next->next == NULL);

    list_free(&head);
    ASSERT_TRUE(head == NULL);
}

static void test_min_max_calculations(void) {
    node *head = NULL;
    int min_val = 0;
    int max_val = 0;

    /* Negative numbers */
    append(-100, &head);
    append(-50, &head);
    append(-1, &head);

    ASSERT_TRUE(list_min(head, &min_val));
    ASSERT_EQ(min_val, -100);

    ASSERT_TRUE(list_max(head, &max_val));
    ASSERT_EQ(max_val, -1);

    ASSERT_EQ(min(head), -100);
    ASSERT_EQ(max(head), -1);

    list_free(&head);
    ASSERT_TRUE(head == NULL);

    /* Single element */
    append(77, &head);
    ASSERT_TRUE(list_min(head, &min_val));
    ASSERT_EQ(min_val, 77);
    ASSERT_TRUE(list_max(head, &max_val));
    ASSERT_EQ(max_val, 77);

    list_free(&head);
    ASSERT_TRUE(head == NULL);

    /* Mixed positive and negative */
    append(5, &head);
    append(-10, &head);
    append(0, &head);
    append(42, &head);
    append(-10, &head);

    ASSERT_TRUE(list_min(head, &min_val));
    ASSERT_EQ(min_val, -10);

    ASSERT_TRUE(list_max(head, &max_val));
    ASSERT_EQ(max_val, 42);

    list_free(&head);
    ASSERT_TRUE(head == NULL);
}

static void test_duplicate_values(void) {
    node *head = NULL;
    int min_val = 0;
    int max_val = 0;

    append(4, &head);
    append(4, &head);
    append(4, &head);

    ASSERT_TRUE(list_min(head, &min_val));
    ASSERT_EQ(min_val, 4);
    ASSERT_TRUE(list_max(head, &max_val));
    ASSERT_EQ(max_val, 4);

    list_free(&head);
    ASSERT_TRUE(head == NULL);
}

static void test_double_free_safety(void) {
    node *head = NULL;
    append(1, &head);
    append(2, &head);

    list_free(&head);
    ASSERT_TRUE(head == NULL);

    /* Calling list_free second time should do nothing and not crash */
    list_free(&head);
    ASSERT_TRUE(head == NULL);
}

static void test_null_guards(void) {
    /* append with NULL head */
    ASSERT_FALSE(list_append_node(NULL, 10));

    /* min / max with NULL out pointers */
    node *head = NULL;
    append(10, &head);

    ASSERT_FALSE(list_min(head, NULL));
    ASSERT_FALSE(list_max(head, NULL));

    list_free(&head);
    ASSERT_TRUE(head == NULL);
}

int main(void) {
    printf("Running singly linked list unit tests...\n");

    test_empty_list_operations();
    test_sequential_append();
    test_min_max_calculations();
    test_duplicate_values();
    test_double_free_safety();
    test_null_guards();

    printf("Tests run: %d, Failures: %d\n", g_tests_run, g_tests_failed);
    return (g_tests_failed == 0) ? 0 : 1;
}
