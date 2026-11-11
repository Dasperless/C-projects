#undef NDEBUG
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "protocol.h"

#include <arpa/inet.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void make_pair(int fds[2]) {
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
}

static void test_roundtrip(void) {
    int fds[2];
    make_pair(fds);
    assert(packet_send(fds[0], PKT_MSG_PRIVATE, "alice", "bob", "hello", 5));

    packet_header_t h;
    char payload[PROTO_MAX_PAYLOAD + 1];
    assert(packet_recv(fds[1], &h, payload) == 1);
    assert(h.type == PKT_MSG_PRIVATE && h.payload_len == 5);
    assert(strcmp(h.sender, "alice") == 0 && strcmp(h.recipient_or_group, "bob") == 0);
    assert(strcmp(payload, "hello") == 0);

    /* empty payload, NULL names */
    assert(packet_send(fds[0], PKT_CMD_USERS, NULL, NULL, NULL, 0));
    assert(packet_recv(fds[1], &h, payload) == 1);
    assert(h.type == PKT_CMD_USERS && h.payload_len == 0 && h.sender[0] == '\0');
    close(fds[0]);
    close(fds[1]);
}

static void test_header_layout(void) {
    assert(sizeof(packet_header_t) == 68);
    int fds[2];
    make_pair(fds);
    assert(packet_send(fds[0], PKT_MSG_PUBLIC, "a", "", "xy", 2));
    unsigned char raw[70];
    assert(recv_all(fds[1], raw, sizeof(raw)) == 70);
    assert(raw[0] == 0 && raw[1] == PKT_MSG_PUBLIC); /* network byte order */
    assert(raw[2] == 0 && raw[3] == 2);
    assert(raw[68] == 'x' && raw[69] == 'y');
    close(fds[0]);
    close(fds[1]);
}

/* Sender dribbles one byte at a time: recv_all/packet_recv must reassemble. */
static void *dribble(void *arg) {
    int fd = *(int *)arg;
    packet_header_t h;
    memset(&h, 0, sizeof(h));
    h.type = htons(PKT_MSG_PUBLIC);
    h.payload_len = htons(11);
    strcpy(h.sender, "carol");
    unsigned char frame[sizeof(h) + 11];
    memcpy(frame, &h, sizeof(h));
    memcpy(frame + sizeof(h), "fragmented!", 11);
    for (size_t i = 0; i < sizeof(frame); i++) {
        assert(send_all(fd, frame + i, 1) == 1);
        if (i % 16 == 0) {
            usleep(1000);
        }
    }
    return NULL;
}

static void test_fragmentation(void) {
    int fds[2];
    make_pair(fds);
    pthread_t t;
    pthread_create(&t, NULL, dribble, &fds[0]);
    packet_header_t h;
    char payload[PROTO_MAX_PAYLOAD + 1];
    assert(packet_recv(fds[1], &h, payload) == 1);
    assert(strcmp(payload, "fragmented!") == 0 && strcmp(h.sender, "carol") == 0);
    pthread_join(t, NULL);
    close(fds[0]);
    close(fds[1]);
}

/* Several packets coalesced into the same TCP segment are split correctly. */
static void test_coalescing(void) {
    int fds[2];
    make_pair(fds);
    for (int i = 0; i < 20; i++) {
        char msg[16];
        int n = snprintf(msg, sizeof(msg), "msg-%d", i);
        assert(packet_send(fds[0], PKT_MSG_PUBLIC, "dave", NULL, msg, (size_t)n));
    }
    for (int i = 0; i < 20; i++) {
        packet_header_t h;
        char payload[PROTO_MAX_PAYLOAD + 1];
        char want[16];
        snprintf(want, sizeof(want), "msg-%d", i);
        assert(packet_recv(fds[1], &h, payload) == 1);
        assert(strcmp(payload, want) == 0);
    }
    close(fds[0]);
    close(fds[1]);
}

static void test_max_payload_and_oversize(void) {
    int fds[2];
    make_pair(fds);
    char big[PROTO_MAX_PAYLOAD + 1];
    memset(big, 'z', sizeof(big));
    assert(!packet_send(fds[0], PKT_MSG_PUBLIC, "a", NULL, big, PROTO_MAX_PAYLOAD + 1));
    assert(!packet_send(fds[0], PKT_MSG_PUBLIC, "a", NULL, NULL, 5));

    /* exactly max size works; payload may contain binary and embedded NULs */
    big[10] = '\0';
    assert(packet_send(fds[0], PKT_MSG_PUBLIC, "a", NULL, big, PROTO_MAX_PAYLOAD));
    packet_header_t h;
    char payload[PROTO_MAX_PAYLOAD + 1];
    assert(packet_recv(fds[1], &h, payload) == 1);
    assert(h.payload_len == PROTO_MAX_PAYLOAD && memcmp(payload, big, PROTO_MAX_PAYLOAD) == 0);

    /* forged header claiming an oversized payload is rejected */
    packet_header_t bad;
    memset(&bad, 0, sizeof(bad));
    bad.type = htons(PKT_MSG_PUBLIC);
    bad.payload_len = htons(PROTO_MAX_PAYLOAD + 1);
    assert(send_all(fds[0], &bad, sizeof(bad)) >= 0);
    assert(packet_recv(fds[1], &h, payload) == -1);

    /* unknown packet type is rejected */
    bad.type = htons(99);
    bad.payload_len = 0;
    assert(send_all(fds[0], &bad, sizeof(bad)) >= 0);
    assert(packet_recv(fds[1], &h, payload) == -1);
    close(fds[0]);
    close(fds[1]);
}

