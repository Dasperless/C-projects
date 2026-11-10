# Data Structures Subsystem

This module contains data structures implemented in C conforming to C11 standards.

## Modules
- `linked-lists/singly-linked-list/`: Singly Linked List with memory safety and CRUD algorithms.
- `vector/`: Dynamic array with geometric 2x growth.
- `stack/`: LIFO Stack ADT.
- `queue/`: FIFO Queue circular ring buffer.
- `linked-lists/doubly-linked-list/`: Doubly Linked List with bidirectional traversal.
- `trees/bst/`: Binary Search Tree with in-order/pre-order/post-order traversals.
- `hash-table/`: Key-value hash table with dynamic rehashing.

## Complexity Table
| Data Structure | Access | Search | Insertion | Deletion |
|----------------|:------:|:------:|:---------:|:--------:|
| Singly Linked List | O(n) | O(n) | O(1) front | O(n) |
| Dynamic Vector | O(1) | O(n) | O(1) amortized | O(1) back |
| Stack | O(n) | O(n) | O(1) | O(1) |
| Queue (Ring Buffer) | O(1) | O(n) | O(1) | O(1) |
| Doubly Linked List | O(n) | O(n) | O(1) front/back | O(1) |
| BST (Balanced) | O(log n) | O(log n) | O(log n) | O(log n) |
| Hash Table | - | O(1) avg | O(1) avg | O(1) avg |
