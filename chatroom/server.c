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

typedef struct {
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[NAME_LEN];
} client_t;

static int client_count = 0;
static int uid_counter = 10;
static client_t *clients[MAX_CLIENTS] = {NULL};
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

static void str_trim_lf(char *arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n' || arr[i] == '\r') {
            arr[i] = '\0';
            break;
        }
    }
}

static void queue_client(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == NULL) {
            clients[i] = cl;
            client_count++;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

static void remove_client(int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->uid == uid) {
            clients[i] = NULL;
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

static void send_message(const char *s, int sender_uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->uid != sender_uid) {
            ssize_t sent = send(clients[i]->sockfd, s, strlen(s), 0);
            (void)sent;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

static void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    char buff_out[BUFFER_SZ];
    char name_buff[NAME_LEN];
    memset(name_buff, 0, sizeof(name_buff));

    /* Receive client username */
    ssize_t r = recv(client->sockfd, name_buff, NAME_LEN - 1, 0);
    if (r <= 0 || strlen(name_buff) < 2 || strlen(name_buff) >= NAME_LEN - 1) {
        printf("Didn't enter valid name, disconnecting.\n");
        close(client->sockfd);
        remove_client(client->uid);
        free(client);
        return NULL;
    }

    str_trim_lf(name_buff, (int)strlen(name_buff));
    strncpy(client->name, name_buff, sizeof(client->name) - 1);
    client->name[sizeof(client->name) - 1] = '\0';

    snprintf(buff_out, sizeof(buff_out), "%s has joined the chat\n", client->name);
    printf("%s", buff_out);
    fflush(stdout);
    send_message(buff_out, client->uid);

    /* Continuous message reception loop */
    while (1) {
        memset(buff_out, 0, sizeof(buff_out));
        ssize_t receive = recv(client->sockfd, buff_out, BUFFER_SZ - 1, 0);
        if (receive > 0) {
            buff_out[receive] = '\0';
            str_trim_lf(buff_out, (int)strlen(buff_out));

            if (strlen(buff_out) == 0) {
                continue;
            }

            if (strcmp(buff_out, "/exit") == 0) {
                break;
            }

            char formatted[BUFFER_SZ + NAME_LEN + 10];
            snprintf(formatted, sizeof(formatted), "%s: %s\n", client->name, buff_out);
            send_message(formatted, client->uid);
            printf("%s: %s\n", client->name, buff_out);
            fflush(stdout);
        } else {
            /* Client disconnected or error occurred */
            break;
        }
    }

    /* Announce departure and clean up */
    snprintf(buff_out, sizeof(buff_out), "%s has left the chat\n", client->name);
    printf("%s", buff_out);
    fflush(stdout);
    send_message(buff_out, client->uid);

    close(client->sockfd);
    remove_client(client->uid);
    free(client);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <Port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    int listefd = socket(AF_INET, SOCK_STREAM, 0);
    if (listefd < 0) {
        perror("Error: socket failed");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(listefd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error: setsockopt(SO_REUSEADDR) failed");
        close(listefd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Error: invalid IP address '%s'\n", ip);
        close(listefd);
        return EXIT_FAILURE;
    }
    server_addr.sin_port = htons((uint16_t)port);

    signal(SIGPIPE, SIG_IGN);

    if (bind(listefd, (struct sockaddr *)&server_addr, (socklen_t)sizeof(server_addr)) < 0) {
        perror("Error: bind failed");
        close(listefd);
        return EXIT_FAILURE;
    }

    if (listen(listefd, 10) < 0) {
        perror("Error: listen failed");
        close(listefd);
        return EXIT_FAILURE;
    }

    printf("Server listening on %s:%d\n", ip, port);
    fflush(stdout);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_size = (socklen_t)sizeof(client_addr);
        int connfd = accept(listefd, (struct sockaddr *)&client_addr, &client_size);
        if (connfd < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("Error: accept failed");
            break;
        }

        pthread_mutex_lock(&clients_mutex);
        if (client_count >= MAX_CLIENTS) {
            pthread_mutex_unlock(&clients_mutex);
            fprintf(stderr, "Max clients reached. Rejected connection.\n");
            close(connfd);
            continue;
        }
        pthread_mutex_unlock(&clients_mutex);

        client_t *newclient = (client_t *)malloc(sizeof(client_t));
        if (!newclient) {
            perror("Error: malloc failed for new client");
            close(connfd);
            continue;
        }
        newclient->address = client_addr;
        newclient->sockfd = connfd;
        newclient->uid = ++uid_counter;
        memset(newclient->name, 0, sizeof(newclient->name));

        queue_client(newclient);

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *)newclient) != 0) {
            perror("Error: pthread_create failed");
            remove_client(newclient->uid);
            close(connfd);
            free(newclient);
            continue;
        }
        pthread_detach(tid);
    }

    close(listefd);
    return EXIT_SUCCESS;
}
