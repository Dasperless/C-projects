#include "test_util.h"

#include <assert.h>

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);                      \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

static int port;

static void test_handshake_and_names(void) {
    int a = tc_login(port, "alice");
    CHECK(a >= 0);
    CHECK(tc_login(port, "alice") == -1); /* duplicate */
    CHECK(tc_login(port, "bad name") == -1);
    CHECK(tc_login(port, "x") == -1);
    close(a);
}

static void test_users_list(void) {
    int a = tc_login(port, "alice");
    int b = tc_login(port, "bob");
    int c = tc_login(port, "carol");
    CHECK(a >= 0 && b >= 0 && c >= 0);
    CHECK(tc_send_text(a, PKT_CMD_USERS, "alice", "", NULL));
    CHECK(tc_wait_for(a, PKT_SERVER_INFO, "server", "Connected users (3)", 2000));
    CHECK(tc_send_text(a, PKT_CMD_USERS, "alice", "", NULL));
    CHECK(tc_wait_for(a, PKT_SERVER_INFO, "server", "bob", 2000));
    close(a);
    close(b);
    close(c);
}

static void test_public_broadcast(void) {
    int a = tc_login(port, "alice");
    int b = tc_login(port, "bob");
    int c = tc_login(port, "carol");
    CHECK(tc_send_text(a, PKT_MSG_PUBLIC, "alice", "", "hello all"));
    CHECK(tc_wait_for(b, PKT_MSG_PUBLIC, "alice", "hello all", 2000));
    CHECK(tc_wait_for(c, PKT_MSG_PUBLIC, "alice", "hello all", 2000));
    CHECK(tc_silent(a, 200)); /* sender does not get an echo */
    close(a);
    close(b);
    close(c);
}

static void test_private_isolation(void) {
    int a = tc_login(port, "alice");
    int b = tc_login(port, "bob");
    int c = tc_login(port, "carol");
    CHECK(tc_send_text(a, PKT_MSG_PRIVATE, "alice", "bob", "secret meeting"));
    CHECK(tc_wait_for(b, PKT_MSG_PRIVATE, "alice", "secret meeting", 2000));
    CHECK(tc_silent(c, 300)); /* no leak to a third party */
    CHECK(tc_silent(a, 100));

    CHECK(tc_send_text(a, PKT_MSG_PRIVATE, "alice", "nobody", "x"));
    CHECK(tc_wait_for(a, PKT_SERVER_ERROR, NULL, "not found", 2000));
    CHECK(tc_send_text(a, PKT_MSG_PRIVATE, "alice", "alice", "x"));
    CHECK(tc_wait_for(a, PKT_SERVER_ERROR, NULL, "yourself", 2000));
    close(a);
    close(b);
    close(c);
}

static void test_group_isolation(void) {
    int a = tc_login(port, "alice");
    int b = tc_login(port, "bob");
    int c = tc_login(port, "carol");
    CHECK(tc_send_text(a, PKT_CMD_JOIN_GRP, "alice", "devs", NULL));
    CHECK(tc_wait_for(a, PKT_SERVER_INFO, NULL, "Joined group #devs", 2000));
    CHECK(tc_send_text(b, PKT_CMD_JOIN_GRP, "bob", "devs", NULL));
    CHECK(tc_wait_for(b, PKT_SERVER_INFO, NULL, "Joined group #devs", 2000));
    CHECK(tc_wait_for(a, PKT_SERVER_INFO, NULL, "bob joined #devs", 2000));

    CHECK(tc_send_text(a, PKT_MSG_GROUP, "alice", "devs", "standup now"));
    CHECK(tc_wait_for(b, PKT_MSG_GROUP, "alice", "standup now", 2000));
    CHECK(tc_silent(c, 300)); /* non-member sees nothing */

    /* non-member cannot post */
    CHECK(tc_send_text(c, PKT_MSG_GROUP, "carol", "devs", "let me in"));
    CHECK(tc_wait_for(c, PKT_SERVER_ERROR, NULL, "Join the group", 2000));
    CHECK(tc_silent(a, 200));

    /* leaving stops delivery */
    CHECK(tc_send_text(b, PKT_CMD_LEAVE_GRP, "bob", "devs", NULL));
    CHECK(tc_wait_for(b, PKT_SERVER_INFO, NULL, "Left group #devs", 2000));
    CHECK(tc_send_text(a, PKT_MSG_GROUP, "alice", "devs", "after leave"));
    CHECK(tc_silent(b, 300));
    CHECK(tc_send_text(b, PKT_CMD_LEAVE_GRP, "bob", "devs", NULL));
    CHECK(tc_wait_for(b, PKT_SERVER_ERROR, NULL, "not in that group", 2000));

    CHECK(tc_send_text(c, PKT_CMD_JOIN_GRP, "carol", "bad name!", NULL));
    CHECK(tc_wait_for(c, PKT_SERVER_ERROR, NULL, "Invalid group", 2000));
    close(a);
    close(b);
    close(c);
}

