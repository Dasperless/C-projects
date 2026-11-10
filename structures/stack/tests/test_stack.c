#include "stack.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void test_stack_basic(void) {
    Stack *s = stack_create(2);
    assert(s != NULL);
    assert(stack_is_empty(s));
    assert(stack_size(s) == 0);
    assert(stack_capacity(s) == 2);

    int out = 0;
    assert(!stack_pop(s, &out));
    assert(!stack_peek(s, &out));

    assert(stack_push(s, 10));
    assert(!stack_is_empty(s));
    assert(stack_size(s) == 1);
    assert(stack_peek(s, &out) && out == 10);

    assert(stack_push(s, 20));
    assert(stack_size(s) == 2);
    assert(stack_peek(s, &out) && out == 20);

    /* Geometric growth */
    assert(stack_push(s, 30));
    assert(stack_size(s) == 3);
    assert(stack_capacity(s) >= 4);
    assert(stack_peek(s, &out) && out == 30);

    assert(stack_pop(s, &out) && out == 30);
    assert(stack_pop(s, NULL)); /* pop without output */
    assert(stack_pop(s, &out) && out == 10);
    assert(stack_is_empty(s));
    assert(!stack_pop(s, &out));

    stack_destroy(s);
    stack_destroy(NULL);
    printf("PASS: test_stack_basic\n");
}

static void test_stack_large_scale(void) {
    const size_t count = 20000;
    Stack *s = stack_create(0);
    assert(s != NULL);

    for (size_t i = 0; i < count; i++) {
        assert(stack_push(s, (int)i));
    }
    assert(stack_size(s) == count);
    assert(stack_capacity(s) >= count);

    int out = 0;
    assert(stack_peek(s, &out) && out == (int)(count - 1));

    for (size_t i = 0; i < count; i++) {
        assert(stack_pop(s, &out));
        assert(out == (int)(count - 1 - i));
    }
    assert(stack_is_empty(s));

    stack_destroy(s);
    printf("PASS: test_stack_large_scale (20,000 elements)\n");
}

static void test_stack_null_safety(void) {
    int out = 0;
    assert(stack_size(NULL) == 0);
    assert(stack_capacity(NULL) == 0);
    assert(stack_is_empty(NULL) == true);
    assert(!stack_push(NULL, 10));
    assert(!stack_pop(NULL, &out));
    assert(!stack_peek(NULL, &out));
    printf("PASS: test_stack_null_safety\n");
}

int main(void) {
    printf("Running stack unit tests...\n");
    test_stack_basic();
    test_stack_large_scale();
    test_stack_null_safety();
    printf("All stack tests passed successfully!\n");
    return 0;
}
