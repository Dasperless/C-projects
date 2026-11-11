#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CLIENTS 50
#define BUFFER_SZ 2048
#define NAME_LEN 32

static int client_count = 0;

typedef struct {
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[NAME_LEN];
} client_t;

static client_t clients[MAX_CLIENTS];

/**
 * @brief Add client to queue
 *
 * @param newclient newclient pointer
 */
void queue_client(client_t *newclient) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].sockfd) {
            clients[i] = *newclient;
            break;
        }
    }
}

void *handle_client(void *arg) {
    client_count++;
    client_t *client = (client_t *)arg;

    char buff_out[BUFFER_SZ];
    int leave_flag = 0;

    ssize_t r = recv(client->sockfd, buff_out, NAME_LEN, 0);
    if (r <= 0 || strlen(buff_out) < 2 || strlen(buff_out) >= NAME_LEN - 1) {
        printf("Didn't enter the name.\n");
        leave_flag = 1;
    } else {
        strncpy(client->name, buff_out, sizeof(client->name) - 1);
        client->name[sizeof(client->name) - 1] = '\0';
        snprintf(buff_out, sizeof(buff_out), "%s has joined to the chat\n", client->name);
        printf("%s", buff_out);
    }

    memset(buff_out, 0, BUFFER_SZ);
    if (leave_flag) {
        close(client->sockfd);
        free(client);
        client_count--;
        return NULL;
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <Port>\n", "./server");
        return EXIT_FAILURE;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    int listefd = 0;
    int connfd = 0;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    pthread_t tid;

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    listefd = socket(AF_INET, SOCK_STREAM, 0);
    if (listefd < 0) {
        perror("Error: socket failed");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        printf("Error: invalid IP address\n");
        close(listefd);
        return EXIT_FAILURE;
    }
    server_addr.sin_port = htons((uint16_t)port);

    signal(SIGPIPE, SIG_IGN);

    if (bind(listefd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error: bind failed");
        close(listefd);
        return EXIT_FAILURE;
    }

    if (listen(listefd, 10) < 0) {
        perror("Error: listen failed");
        close(listefd);
        return EXIT_FAILURE;
    }

    printf("server listen on %s:%d\n", ip, port);

    while (1) {
        socklen_t client_size = sizeof(client_addr);

        connfd = accept(listefd, (struct sockaddr *)&client_addr, &client_size);
        if (connfd < 0) {
            perror("Error: accept failed");
            continue;
        }

        if ((client_count + 1) == MAX_CLIENTS) {
            printf("Max clients reached. Rejected.\n");
            close(connfd);
            continue;
        }

        client_t *newclient = (client_t *)malloc(sizeof(client_t));
        if (newclient == NULL) {
            close(connfd);
            continue;
        }
        newclient->address = client_addr;
        newclient->sockfd = connfd;
        newclient->uid = client_count;

        queue_client(newclient);

        if (pthread_create(&tid, NULL, handle_client, (void *)newclient) != 0) {
            perror("Error: pthread_create failed");
            free(newclient);
            close(connfd);
            continue;
        }
        sleep(1);
    }

    close(listefd);
    return EXIT_SUCCESS;
}
