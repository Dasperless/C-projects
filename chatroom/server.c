#include <sys/un.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 50
#define BUFFER_SZ 2048
#define NAME_LEN 32

static int client_count = 0;

typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[NAME_LEN];
} client_t;

client_t clients[MAX_CLIENTS];

/**
 * @brief Add client to queue
 * 
 * @param newclient newclient pointer
 */
void queue_client(client_t *newclient){
	for(int i = 0; i < MAX_CLIENTS; i++){
		if(!clients[i].sockfd){
			clients[i] = *newclient;
			break;
		}
	}
}

void *handle_client(void *arg){
	client_count++;
	client_t *client = (client_t *)arg; // cast to client_t
	
	char buff_out[BUFFER_SZ];			// buffer for output
	int leave_flag = 0;					// flag to indicate if client is leaving

	// receive name
	if(recv(client->sockfd, client->name, NAME_LEN, 0) <= 0 || strlen(client->name) < 2 || strlen(client->name) >= NAME_LEN - 1){
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	} else {
		strcpy(client->name, buff_out);
		sprintf(buff_out, "%s has joined to the chat\n", client->name);
		printf("%s", buff_out);
	}

	bzero(buff_out, BUFFER_SZ);


	





}

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage: %s <IP> <Port>\n",  "./server");
		return EXIT_FAILURE;
	}

	char* ip = argv[1];
	int port = atoi(argv[2]);

	// structures and variables for socket
	int listefd = 0, connfd =0;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	pthread_t tid;

	// create socket
	listefd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	signal(SIGPIPE, SIG_IGN);

	// bind socket
	if(bind(listefd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		printf("Error: bind failed");
		return EXIT_FAILURE;
	}

	// listen
	if(listen(listefd, 10) < 0) {
		printf("Error: listen failed");
		return EXIT_FAILURE;
	}
	
	printf("server listen on %s:%d\n", ip, port);

	while (1){
		int client_size = sizeof(client_addr);

		// accept
		connfd = accept(listefd, (struct sockaddr*)&client_addr, &client_size);

		if((client_count + 1) == MAX_CLIENTS) {
			printf("Max clients reached. Rejected: ");
			close(connfd);
			continue;
		}

		// new client
		client_t *newclient = (client_t*)malloc(sizeof(client_t));
		newclient->address = client_addr;
		newclient->sockfd = connfd;
		newclient->uid = client_count;

		queue_client(newclient); // add client to queue

		pthread_create(&tid, NULL, &handle_client, (void*)newclient);
		sleep(1);
		
	}
	return EXIT_SUCCESS;	
}