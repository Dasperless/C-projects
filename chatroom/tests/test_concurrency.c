#include "test_util.h"

#include <assert.h>
#include <pthread.h>

/* Concurrency, capacity and slot-reuse tests (issue #11), on the framed protocol (issue #12). */

static int port;

typedef struct {
    int id;
    int sock;
} thread_client_t;

static void *client_worker(void *arg) {
    thread_client_t *tc = (thread_client_t *)arg;
    char name[32];
    snprintf(name, sizeof(name), "tester_%d", tc->id);
    tc->sock = tc_login(port, name);
    return NULL;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc != 2) {
        fprintf(stderr, "usage: %s <path-to-chatroom_server>\n", argv[0]);
        return 2;
    }
    signal(SIGPIPE, SIG_IGN);
    port = tc_port(2);
    printf("=== Chatroom Concurrency & Resource Leak Tests ===\n");

    pid_t server_pid = tc_start_server(argv[1], port);
    assert(server_pid > 0);

    printf("[TEST 1] Concurrency: connecting 10 clients in parallel...\n");
    enum { NUM_CONCURRENT = 10 };
    pthread_t threads[NUM_CONCURRENT];
    thread_client_t clients[NUM_CONCURRENT];
    for (int i = 0; i < NUM_CONCURRENT; i++) {
        clients[i].id = i;
        clients[i].sock = -1;
        pthread_create(&threads[i], NULL, client_worker, &clients[i]);
    }
    for (int i = 0; i < NUM_CONCURRENT; i++) {
        pthread_join(threads[i], NULL);
        assert(clients[i].sock >= 0);
    }
    printf("[TEST 1 PASSED]\n");

    printf("[TEST 2] Broadcast: client 0 -> client 1...\n");
    assert(tc_send_text(clients[0].sock, PKT_MSG_PUBLIC, "tester_0", "", "Hello Concurrency"));
    assert(tc_wait_for(clients[1].sock, PKT_MSG_PUBLIC, "tester_0", "Hello Concurrency", 3000));
    printf("[TEST 2 PASSED]\n");

    printf("[TEST 3] Clean disconnect of all 10 clients...\n");
    for (int i = 0; i < NUM_CONCURRENT; i++) {
        tc_send_text(clients[i].sock, PKT_DISCONNECT, "x", "", NULL);
        close(clients[i].sock);
    }
    usleep(200000);
    printf("[TEST 3 PASSED]\n");

    printf("[TEST 4] No lockout: 60 sequential connects/disconnects (capacity 50)...\n");
    for (int i = 0; i < 60; i++) {
        char name[32];
        snprintf(name, sizeof(name), "seq_usr_%d", i);
        int sock = tc_login(port, name);
        assert(sock >= 0);
        tc_send_text(sock, PKT_MSG_PUBLIC, name, "", "Ping");
        tc_send_text(sock, PKT_DISCONNECT, name, "", NULL);
        close(sock);
        usleep(5000);
    }
    printf("[TEST 4 PASSED]\n");

    printf("[TEST 5] Capacity limit and slot reclaim (50 concurrent)...\n");
    usleep(200000);
    int max_socks[50];
    for (int i = 0; i < 50; i++) {
        char name[32];
        snprintf(name, sizeof(name), "cap_%d", i);
        max_socks[i] = tc_login(port, name);
        assert(max_socks[i] >= 0);
    }
    assert(tc_login(port, "cap_overflow") == -1); /* 51st is rejected ("Server full") */

    tc_send_text(max_socks[0], PKT_DISCONNECT, "cap_0", "", NULL);
    close(max_socks[0]);
    usleep(200000);
    int reuse = tc_login(port, "reuse_client");
    assert(reuse >= 0);
    printf("[TEST 5 PASSED]\n");
    close(reuse);
    for (int i = 1; i < 50; i++) {
        close(max_socks[i]);
    }
    usleep(100000);

    tc_stop_server(server_pid);
    printf("=== ALL TESTS PASSED ===\n");
    return 0;
}
