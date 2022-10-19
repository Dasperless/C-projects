#include "node.h"
#include <stdlib.h>

void append(int data, node **head){
	node *new_node = (node *) malloc(sizeof(node));
	new_node->data = data;
	new_node->next = NULL;

	if(*head == NULL){
		*head = new_node;
		return;
	}
	
	node *curr_node = *head;
	while (curr_node->next != NULL){ 
		curr_node = curr_node->next;
	}

	curr_node->next = new_node;
	return;
}

void print_list(node *head){
	while(head != NULL){
		printf("[%d] -> %p\n",head->data, head->next);
		head = head->next;
	}
}

int main(){
	node *head = NULL;
	append(1,&head);
	append(2,&head);
	append(3,&head);
	append(4,&head);
	append(5,&head);
	append(6,&head);
	print_list(head);
	
}