static void test_eof_handling(void) {
    int fds[2];
    packet_header_t h;
    char payload[PROTO_MAX_PAYLOAD + 1];

    make_pair(fds);
    close(fds[0]);
    assert(packet_recv(fds[1], &h, payload) == 0); /* clean EOF */
    close(fds[1]);

    make_pair(fds);
    assert(send_all(fds[0], "abc", 3) == 3); /* truncated header */
    close(fds[0]);
    assert(packet_recv(fds[1], &h, payload) == -1);
    close(fds[1]);

    make_pair(fds);
    packet_header_t truncated;
    memset(&truncated, 0, sizeof(truncated));
    truncated.type = htons(PKT_MSG_PUBLIC);
    truncated.payload_len = htons(10);
    assert(send_all(fds[0], &truncated, sizeof(truncated)) >= 0);
    assert(send_all(fds[0], "short", 5) == 5); /* truncated payload */
    close(fds[0]);
    assert(packet_recv(fds[1], &h, payload) == -1);
    close(fds[1]);
}

static void test_name_forced_termination(void) {
    int fds[2];
    make_pair(fds);
    packet_header_t h;
    memset(&h, 'A', sizeof(h)); /* no NULs anywhere in the name fields */
    h.type = htons(PKT_MSG_PUBLIC);
    h.payload_len = htons(0);
    assert(send_all(fds[0], &h, sizeof(h)) >= 0);
    packet_header_t in;
    char payload[PROTO_MAX_PAYLOAD + 1];
    assert(packet_recv(fds[1], &in, payload) == 1);
    assert(strlen(in.sender) == PROTO_NAME_LEN - 1);
    assert(strlen(in.recipient_or_group) == PROTO_NAME_LEN - 1);
    close(fds[0]);
    close(fds[1]);
}

static void test_valid_names(void) {
    assert(proto_valid_name("al") && proto_valid_name("Bob_2.x-y"));
    assert(!proto_valid_name(NULL) && !proto_valid_name("a") && !proto_valid_name(""));
    assert(!proto_valid_name("has space") && !proto_valid_name("semi;colon"));
    assert(!proto_valid_name("abcdefghijklmnopqrstuvwxyz01234")); /* 31 chars */
    assert(proto_valid_name("abcdefghijklmnopqrstuvwxyz0123"));   /* 30 chars */
}

static parsed_input_t parse(const char *line) {
    parsed_input_t p;
    proto_parse_input(line, &p);
    return p;
}

static void test_parse_input(void) {
    parsed_input_t p = parse("hello world\n");
    assert(p.action == INPUT_SEND && p.type == PKT_MSG_PUBLIC &&
           strcmp(p.text, "hello world") == 0);

    p = parse("/msg bob hi there  friend");
    assert(p.action == INPUT_SEND && p.type == PKT_MSG_PRIVATE);
    assert(strcmp(p.target, "bob") == 0 && strcmp(p.text, "hi there  friend") == 0);

    p = parse("#devs ship it");
    assert(p.type == PKT_MSG_GROUP && strcmp(p.target, "devs") == 0 &&
           strcmp(p.text, "ship it") == 0);

    p = parse("/group devs");
    assert(p.action == INPUT_SEND && p.type == PKT_CMD_JOIN_GRP && strcmp(p.target, "devs") == 0);
    p = parse("/leave devs\r\n");
    assert(p.type == PKT_CMD_LEAVE_GRP && strcmp(p.target, "devs") == 0);
    p = parse("/users");
    assert(p.action == INPUT_SEND && p.type == PKT_CMD_USERS);

    assert(parse("/exit").action == INPUT_EXIT);
    assert(parse("/help").action == INPUT_HELP);
    assert(parse("").action == INPUT_EMPTY && parse("\n").action == INPUT_EMPTY);
    assert(parse(NULL).action == INPUT_EMPTY);

    assert(parse("/msg").action == INPUT_ERROR);
    assert(parse("/msg bob").action == INPUT_ERROR);
    assert(parse("/group").action == INPUT_ERROR);
    assert(parse("#devs").action == INPUT_ERROR && parse("#").action == INPUT_ERROR);
    assert(parse("/bogus x").action == INPUT_ERROR);

    p = parse("/sendfile bob /tmp/my photo.png");
    assert(p.action == INPUT_SENDFILE && strcmp(p.target, "bob") == 0);
    assert(strcmp(p.text, "/tmp/my photo.png") == 0);
    assert(parse("/sendfile bob").action == INPUT_ERROR &&
           parse("/sendfile").action == INPUT_ERROR);
    p = parse("/fileaccept 7");
    assert(p.action == INPUT_FILE_ACCEPT && strcmp(p.text, "7") == 0);
    p = parse("/filereject 7");
    assert(p.action == INPUT_FILE_REJECT && strcmp(p.text, "7") == 0);
    assert(parse("/fileaccept").action == INPUT_ERROR);
    assert(parse("/msg bob x").error == NULL);

    char big[PROTO_MAX_PAYLOAD + 10];
    memset(big, 'a', sizeof(big));
    big[sizeof(big) - 1] = '\0';
    assert(parse(big).action == INPUT_ERROR);
    big[PROTO_MAX_PAYLOAD] = '\0';
    assert(parse(big).action == INPUT_SEND);
}

int main(void) {
    test_roundtrip();
    test_header_layout();
    test_fragmentation();
    test_coalescing();
    test_max_payload_and_oversize();
    test_eof_handling();
    test_name_forced_termination();
    test_valid_names();
    test_parse_input();
    printf("All protocol tests passed.\n");
    return 0;
}
