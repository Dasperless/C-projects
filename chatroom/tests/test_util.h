#undef NDEBUG
#ifndef CHATROOM_TEST_UTIL_H
#define CHATROOM_TEST_UTIL_H

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "protocol.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define TC_IP "127.0.0.1"

/* Per-process port so concurrent CI jobs do not collide; `slot` separates test binaries. */
static inline int tc_port(int slot) {
    return 20000 + (int)((getpid() + slot * 997) % 20000);
}

static inline int tc_connect(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, TC_IP, &addr.sin_addr);
    addr.sin_port = htons((uint16_t)port);
    if (connect(sock, (struct sockaddr *)&addr, (socklen_t)sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}

/* Forks the server binary and waits (up to ~5 s) until it accepts connections. */
static inline pid_t tc_start_server(const char *bin, int port) {
    pid_t pid = fork();
    if (pid == 0) {
        char port_str[16];
        snprintf(port_str, sizeof(port_str), "%d", port);
        execl(bin, bin, TC_IP, port_str, (char *)NULL);
        perror("execl failed");
        _exit(127);
    }
    for (int i = 0; i < 100; i++) {
        int s = tc_connect(port);
        if (s >= 0) {
            close(s);
            return pid;
        }
        usleep(50000);
    }
    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);
    return -1;
}

static inline int tc_stop_server(pid_t pid) {
    kill(pid, SIGTERM);
    int status = 0;
    waitpid(pid, &status, 0);
    return status;
}

static inline void tc_set_timeout(int fd, int ms) {
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

/* Connect and handshake. Returns the socket, or -1 if the server rejected the name. */
static inline int tc_login(int port, const char *name) {
    int fd = tc_connect(port);
    if (fd < 0) {
        return -1;
    }
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    tc_set_timeout(fd, 2000);
    if (!packet_send(fd, PKT_HANDSHAKE, name, NULL, NULL, 0) || packet_recv(fd, &h, p) != 1 ||
        h.type != PKT_SERVER_INFO) {
        close(fd);
        return -1;
    }
    return fd;
}

static inline int tc_send_text(int fd, packet_type_t type, const char *me, const char *target,
                               const char *text) {
    return packet_send(fd, type, me, target, text, text ? strlen(text) : 0) ? 1 : 0;
}

/* Reads packets until one has `type`, `sender` (if non-NULL) and a payload containing `needle`
 * (if non-NULL). Returns 1 if found before the timeout, 0 otherwise. */
static inline int tc_wait_for(int fd, packet_type_t type, const char *sender, const char *needle,
                              int timeout_ms) {
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    tc_set_timeout(fd, timeout_ms);
    while (packet_recv(fd, &h, p) == 1) {
        if (h.type == type && (sender == NULL || strcmp(h.sender, sender) == 0) &&
            (needle == NULL || strstr(p, needle) != NULL)) {
            return 1;
        }
    }
    return 0;
}

/* Returns 1 if no chat message (public/private/group) arrives within timeout_ms. */
static inline int tc_silent(int fd, int timeout_ms) {
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    tc_set_timeout(fd, timeout_ms);
    while (packet_recv(fd, &h, p) == 1) {
        if (h.type == PKT_MSG_PUBLIC || h.type == PKT_MSG_PRIVATE || h.type == PKT_MSG_GROUP) {
            return 0;
        }
    }
    return 1;
}

#endif /* CHATROOM_TEST_UTIL_H */
