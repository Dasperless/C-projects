#include "singly_linked_list.h"

#include <stdio.h>

int main(void) {
    /* Demonstrate Node-level API */
    printf("=== Singly Linked List: Node API ===\n");
    Node *head = NULL;
    list_append_node(&head, 1);
    list_append_node(&head, 2);
    list_append_node(&head, 3);
    list_append_node(&head, 4);
    list_append_node(&head, 5);
    list_append_node(&head, 6);
    print_list_nodes(head);

    int min_val = 0;
    int max_val = 0;
    if (list_min_node(head, &min_val)) {
        printf("Min: %d\n", min_val);
    }
    if (list_max_node(head, &max_val)) {
        printf("Max: %d\n", max_val);
    }
    list_free_nodes(&head);

    /* Demonstrate LinkedList Container API */
    printf("\n=== Singly Linked List: Container API ===\n");
    LinkedList *list = list_create();
    list_append(list, 50);
    list_prepend(list, 20);
    list_append(list, 10);
    list_insert_at(list, 1, 30);
    printf("Original list:\n");
    list_print(list);

    printf("Sorting list...\n");
    list_sort(list);
    list_print(list);

    printf("Reversing list...\n");
    list_reverse(list);
    list_print(list);

    if (list_min(list, &min_val)) {
        printf("Container Min: %d\n", min_val);
    }
    if (list_max(list, &max_val)) {
        printf("Container Max: %d\n", max_val);
    }

    list_destroy(list);
    return 0;
}
