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
/* Large enough for one 4 KB file chunk plus its 11-byte chunk header (issue #14). */
#define PROTO_MAX_PAYLOAD 4160

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
    PKT_SERVER_ERROR = 10,
    PKT_FILE_OFFER = 11,
    PKT_FILE_ACCEPT = 12,
    PKT_FILE_REJECT = 13,
    PKT_FILE_CHUNK = 14,
    PKT_FILE_ABORT = 15
} packet_type_t;

#define PKT_TYPE_MAX PKT_FILE_ABORT

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

/* ---- client-side input parsing ---- */

typedef enum {
    INPUT_EMPTY, /* blank line, nothing to do */
    INPUT_SEND,  /* send a packet built from the fields below */
    INPUT_HELP,  /* show local help */
    INPUT_EXIT,  /* leave the chat */
    INPUT_SENDFILE,    /* target = recipient, text = file path */
    INPUT_FILE_ACCEPT, /* text = transfer id */
    INPUT_FILE_REJECT, /* text = transfer id */
    INPUT_ERROR  /* invalid input, show `error` */
} input_action_t;

typedef struct {
    input_action_t action;
    packet_type_t type;
    char target[PROTO_NAME_LEN];
    char text[PROTO_MAX_PAYLOAD + 1];
    const char *error;
} parsed_input_t;

/*
 * Maps one line typed by the user to an action:
 *   text                  -> PKT_MSG_PUBLIC
 *   /msg <user> <text>    -> PKT_MSG_PRIVATE
 *   #<group> <text>       -> PKT_MSG_GROUP
 *   /users                -> PKT_CMD_USERS
 *   /group <name>         -> PKT_CMD_JOIN_GRP
 *   /leave <name>         -> PKT_CMD_LEAVE_GRP
 *   /help, /exit          -> INPUT_HELP, INPUT_EXIT
 *   /sendfile <user> <path>, /fileaccept <id>, /filereject <id>
 *                         -> INPUT_SENDFILE, INPUT_FILE_ACCEPT, INPUT_FILE_REJECT
 */
input_action_t proto_parse_input(const char *line, parsed_input_t *out);

#endif /* CHATROOM_PROTOCOL_H */
