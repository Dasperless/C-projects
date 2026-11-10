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
#define NAME_LEN 32

static volatile sig_atomic_t flag = 0;
static char name[NAME_LEN];
static int sockfd = 0;
static pthread_t send_thread;
static pthread_t recv_thread;

static void str_trim_lf(char *arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n' || arr[i] == '\r') {
            arr[i] = '\0';
            break;
        }
    }
}

static void catch_sigint(int sig) {
    (void)sig;
    flag = 1;
    if (sockfd > 0) {
        shutdown(sockfd, SHUT_RDWR);
    }
}

static void *send_msg_handler(void *arg) {
    (void)arg;
    char message[MAX_BUFFER];
    char buffer[MAX_BUFFER + NAME_LEN];

    while (!flag) {
        memset(message, 0, sizeof(message));
        memset(buffer, 0, sizeof(buffer));

        if (fgets(message, (int)sizeof(message), stdin) == NULL) {
            flag = 1;
            break;
        }

        str_trim_lf(message, (int)strlen(message));

        if (strcmp(message, "/exit") == 0) {
            flag = 1;
            break;
        }

        if (strlen(message) > 0) {
            snprintf(buffer, sizeof(buffer), "%s\n", message);
            ssize_t sent = send(sockfd, buffer, strlen(buffer), 0);
            if (sent <= 0) {
                flag = 1;
                break;
            }
        }
    }

    /* Signal receive thread and socket */
    if (sockfd > 0) {
        shutdown(sockfd, SHUT_RDWR);
    }
    return NULL;
}

static void *receive_msg_handler(void *arg) {
    (void)arg;
    char message[MAX_BUFFER];

    while (!flag) {
        memset(message, 0, sizeof(message));
        ssize_t receive = recv(sockfd, message, sizeof(message) - 1, 0);
        if (receive > 0) {
            message[receive] = '\0';
            printf("%s", message);
            fflush(stdout);
        } else if (receive == 0) {
            /* Server disconnected */
            flag = 1;
            break;
        } else {
            /* Error or socket closed by shutdown */
            flag = 1;
            break;
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <Port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = catch_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    printf("Enter your name: ");
    fflush(stdout);
    if (fgets(name, (int)sizeof(name), stdin) == NULL) {
        return EXIT_FAILURE;
    }
    str_trim_lf(name, (int)strlen(name));

    while (strlen(name) >= (NAME_LEN - 1) || strlen(name) < 2) {
        printf("Name must be between 2 and %d characters.\nEnter your name: ", NAME_LEN - 2);
        fflush(stdout);
        if (fgets(name, (int)sizeof(name), stdin) == NULL) {
            return EXIT_FAILURE;
        }
        str_trim_lf(name, (int)strlen(name));
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Error: invalid IP address '%s'\n", ip);
        return EXIT_FAILURE;
    }
    server_addr.sin_port = htons((uint16_t)port);

    /* Socket settings - fix variable shadowing of sockfd */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error: socket creation failed");
        return EXIT_FAILURE;
    }

    /* Connect to server */
    int err = connect(sockfd, (struct sockaddr *)&server_addr, (socklen_t)sizeof(server_addr));
    if (err == -1) {
        perror("Error: connect to server failed");
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* Send username */
    ssize_t sent = send(sockfd, name, strlen(name), 0);
    if (sent <= 0) {
        perror("Error: failed to send username");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("=== Connected to server ===\n");
    fflush(stdout);

    /* Create threads */
    if (pthread_create(&recv_thread, NULL, receive_msg_handler, NULL) != 0) {
        perror("Error: pthread_create for receive failed");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (pthread_create(&send_thread, NULL, send_msg_handler, NULL) != 0) {
        perror("Error: pthread_create for send failed");
        flag = 1;
        pthread_join(recv_thread, NULL);
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* Clean thread synchronization: eliminate CPU spin-loop */
    pthread_join(send_thread, NULL);

    /* If send_thread ended first, ensure recv_thread also joins */
    pthread_join(recv_thread, NULL);

    printf("\nDisconnected from server.\n");
    close(sockfd);
    return EXIT_SUCCESS;
}