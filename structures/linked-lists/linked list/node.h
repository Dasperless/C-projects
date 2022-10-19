#ifndef NODE_H
#define NODE_H

#include <stdio.h>

typedef struct Node{
	int data;
	struct Node *next;
}node;

void append(int data, node **head);
void print_list(node *head);
#endif