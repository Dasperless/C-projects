#include "vector.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void test_create_and_destroy(void) {
    Vector *v0 = vector_create(0);
    assert(v0 != NULL);
    assert(vector_size(v0) == 0);
    assert(vector_capacity(v0) == 0);
    assert(vector_is_empty(v0) == true);
    vector_destroy(v0);

    Vector *v10 = vector_create(10);
    assert(v10 != NULL);
    assert(vector_size(v10) == 0);
    assert(vector_capacity(v10) == 10);
    assert(vector_is_empty(v10) == true);
    vector_destroy(v10);

    /* Safe destroy on NULL */
    vector_destroy(NULL);
    printf("PASS: test_create_and_destroy\n");
}

static void test_push_pop(void) {
    Vector *v = vector_create(2);
    assert(v != NULL);

    int out = 0;
    assert(!vector_pop_back(v, &out));

    assert(vector_push_back(v, 42));
    assert(vector_size(v) == 1);
    assert(!vector_is_empty(v));

    assert(vector_push_back(v, 43));
    assert(vector_size(v) == 2);
    assert(vector_capacity(v) >= 2);

    /* Triggers geometric resize */
    assert(vector_push_back(v, 44));
    assert(vector_size(v) == 3);
    assert(vector_capacity(v) >= 4);

    assert(vector_pop_back(v, &out));
    assert(out == 44);
    assert(vector_size(v) == 2);

    /* Pop without saving out value */
    assert(vector_pop_back(v, NULL));
    assert(vector_size(v) == 1);

    assert(vector_pop_back(v, &out));
    assert(out == 42);
    assert(vector_size(v) == 0);
    assert(vector_is_empty(v));

    assert(!vector_pop_back(v, &out));

    vector_destroy(v);
    printf("PASS: test_push_pop\n");
}

static void test_get_set(void) {
    Vector *v = vector_create(4);
    assert(v != NULL);

    for (int i = 0; i < 5; i++) {
        assert(vector_push_back(v, i * 10));
    }

    int val = 0;
    for (size_t i = 0; i < 5; i++) {
        assert(vector_get(v, i, &val));
        assert(val == (int)i * 10);
    }

    /* Out of bounds get */
    assert(!vector_get(v, 5, &val));
    assert(!vector_get(v, 100, &val));
    assert(!vector_get(v, 0, NULL));

    /* Update values */
    assert(vector_set(v, 2, 999));
    assert(vector_get(v, 2, &val));
    assert(val == 999);

    /* Out of bounds set */
    assert(!vector_set(v, 5, 123));

    vector_destroy(v);
    printf("PASS: test_get_set\n");
}

static void test_insert_erase(void) {
    Vector *v = vector_create(0);
    assert(v != NULL);

    /* Insert at index 0 of empty vector */
    assert(vector_insert(v, 0, 10)); /* [10] */
    assert(vector_size(v) == 1);

    /* Insert at end (index == size) */
    assert(vector_insert(v, 1, 30)); /* [10, 30] */
    assert(vector_size(v) == 2);

    /* Insert in middle */
    assert(vector_insert(v, 1, 20)); /* [10, 20, 30] */
    assert(vector_size(v) == 3);

    /* Insert at beginning */
    assert(vector_insert(v, 0, 0)); /* [0, 10, 20, 30] */
    assert(vector_size(v) == 4);

    /* Out of bounds insert */
    assert(!vector_insert(v, 10, 999));

    int val = 0;
    assert(vector_get(v, 0, &val) && val == 0);
    assert(vector_get(v, 1, &val) && val == 10);
    assert(vector_get(v, 2, &val) && val == 20);
    assert(vector_get(v, 3, &val) && val == 30);

    /* Erase middle: index 1 */
    assert(vector_erase(v, 1)); /* [0, 20, 30] */
    assert(vector_size(v) == 3);
    assert(vector_get(v, 1, &val) && val == 20);

    /* Erase beginning: index 0 */
    assert(vector_erase(v, 0)); /* [20, 30] */
    assert(vector_size(v) == 2);
    assert(vector_get(v, 0, &val) && val == 20);

    /* Erase end: index 1 */
    assert(vector_erase(v, 1)); /* [20] */
    assert(vector_size(v) == 1);

    /* Out of bounds erase */
    assert(!vector_erase(v, 1));
    assert(!vector_erase(v, 10));

    /* Erase last element */
    assert(vector_erase(v, 0));
    assert(vector_size(v) == 0);
    assert(vector_is_empty(v));
    assert(!vector_erase(v, 0));

    vector_destroy(v);
    printf("PASS: test_insert_erase\n");
}

static void test_clear_and_shrink(void) {
    Vector *v = vector_create(16);
    assert(v != NULL);

    for (int i = 0; i < 10; i++) {
        assert(vector_push_back(v, i));
    }
    assert(vector_size(v) == 10);
    assert(vector_capacity(v) >= 16);

    assert(vector_shrink_to_fit(v));
    assert(vector_size(v) == 10);
    assert(vector_capacity(v) == 10);

    vector_clear(v);
    assert(vector_size(v) == 0);
    assert(vector_is_empty(v));
    assert(vector_capacity(v) == 10);

    assert(vector_shrink_to_fit(v));
    assert(vector_capacity(v) == 0);
    assert(vector_size(v) == 0);

    /* Push after shrinking to 0 */
    assert(vector_push_back(v, 100));
    assert(vector_size(v) == 1);
    int val = 0;
    assert(vector_get(v, 0, &val) && val == 100);

    vector_destroy(v);
    printf("PASS: test_clear_and_shrink\n");
}

static void test_large_scale(void) {
    const size_t count = 20000;
    Vector *v = vector_create(0);
    assert(v != NULL);

    for (size_t i = 0; i < count; i++) {
        assert(vector_push_back(v, (int)i));
    }
    assert(vector_size(v) == count);
    assert(vector_capacity(v) >= count);

    for (size_t i = 0; i < count; i++) {
        int val = 0;
        assert(vector_get(v, i, &val));
        assert(val == (int)i);
    }

    for (size_t i = 0; i < count; i++) {
        int val = 0;
        assert(vector_pop_back(v, &val));
        assert(val == (int)(count - 1 - i));
    }
    assert(vector_size(v) == 0);

    vector_destroy(v);
    printf("PASS: test_large_scale (20,000 elements)\n");
}

static void test_null_safety(void) {
    int out = 0;
    assert(vector_size(NULL) == 0);
    assert(vector_capacity(NULL) == 0);
    assert(vector_is_empty(NULL) == true);
    assert(!vector_push_back(NULL, 10));
    assert(!vector_pop_back(NULL, &out));
    assert(!vector_get(NULL, 0, &out));
    assert(!vector_set(NULL, 0, 10));
    assert(!vector_insert(NULL, 0, 10));
    assert(!vector_erase(NULL, 0));
    assert(!vector_shrink_to_fit(NULL));
    vector_clear(NULL);
    printf("PASS: test_null_safety\n");
}

int main(void) {
    printf("Running vector unit tests...\n");
    test_create_and_destroy();
    test_push_pop();
    test_get_set();
    test_insert_erase();
    test_clear_and_shrink();
    test_large_scale();
    test_null_safety();
    printf("All vector tests passed successfully!\n");
    return 0;
}
