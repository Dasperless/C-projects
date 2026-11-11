#ifndef CHATROOM_PROTOCOL_H
#define CHATROOM_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/*
 * Length-prefixed application-layer framing for the chatroom.
 *
 * Wire format: packet_header_t (68 bytes, type and payload_len in network
 * byte order) immediately followed by payload_len payload bytes.
 */

#define PROTO_NAME_LEN 32
#define PROTO_MAX_PAYLOAD 2048

typedef enum {
    PKT_HANDSHAKE = 1,
    PKT_MSG_PUBLIC = 2,
    PKT_MSG_PRIVATE = 3,
    PKT_MSG_GROUP = 4,
    PKT_CMD_USERS = 5,
    PKT_CMD_JOIN_GRP = 6,
    PKT_CMD_LEAVE_GRP = 7,
    PKT_DISCONNECT = 8,
    PKT_SERVER_INFO = 9,
    PKT_SERVER_ERROR = 10
} packet_type_t;

typedef struct __attribute__((packed)) {
    uint16_t type;
    uint16_t payload_len;
    char sender[PROTO_NAME_LEN];
    char recipient_or_group[PROTO_NAME_LEN];
} packet_header_t;

/* Writes exactly len bytes (retries on EINTR / short writes). Returns len or -1. */
ssize_t send_all(int sockfd, const void *buf, size_t len);

/* Reads exactly len bytes. Returns len, 0 on clean EOF before the first byte, -1 on error or
 * EOF in the middle of the buffer. */
ssize_t recv_all(int sockfd, void *buf, size_t len);

/* Frames and sends one packet with a single send_all call (so it is atomic with respect to
 * other threads that serialize on the socket). sender/target may be NULL. */
bool packet_send(int sockfd, packet_type_t type, const char *sender, const char *target,
                 const void *payload, size_t payload_len);

/* Receives one packet. payload must hold PROTO_MAX_PAYLOAD + 1 bytes and is NUL-terminated.
 * The header is converted to host byte order and its strings are NUL-terminated.
 * Returns 1 on success, 0 on clean EOF, -1 on error or an invalid/oversized packet. */
int packet_recv(int sockfd, packet_header_t *hdr, char *payload);

/* A valid user name is 2..PROTO_NAME_LEN-2 characters of [A-Za-z0-9_.-]. */
bool proto_valid_name(const char *name);

#endif /* CHATROOM_PROTOCOL_H */
