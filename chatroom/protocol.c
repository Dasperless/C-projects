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

/* Splits "word rest" in place: returns pointer to rest (trimmed) or "" if none. */
static char *split_word(char *s) {
    while (*s != '\0' && *s != ' ') {
        s++;
    }
    if (*s == '\0') {
        return s;
    }
    *s++ = '\0';
    while (*s == ' ') {
        s++;
    }
    return s;
}

static input_action_t fail(parsed_input_t *out, const char *msg) {
    out->action = INPUT_ERROR;
    out->error = msg;
    return INPUT_ERROR;
}

static bool set_target(parsed_input_t *out, const char *t) {
    if (t[0] == '\0' || strlen(t) > PROTO_NAME_LEN - 1) {
        return false;
    }
    memcpy(out->target, t, strlen(t) + 1);
    return true;
}

input_action_t proto_parse_input(const char *line, parsed_input_t *out) {
    memset(out, 0, sizeof(*out));
    out->action = INPUT_EMPTY;
    if (line == NULL) {
        return INPUT_EMPTY;
    }
    char buf[PROTO_MAX_PAYLOAD + 2];
    size_t len = strcspn(line, "\r\n");
    if (len > PROTO_MAX_PAYLOAD) {
        return fail(out, "Message too long");
    }
    memcpy(buf, line, len);
    buf[len] = '\0';
    if (buf[0] == '\0') {
        return INPUT_EMPTY;
    }

    if (buf[0] != '/' && buf[0] != '#') {
        out->action = INPUT_SEND;
        out->type = PKT_MSG_PUBLIC;
        memcpy(out->text, buf, len + 1);
        return INPUT_SEND;
    }

    char *rest = split_word(buf);
    out->action = INPUT_SEND;
    if (buf[0] == '#') {
        out->type = PKT_MSG_GROUP;
        if (!set_target(out, buf + 1) || rest[0] == '\0') {
            return fail(out, "Usage: #<group> <text>");
        }
        memcpy(out->text, rest, strlen(rest) + 1);
        return INPUT_SEND;
    }
    if (strcmp(buf, "/exit") == 0) {
        out->action = INPUT_EXIT;
    } else if (strcmp(buf, "/help") == 0) {
        out->action = INPUT_HELP;
    } else if (strcmp(buf, "/users") == 0) {
        out->type = PKT_CMD_USERS;
    } else if (strcmp(buf, "/msg") == 0) {
        char *text = split_word(rest);
        out->type = PKT_MSG_PRIVATE;
        if (!set_target(out, rest) || text[0] == '\0') {
            return fail(out, "Usage: /msg <username> <text>");
        }
        memcpy(out->text, text, strlen(text) + 1);
    } else if (strcmp(buf, "/group") == 0 || strcmp(buf, "/leave") == 0) {
        out->type = buf[1] == 'g' ? PKT_CMD_JOIN_GRP : PKT_CMD_LEAVE_GRP;
        if (!set_target(out, rest)) {
            return fail(out, buf[1] == 'g' ? "Usage: /group <name>" : "Usage: /leave <name>");
        }
    } else {
        return fail(out, "Unknown command. Type /help");
    }
    return out->action;
}
