#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_BUFFER 2048

static volatile sig_atomic_t flag = 0;
static char name[32];
static int sockfd = 0;

void str_trim_lf(char *arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

void *send_msg_handler(void *arg) {
    (void)arg;
    char message[MAX_BUFFER];
    char buffer[MAX_BUFFER + 32];
    memset(message, 0, sizeof(message));
    memset(buffer, 0, sizeof(buffer));

    while (1) {
        if (fgets(message, MAX_BUFFER, stdin) == NULL) {
            flag = 1;
            break;
        }

        if (strcmp(message, "/exit\n") == 0) {
            flag = 1;
            break;
        } else {
            snprintf(buffer, sizeof(buffer), "%s\n", message);
            send(sockfd, buffer, strlen(buffer), 0);
        }
        memset(message, 0, sizeof(message));
        memset(buffer, 0, sizeof(buffer));
    }
    return NULL;
}

void *receive_msg_handler(void *arg) {
    (void)arg;
    char message[MAX_BUFFER];
    while (1) {
        ssize_t receive = recv(sockfd, message, MAX_BUFFER, 0);
        if (receive > 0) {
            printf("%s", message);
            str_trim_lf(message, (int)receive);
        } else if (receive == 0) {
            break;
        }
        memset(message, 0, sizeof(message));
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <Port>\n", "./client");
        return EXIT_FAILURE;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    printf("Enter your name: ");
    if (fgets(name, 32, stdin) == NULL) {
        return EXIT_FAILURE;
    }
    str_trim_lf(name, (int)strlen(name));

    while (strlen(name) > 30 || strlen(name) < 2) {
        printf("Name must be less than 30 and more than 2 characters.\n");
        printf("Enter your name: ");
        if (fgets(name, 32, stdin) == NULL) {
            return EXIT_FAILURE;
        }
        str_trim_lf(name, (int)strlen(name));
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error: socket failed");
        return EXIT_FAILURE;
    }
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        printf("Error: invalid IP address\n");
        close(sockfd);
        return EXIT_FAILURE;
    }
    server_addr.sin_port = htons((uint16_t)port);

    int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err == -1) {
        printf("Error: connect to server failed\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    send(sockfd, name, 32, 0);
    printf("Connected to server\n");

    pthread_t receive;
    if (pthread_create(&receive, NULL, receive_msg_handler, NULL) != 0) {
        printf("Error: pthread\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    pthread_t send_thread;
    if (pthread_create(&send_thread, NULL, send_msg_handler, NULL) != 0) {
        printf("Error: pthread\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    while (1) {
        if (flag) {
            printf("Bye\n");
            break;
        }
        usleep(10000);
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
