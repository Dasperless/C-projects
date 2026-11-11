#define _POSIX_C_SOURCE 200809L
#include "protocol.h"

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

static volatile sig_atomic_t flag = 0;
static int sockfd = 0;
static char name[PROTO_NAME_LEN];

static void str_trim_lf(char *arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n' || arr[i] == '\r') {
            arr[i] = '\0';
            break;
        }
    }
}

static void print_help(void) {
    printf("Commands:\n"
           "  <text>              send to everyone\n"
           "  /msg <user> <text>  private message\n"
           "  /group <name>       join a group channel\n"
           "  #<name> <text>      message a group you joined\n"
           "  /leave <name>       leave a group\n"
           "  /users              list connected users\n"
           "  /help               show this help\n"
           "  /exit               disconnect\n");
    fflush(stdout);
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
    char line[PROTO_MAX_PAYLOAD + 64];

    while (!flag) {
        if (fgets(line, (int)sizeof(line), stdin) == NULL) {
            break;
        }
        parsed_input_t in;
        proto_parse_input(line, &in);
        if (in.action == INPUT_EXIT) {
            break;
        }
        if (in.action == INPUT_HELP) {
            print_help();
        } else if (in.action == INPUT_ERROR) {
            printf("! %s\n", in.error);
            fflush(stdout);
        } else if (in.action == INPUT_SEND) {
            if (!packet_send(sockfd, in.type, name, in.target, in.text, strlen(in.text))) {
                break;
            }
        }
    }

    flag = 1;
    (void)packet_send(sockfd, PKT_DISCONNECT, name, NULL, NULL, 0);
    shutdown(sockfd, SHUT_RDWR);
    return NULL;
}

static void *receive_msg_handler(void *arg) {
    (void)arg;
    packet_header_t hdr;
    char payload[PROTO_MAX_PAYLOAD + 1];

    while (!flag) {
        if (packet_recv(sockfd, &hdr, payload) != 1) {
            break;
        }
        switch (hdr.type) {
            case PKT_MSG_PUBLIC:
                printf("%s: %s\n", hdr.sender, payload);
                break;
            case PKT_MSG_PRIVATE:
                printf("[private from %s] %s\n", hdr.sender, payload);
                break;
            case PKT_MSG_GROUP:
                printf("[#%s] %s: %s\n", hdr.recipient_or_group, hdr.sender, payload);
                break;
            case PKT_SERVER_ERROR:
                printf("! %s\n", payload);
                break;
            default:
                printf("* %s\n", payload);
                break;
        }
        fflush(stdout);
    }

    flag = 1;
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
    signal(SIGPIPE, SIG_IGN);

    printf("Enter your name: ");
    fflush(stdout);
    if (fgets(name, (int)sizeof(name), stdin) == NULL) {
        return EXIT_FAILURE;
    }
    str_trim_lf(name, (int)strlen(name));

    while (!proto_valid_name(name)) {
        printf("Name must be 2-%d characters of letters, digits, '_', '-' or '.'.\nEnter your name: ",
               PROTO_NAME_LEN - 2);
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

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error: socket creation failed");
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, (socklen_t)sizeof(server_addr)) == -1) {
        perror("Error: connect to server failed");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (!packet_send(sockfd, PKT_HANDSHAKE, name, NULL, NULL, 0)) {
        perror("Error: failed to send handshake");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("=== Connected to server ===\n");
    fflush(stdout);

    pthread_t recv_thread;
    pthread_t send_thread;
    if (pthread_create(&recv_thread, NULL, receive_msg_handler, NULL) != 0) {
        perror("Error: pthread_create for receive failed");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (pthread_create(&send_thread, NULL, send_msg_handler, NULL) != 0) {
        perror("Error: pthread_create for send failed");
        flag = 1;
        shutdown(sockfd, SHUT_RDWR);
        pthread_join(recv_thread, NULL);
        close(sockfd);
        return EXIT_FAILURE;
    }
    /* The send thread may be blocked in fgets() if the server hung up; do not wait for it. */
    pthread_detach(send_thread);

    pthread_join(recv_thread, NULL);

    printf("\nDisconnected from server.\n");
    close(sockfd);
    return EXIT_SUCCESS;
}
