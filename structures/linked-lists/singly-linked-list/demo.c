#include "node.h"

#include <stdio.h>

int main(void) {
    node *head = NULL;
    append(1, &head);
    append(2, &head);
    append(3, &head);
    append(4, &head);
    append(5, &head);
    append(6, &head);
    print_list(head);

    int min_val = 0;
    int max_val = 0;
    if (list_min(head, &min_val)) {
        printf("Min: %d\n", min_val);
    }
    if (list_max(head, &max_val)) {
        printf("Max: %d\n", max_val);
    }

    list_free(&head);
    return 0;
}
