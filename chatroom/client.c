#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_BUFFER 2048

volatile sig_atomic_t flag = 0;
char name [32];
int sockfd = 0;

void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim the \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void send_msg_handler() {
	char message[MAX_BUFFER] = {};
	char buffer[MAX_BUFFER + 32] = {};

	while (1){
		fgets(message, MAX_BUFFER, stdin);

		if(strcmp(message, "/exit\n") == 0){
			flag = 1;
			break;
		} else {
			sprintf(buffer,"%s\n", message);
			send(sockfd, buffer, strlen(buffer), 0);
		}
		bzero(message, MAX_BUFFER);
		bzero(buffer, MAX_BUFFER + 32);
	}
		
}

void receive_msg_handler() {
	char message[MAX_BUFFER];
	while (1) {
		int receive = recv(sockfd, message, MAX_BUFFER, 0);
		if (receive > 0) {
			printf("%s", message);
			str_trim_lf(message, MAX_BUFFER);
		} else if (receive == 0) {
			break;
		} 
		memset(message, 0, sizeof(message));
	}
}

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage: %s <IP> <Port>\n", "./client");
		return EXIT_FAILURE;
	}

	char *ip = argv[1];
	int port = atoi(argv[2]);

	printf("Enter your name: ");
	fgets(name, 32, stdin);
	str_trim_lf(name, strlen(name));

	while (strlen(name) > 32 || strlen(name) < 2) {
		printf("Name must be less than 30 and more than 2 characters.\n");
		printf("Enter your name: ");
		fgets(name, 32, stdin);
		str_trim_lf(name, strlen(name));
	}

	struct sockaddr_in server_addr;

	// socket settings
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	// connect to server
	int err = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (err == -1) {
		printf("Error: connect to server failed\n");
		return EXIT_FAILURE;
	}

	// send name
	send(sockfd, name, 32, 0);
	printf("Connected to server\n");

	// Create a thread to receive messages
	pthread_t receive;
	if (pthread_create(&receive, NULL, (void *)receive_msg_handler, NULL) != 0) {
		printf("Error: pthread\n");
		return EXIT_FAILURE;
	}

	// thread to send messages
	pthread_t send;
	if (pthread_create(&send, NULL, (void *)send_msg_handler, NULL) != 0) {
		printf("Error: pthread\n");
		return EXIT_FAILURE;
	}

	while(1){
		if(flag){
			printf("Bye");
			break;
		}
	}

	close(sockfd);
	return EXIT_SUCCESS;
}