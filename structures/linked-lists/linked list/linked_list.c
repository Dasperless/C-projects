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

int min(node *head){
	if(head == NULL) return -1;

	int min_value = head->data;
	node* curr_node = head;
	while (curr_node != NULL){
		if(curr_node->data < min_value) min_value = curr_node->data;
		curr_node = curr_node->next;
	}
	return min_value;
}

int max(node* head){
	if(head == NULL) return -1;

	int max_value = head->data;
	node* curr_node = head;
	while (curr_node != NULL){
		if(curr_node->data > max_value) max_value = curr_node->data;
		curr_node = curr_node->next;
	}
	return max_value;
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
	printf("Min: %d\n",min(head));
	printf("Max: %d\n",max(head));
}