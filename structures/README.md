# Data Structures (`structures/`)

Self-contained C11 libraries with unit tests under `tests/`. Every module compiles as a static library with `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror` and runs under ASan + UBSan in CTest.

## Complexity summary

| Structure | Header | Insert / Push | Delete / Pop | Search / Access | Extra |
|---|---|---|---|---|---|
| Singly Linked List (`LinkedList`) | [`node.h`](linked-lists/singly-linked-list/node.h) | prepend/append $O(1)$, at index $O(N)$ | pop front $O(1)$, pop back / at index / by value $O(N)$ | `get_at` / `index_of` $O(N)$ | `list_reverse` $O(N)$, `list_sort` (MergeSort) $O(N \log N)$ |
| Doubly Linked List (`dll_t`) | [`dll.h`](linked-lists/doubly-linked-list/dll.h) | push front/back $O(1)$, insert after/before node $O(1)$ | pop front/back $O(1)$, remove node $O(1)$ | `dll_find` $O(N)$ | `dll_splice` $O(1)$ |
| Dynamic Array (`Vector`) | [`vector.h`](vector/vector.h) | `push_back` $O(1)$ amortized, `insert_at` $O(N)$ | `pop_back` $O(1)$, `remove_at` $O(N)$ | `get` / `set` $O(1)$ | `reserve`, `shrink_to_fit` |
| Stack (`Stack`) | [`stack.h`](stack/stack.h) | `stack_push` $O(1)$ amortized | `stack_pop` $O(1)$ | `stack_peek` $O(1)$ | geometric $2\times$ growth |
| Circular Queue (`Queue`) | [`queue.h`](queue/queue.h) | `queue_enqueue` $O(1)$ amortized | `queue_dequeue` $O(1)$ | `queue_peek` $O(1)$ | wrap-around ring buffer |
| Binary Search Tree (`BST`) | [`bst.h`](binary_search_tree/bst.h) | $O(h)$ ($O(\log N)$ avg, $O(N)$ worst) | $O(h)$ (0/1/2-child successor replacement) | $O(h)$ | in/pre/post-order callbacks, `bst_height` $O(N)$ |
| Hash Table (`HashTable`) | [`hash_table.h`](hash_table/hash_table.h) | $O(1)$ avg (rehashes at $\alpha \ge 0.75$) | $O(1)$ avg | $O(1)$ avg | DJB2 hash, separate chaining, copies string keys |

## Usage examples

### Singly Linked List (`LinkedList`)

```c
#include "node.h"

LinkedList *l = list_create();
list_append(l, 30);
list_prepend(l, 10);
list_insert_at(l, 1, 20);
list_sort(l);      /* top-down MergeSort, O(N log N) */
list_reverse(l);   /* in-place 3-pointer reversal, O(N) */
int front = 0;
list_pop_front(l, &front);
list_destroy(l);
```

### Vector, Stack, and Queue

```c
#include "vector.h"
#include "stack.h"
#include "queue.h"

Vector *v = vector_create(4);
vector_push_back(v, 42);
int x = 0;
vector_get(v, 0, &x);
vector_destroy(v);

Stack *s = stack_create(4);
stack_push(s, 1);
stack_pop(s, &x);
stack_destroy(s);

Queue *q = queue_create(4);
queue_enqueue(q, 99);
queue_dequeue(q, &x);
queue_destroy(q);
```

### Binary Search Tree and Hash Table

```c
#include "bst.h"
#include "hash_table.h"

BST *t = bst_create();
bst_insert(t, 50, 500);
bst_insert(t, 30, 300);
bst_delete(t, 50);
bst_destroy(t);

HashTable *ht = ht_create(8);
ht_insert(ht, "alice", 1);
int id = 0;
if (ht_get(ht, "alice", &id)) { /* id == 1 */ }
ht_destroy(ht);
```

## Running the structure tests

```bash
cmake -B build -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
cmake --build build
ctest --test-dir build --output-on-failure
```
