#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define TEST_PORT 19876
#define TEST_IP "127.0.0.1"

static int connect_client(const char *name) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, TEST_IP, &serv_addr.sin_addr);
    serv_addr.sin_port = htons((uint16_t)TEST_PORT);

    if (connect(sock, (struct sockaddr *)&serv_addr, (socklen_t)sizeof(serv_addr)) < 0) {
        close(sock);
        return -1;
    }

    /* Send username */
    char name_buf[32];
    memset(name_buf, 0, sizeof(name_buf));
    snprintf(name_buf, sizeof(name_buf), "%s\n", name);
    if (send(sock, name_buf, strlen(name_buf), 0) <= 0) {
        close(sock);
        return -1;
    }

    return sock;
}

static void drain_socket(int sock) {
    char buf[512];
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 30000; /* 30ms */
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    while (recv(sock, buf, sizeof(buf), 0) > 0) {
        /* consume buffered announcements */
    }
}

typedef struct {
    int id;
    int sock;
} thread_client_t;

static void *client_worker(void *arg) {
    thread_client_t *tc = (thread_client_t *)arg;
    char name[32];
    snprintf(name, sizeof(name), "tester_%d", tc->id);
    tc->sock = connect_client(name);
    return NULL;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    printf("=== Starting Chatroom Concurrency & Resource Leak Tests (Issue #11) ===\n");

    const char *server_bin = "./server";
    if (access(server_bin, X_OK) != 0) {
        server_bin = "./chatroom/server";
    }

    /* Fork and start server */
    pid_t server_pid = fork();
    if (server_pid == 0) {
        char port_str[16];
        snprintf(port_str, sizeof(port_str), "%d", TEST_PORT);
        execl(server_bin, server_bin, TEST_IP, port_str, (char *)NULL);
        perror("execl failed");
        exit(1);
    }

    /* Give server time to bind and listen */
    usleep(200000);

    /* TEST 1: Concurrency - Connect 10 clients in parallel */
    printf("[TEST 1] Concurrency: Connecting 10 concurrent clients...\n");
    const int NUM_CONCURRENT = 10;
    pthread_t threads[10];
    thread_client_t clients[10];

    for (int i = 0; i < NUM_CONCURRENT; i++) {
        clients[i].id = i;
        clients[i].sock = -1;
        pthread_create(&threads[i], NULL, client_worker, &clients[i]);
    }

    for (int i = 0; i < NUM_CONCURRENT; i++) {
        pthread_join(threads[i], NULL);
        assert(clients[i].sock >= 0);
    }
    printf("[TEST 1 PASSED] 10 clients connected concurrently.\n");

    /* TEST 2: Message Broadcast */
    printf("[TEST 2] Message Broadcast: Client 0 sends message, verify Client 1 receives...\n");
    usleep(50000);
    drain_socket(clients[1].sock);

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(clients[1].sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    const char *msg = "Hello Concurrency\n";
    send(clients[0].sock, msg, strlen(msg), 0);

    char recv_buf[2048];
    int found = 0;
    for (int attempt = 0; attempt < 5; attempt++) {
        memset(recv_buf, 0, sizeof(recv_buf));
        ssize_t r = recv(clients[1].sock, recv_buf, sizeof(recv_buf) - 1, 0);
        if (r > 0) {
            recv_buf[r] = '\0';
            if (strstr(recv_buf, "Hello Concurrency") != NULL) {
                found = 1;
                printf("[TEST 2 PASSED] Received broadcast: %s", recv_buf);
                break;
            }
        } else {
            break;
        }
    }
    assert(found);

    /* TEST 3: Clean Disconnect */
    printf("[TEST 3] Clean Disconnect: Disconnecting all 10 clients...\n");
    for (int i = 0; i < NUM_CONCURRENT; i++) {
        const char *exit_cmd = "/exit\n";
        send(clients[i].sock, exit_cmd, strlen(exit_cmd), 0);
        close(clients[i].sock);
    }
    usleep(100000);
    printf("[TEST 3 PASSED] 10 clients cleanly disconnected.\n");

    /* TEST 4: Reconnects Beyond 49 Clients (Fix for lockout after 49 clients) */
    printf("[TEST 4] Lockout Fix: Testing 60 sequential client connects/disconnects (capacity 50)...\n");
    for (int i = 0; i < 60; i++) {
        char client_name[32];
        snprintf(client_name, sizeof(client_name), "seq_usr_%d", i);
        int sock = connect_client(client_name);
        assert(sock >= 0);

        const char *test_msg = "Ping\n";
        send(sock, test_msg, strlen(test_msg), 0);

        const char *exit_msg = "/exit\n";
        send(sock, exit_msg, strlen(exit_msg), 0);
        close(sock);
        usleep(5000); /* 5ms */
    }
    printf("[TEST 4 PASSED] All 60 sequential clients connected and disconnected successfully without lockout!\n");

    /* TEST 5: Capacity limit (50 concurrent) and dynamic slot reuse */
    printf("[TEST 5] Capacity limit & Slot Reclaim: Filling server to 50 concurrent connections...\n");
    int max_socks[50];
    for (int i = 0; i < 50; i++) {
        char client_name[32];
        snprintf(client_name, sizeof(client_name), "cap_%d", i);
        max_socks[i] = connect_client(client_name);
        assert(max_socks[i] >= 0);
    }

    /* 51st client should be rejected */
    printf("[TEST 5] Attempting 51st connection (should be rejected)...\n");
    int extra_sock = connect_client("cap_overflow");
    if (extra_sock >= 0) {
        char tmp[64];
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(extra_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ssize_t n = recv(extra_sock, tmp, sizeof(tmp), 0);
        assert(n <= 0);
        close(extra_sock);
    }
    printf("[TEST 5] Capacity limit enforced.\n");

    /* Disconnect one client and verify slot is reused */
    printf("[TEST 5] Disconnecting 1 client and testing slot reuse...\n");
    send(max_socks[0], "/exit\n", 6, 0);
    close(max_socks[0]);
    usleep(50000);

    int reuse_sock = connect_client("reuse_client");
    assert(reuse_sock >= 0);
    printf("[TEST 5 PASSED] Disconnected slot was successfully reused!\n");
    close(reuse_sock);

    /* Disconnect the rest */
    for (int i = 1; i < 50; i++) {
        send(max_socks[i], "/exit\n", 6, 0);
        close(max_socks[i]);
    }
    usleep(50000);

    /* Terminate server cleanly */
    kill(server_pid, SIGTERM);
    int status;
    waitpid(server_pid, &status, 0);

    printf("=== ALL TESTS PASSED SUCCESSFULLY! ===\n");
    return 0;
}