static void test_departure_notice(void) {
    int a = tc_login(port, "alice");
    int c = tc_login(port, "carol");
    CHECK(tc_send_text(c, PKT_DISCONNECT, "carol", "", NULL));
    CHECK(tc_wait_for(a, PKT_SERVER_INFO, NULL, "carol has left", 2000));
    close(c);
    /* abrupt close (no PKT_DISCONNECT) is also announced */
    int d = tc_login(port, "dave");
    close(d);
    CHECK(tc_wait_for(a, PKT_SERVER_INFO, NULL, "dave has left", 2000));
    close(a);
}

/* TCP tearing: bytes arrive one at a time / two frames in one write. */
static void test_stream_fragmentation(void) {
    int a = tc_login(port, "alice");
    int raw = tc_connect(port);
    CHECK(raw >= 0);

    packet_header_t h;
    memset(&h, 0, sizeof(h));
    h.type = htons(PKT_HANDSHAKE);
    strcpy(h.sender, "eve");
    const unsigned char *bytes = (const unsigned char *)&h;
    for (size_t i = 0; i < sizeof(h); i++) {
        CHECK(send_all(raw, bytes + i, 1) == 1); /* header torn into 68 writes */
        if (i % 17 == 0) {
            usleep(2000);
        }
    }
    packet_header_t in;
    char p[PROTO_MAX_PAYLOAD + 1];
    tc_set_timeout(raw, 2000);
    CHECK(packet_recv(raw, &in, p) == 1 && in.type == PKT_SERVER_INFO);

    /* two frames coalesced into one send() */
    unsigned char two[2 * (sizeof(h) + 5)];
    memset(&h, 0, sizeof(h));
    h.type = htons(PKT_MSG_PUBLIC);
    h.payload_len = htons(5);
    memcpy(two, &h, sizeof(h));
    memcpy(two + sizeof(h), "first", 5);
    memcpy(two + sizeof(h) + 5, &h, sizeof(h));
    memcpy(two + 2 * sizeof(h) + 5, "secnd", 5);
    CHECK(send_all(raw, two, sizeof(two)) == (ssize_t)sizeof(two));
    CHECK(tc_wait_for(a, PKT_MSG_PUBLIC, "eve", "first", 2000));
    CHECK(tc_wait_for(a, PKT_MSG_PUBLIC, "eve", "secnd", 2000));
    close(raw);
    close(a);
}

static void test_sender_spoofing_ignored(void) {
    int a = tc_login(port, "alice");
    int b = tc_login(port, "bob");
    CHECK(tc_send_text(b, PKT_MSG_PUBLIC, "alice", "", "i am totally alice"));
    CHECK(tc_wait_for(a, PKT_MSG_PUBLIC, "bob", "totally alice", 2000));
    close(a);
    close(b);
}

static void test_malformed_packet_disconnects_only_offender(void) {
    int a = tc_login(port, "alice");
    int b = tc_login(port, "bob");
    packet_header_t h;
    memset(&h, 0, sizeof(h));
    h.type = htons(PKT_MSG_PUBLIC);
    h.payload_len = htons(PROTO_MAX_PAYLOAD + 500);
    CHECK(send_all(b, &h, sizeof(h)) >= 0);
    packet_header_t in;
    char p[PROTO_MAX_PAYLOAD + 1];
    tc_set_timeout(b, 2000);
    while (packet_recv(b, &in, p) == 1) {
    }
    CHECK(tc_wait_for(a, PKT_SERVER_INFO, NULL, "bob has left", 2000));
    CHECK(tc_send_text(a, PKT_CMD_USERS, "alice", "", NULL)); /* server still healthy */
    CHECK(tc_wait_for(a, PKT_SERVER_INFO, NULL, "Connected users (1)", 2000));
    close(a);
    close(b);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc != 2) {
        fprintf(stderr, "usage: %s <path-to-chatroom_server>\n", argv[0]);
        return 2;
    }
    signal(SIGPIPE, SIG_IGN);
    port = tc_port(1);
    pid_t srv = tc_start_server(argv[1], port);
    CHECK(srv > 0);

    test_handshake_and_names();
    usleep(250000); /* let the server deregister closed clients */
    test_users_list();
    usleep(250000); /* let the server deregister closed clients */
    test_public_broadcast();
    usleep(250000); /* let the server deregister closed clients */
    test_private_isolation();
    usleep(250000); /* let the server deregister closed clients */
    test_group_isolation();
    usleep(250000); /* let the server deregister closed clients */
    test_departure_notice();
    usleep(250000); /* let the server deregister closed clients */
    test_stream_fragmentation();
    usleep(250000); /* let the server deregister closed clients */
    test_sender_spoofing_ignored();
    usleep(250000); /* let the server deregister closed clients */
    test_malformed_packet_disconnects_only_offender();
    usleep(250000); /* let the server deregister closed clients */

    tc_stop_server(srv);
    printf("All chat feature tests passed.\n");
    return 0;
}
