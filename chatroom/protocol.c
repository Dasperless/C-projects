#define _POSIX_C_SOURCE 200809L
#include "protocol.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

ssize_t send_all(int sockfd, const void *buf, size_t len) {
    const char *p = (const char *)buf;
    size_t done = 0;
    while (done < len) {
        ssize_t n = send(sockfd, p + done, len - done, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        done += (size_t)n;
    }
    return (ssize_t)done;
}

ssize_t recv_all(int sockfd, void *buf, size_t len) {
    char *p = (char *)buf;
    size_t done = 0;
    while (done < len) {
        ssize_t n = recv(sockfd, p + done, len - done, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return done == 0 ? 0 : -1;
        }
        done += (size_t)n;
    }
    return (ssize_t)done;
}

static void copy_name(char dst[PROTO_NAME_LEN], const char *src) {
    memset(dst, 0, PROTO_NAME_LEN);
    if (src != NULL) {
        size_t n = strlen(src);
        if (n > PROTO_NAME_LEN - 1) {
            n = PROTO_NAME_LEN - 1;
        }
        memcpy(dst, src, n);
    }
}

bool packet_send(int sockfd, packet_type_t type, const char *sender, const char *target,
                 const void *payload, size_t payload_len) {
    if (payload_len > PROTO_MAX_PAYLOAD || (payload_len > 0 && payload == NULL)) {
        return false;
    }
    char frame[sizeof(packet_header_t) + PROTO_MAX_PAYLOAD];
    packet_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.type = htons((uint16_t)type);
    hdr.payload_len = htons((uint16_t)payload_len);
    copy_name(hdr.sender, sender);
    copy_name(hdr.recipient_or_group, target);
    memcpy(frame, &hdr, sizeof(hdr));
    if (payload_len > 0) {
        memcpy(frame + sizeof(hdr), payload, payload_len);
    }
    return send_all(sockfd, frame, sizeof(hdr) + payload_len) >= 0;
}

int packet_recv(int sockfd, packet_header_t *hdr, char *payload) {
    ssize_t r = recv_all(sockfd, hdr, sizeof(*hdr));
    if (r <= 0) {
        return r == 0 ? 0 : -1;
    }
    hdr->type = ntohs(hdr->type);
    hdr->payload_len = ntohs(hdr->payload_len);
    hdr->sender[PROTO_NAME_LEN - 1] = '\0';
    hdr->recipient_or_group[PROTO_NAME_LEN - 1] = '\0';
    if (hdr->payload_len > PROTO_MAX_PAYLOAD || hdr->type < PKT_HANDSHAKE ||
        hdr->type > PKT_SERVER_ERROR) {
        return -1;
    }
    if (hdr->payload_len > 0 && recv_all(sockfd, payload, hdr->payload_len) <= 0) {
        return -1;
    }
    payload[hdr->payload_len] = '\0';
    return 1;
}

bool proto_valid_name(const char *name) {
    if (name == NULL) {
        return false;
    }
    size_t n = strlen(name);
    if (n < 2 || n > PROTO_NAME_LEN - 2) {
        return false;
    }
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)name[i];
        if (!(isalnum(c) || c == '_' || c == '-' || c == '.')) {
            return false;
        }
    }
    return true;
}
