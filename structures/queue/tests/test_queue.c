#undef NDEBUG
#include "queue.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void test_queue_basic(void) {
    Queue *q = queue_create(4);
    assert(q != NULL);
    assert(queue_is_empty(q));
    assert(queue_size(q) == 0);
    assert(queue_capacity(q) == 4);

    int out = 0;
    assert(!queue_dequeue(q, &out));
    assert(!queue_peek(q, &out));

    assert(queue_enqueue(q, 10));
    assert(!queue_is_empty(q));
    assert(queue_size(q) == 1);
    assert(queue_peek(q, &out) && out == 10);

    assert(queue_enqueue(q, 20));
    assert(queue_size(q) == 2);
    assert(queue_peek(q, &out) && out == 10);

    assert(queue_dequeue(q, &out) && out == 10);
    assert(queue_size(q) == 1);
    assert(queue_peek(q, &out) && out == 20);

    assert(queue_dequeue(q, NULL)); /* dequeue without output */
    assert(queue_is_empty(q));
    assert(!queue_dequeue(q, &out));

    queue_destroy(q);
    queue_destroy(NULL);
    printf("PASS: test_queue_basic\n");
}

static void test_queue_circular_wrap(void) {
    Queue *q = queue_create(4);
    assert(q != NULL);

    /* Enqueue 3 elements: [1, 2, 3, _] */
    assert(queue_enqueue(q, 1));
    assert(queue_enqueue(q, 2));
    assert(queue_enqueue(q, 3));

    int out = 0;
    /* Dequeue 2 elements: front moves to index 2, buffer has 1 element [_, _, 3, _] */
    assert(queue_dequeue(q, &out) && out == 1);
    assert(queue_dequeue(q, &out) && out == 2);
    assert(queue_size(q) == 1);

    /* Enqueue 3 elements to trigger circular wrap-around to index 0, 1:
       indices: 0: 5, 1: 6, 2: 3, 3: 4
       size = 4, capacity = 4 */
    assert(queue_enqueue(q, 4));
    assert(queue_enqueue(q, 5));
    assert(queue_enqueue(q, 6));
    assert(queue_size(q) == 4);
    assert(queue_capacity(q) == 4);

    /* Now trigger dynamic resizing while wrap-around is active! */
    assert(queue_enqueue(q, 7));
    assert(queue_size(q) == 5);
    assert(queue_capacity(q) >= 8);

    /* Verify elements come out in strict FIFO order: 3, 4, 5, 6, 7 */
    assert(queue_dequeue(q, &out) && out == 3);
    assert(queue_dequeue(q, &out) && out == 4);
    assert(queue_dequeue(q, &out) && out == 5);
    assert(queue_dequeue(q, &out) && out == 6);
    assert(queue_dequeue(q, &out) && out == 7);
    assert(queue_is_empty(q));

    queue_destroy(q);
    printf("PASS: test_queue_circular_wrap\n");
}

static void test_queue_large_scale(void) {
    const size_t count = 20000;
    Queue *q = queue_create(0);
    assert(q != NULL);

    for (size_t i = 0; i < count; i++) {
        assert(queue_enqueue(q, (int)i));
    }
    assert(queue_size(q) == count);
    assert(queue_capacity(q) >= count);

    int out = 0;
    assert(queue_peek(q, &out) && out == 0);

    for (size_t i = 0; i < count; i++) {
        assert(queue_dequeue(q, &out));
        assert(out == (int)i);
    }
    assert(queue_is_empty(q));

    /* Interleaved enqueue and dequeue simulating sustained circular streaming */
    for (size_t i = 0; i < 50000; i++) {
        assert(queue_enqueue(q, (int)i));
        assert(queue_dequeue(q, &out));
        assert(out == (int)i);
    }
    assert(queue_is_empty(q));

    queue_destroy(q);
    printf("PASS: test_queue_large_scale (20,000 scaling + 50,000 stream)\n");
}

static void test_queue_null_safety(void) {
    int out = 0;
    assert(queue_size(NULL) == 0);
    assert(queue_capacity(NULL) == 0);
    assert(queue_is_empty(NULL) == true);
    assert(!queue_enqueue(NULL, 10));
    assert(!queue_dequeue(NULL, &out));
    assert(!queue_peek(NULL, &out));
    printf("PASS: test_queue_null_safety\n");
}

int main(void) {
    printf("Running queue unit tests...\n");
    test_queue_basic();
    test_queue_circular_wrap();
    test_queue_large_scale();
    test_queue_null_safety();
    printf("All queue tests passed successfully!\n");
    return 0;
}
