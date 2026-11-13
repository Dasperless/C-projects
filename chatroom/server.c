#define _POSIX_C_SOURCE 200809L
#include "fileshare.h"
#include "logger.h"
#include "protocol.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_CLIENTS 50
#define MAX_GROUPS_PER_CLIENT 8
#define MAX_TRANSFERS 32

typedef struct {
    struct sockaddr_in address;
    int sockfd;
    int uid;
    time_t connected_at;
    pthread_t tid;
    bool active;               /* worker thread is still running */
    bool kicked;               /* suppress the duplicate "has left" broadcast */
    char name[PROTO_NAME_LEN]; /* empty until the handshake succeeds */
    char groups[MAX_GROUPS_PER_CLIENT][PROTO_NAME_LEN];
    int group_count;
} client_t;

/* A relayed file transfer, tracked by user name so it survives only as long as both peers do. */
typedef struct {
    bool used;
    bool accepted;
    uint32_t id;
    char from[PROTO_NAME_LEN];
    char to[PROTO_NAME_LEN];
    char filename[FS_FILENAME_LEN];
    uint64_t size;
    uint64_t relayed;
    uint32_t next_index;
} transfer_t;

static transfer_t transfers[MAX_TRANSFERS];
static uint32_t transfer_counter = 0;

static int client_count = 0;
static int uid_counter = 10;
static client_t *clients[MAX_CLIENTS] = {NULL};
/* Guards the clients table, metrics and every socket write (so frames never interleave). */
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ---- runtime metrics and admin state (issue #15) ---- */

static time_t server_started_at = 0;
static int peak_clients = 0;
static uint64_t total_connections = 0;
static uint64_t msg_public_count = 0;
static uint64_t msg_private_count = 0;
static uint64_t msg_group_count = 0;
static uint64_t bytes_tx = 0;
static uint64_t bytes_rx = 0;
static volatile sig_atomic_t monitor_enabled = 1;
static volatile sig_atomic_t stop_requested = 0;
static int listen_fd = -1;

static void on_stop_signal(int sig) {
    (void)sig;
    stop_requested = 1;
    if (listen_fd >= 0) {
        shutdown(listen_fd, SHUT_RDWR); /* async-signal-safe; wakes accept() */
    }
}

static void request_shutdown(void) {
    stop_requested = 1;
    if (listen_fd >= 0) {
        shutdown(listen_fd, SHUT_RDWR);
    }
}

/* ---- helpers: *_locked functions require clients_mutex to be held ---- */

static void send_raw_locked(const client_t *to, packet_type_t type, const char *sender,
                            const char *target, const void *data, size_t len) {
    if (packet_send(to->sockfd, type, sender, target, data, len)) {
        bytes_tx += sizeof(packet_header_t) + len;
    }
}

static void send_to_locked(const client_t *to, packet_type_t type, const char *sender,
                           const char *target, const char *text) {
    send_raw_locked(to, type, sender, target, text, strlen(text));
}

static int recv_counted(int sockfd, packet_header_t *hdr, char *payload) {
    int r = packet_recv(sockfd, hdr, payload);
    if (r == 1) {
        pthread_mutex_lock(&clients_mutex);
        bytes_rx += sizeof(packet_header_t) + hdr->payload_len;
        pthread_mutex_unlock(&clients_mutex);
    }
    return r;
}

static client_t *find_by_name_locked(const char *name) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->name[0] != '\0' &&
            strcmp(clients[i]->name, name) == 0) {
            return clients[i];
        }
    }
    return NULL;
}

static bool is_member(const client_t *c, const char *group) {
    for (int i = 0; i < c->group_count; i++) {
        if (strcmp(c->groups[i], group) == 0) {
            return true;
        }
    }
    return false;
}

/* Sends to every registered client except skip_uid. If group is non-NULL only its members
 * receive the packet. */
static void fanout_locked(packet_type_t type, const char *sender, const char *target,
                          const char *text, int skip_uid, const char *group) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_t *c = clients[i];
        if (c == NULL || c->name[0] == '\0' || c->uid == skip_uid) {
            continue;
        }
        if (group != NULL && !is_member(c, group)) {
            continue;
        }
        send_to_locked(c, type, sender, target, text);
    }
}

static void peer_ip(const client_t *c, char *buf, size_t len) {
    if (inet_ntop(AF_INET, &c->address.sin_addr, buf, (socklen_t)len) == NULL) {
        snprintf(buf, len, "?");
    }
}

/* Reaps finished worker threads so the slot table never stays full of zombies. */
static void reap_finished_clients(void) {
    while (1) {
        client_t *dead = NULL;
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != NULL && !clients[i]->active) {
                dead = clients[i];
                clients[i] = NULL;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
        if (dead == NULL) {
            return;
        }
        pthread_join(dead->tid, NULL);
        free(dead);
    }
}

static bool queue_client(client_t *cl) {
    reap_finished_clients();
    bool ok = false;
    pthread_mutex_lock(&clients_mutex);
    if (client_count < MAX_CLIENTS) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == NULL) {
                clients[i] = cl;
                client_count++;
                total_connections++;
                if (client_count > peak_clients) {
                    peak_clients = client_count;
                }
                ok = true;
                break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return ok;
}

/* ---- command handlers ---- */

static void reply_error(client_t *client, const char *text) {
    pthread_mutex_lock(&clients_mutex);
    send_to_locked(client, PKT_SERVER_ERROR, "server", client->name, text);
    pthread_mutex_unlock(&clients_mutex);
}

static void handle_users(client_t *client) {
    char list[PROTO_MAX_PAYLOAD];
    int count = 0;
    size_t used = 0;
    list[0] = '\0';
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->name[0] != '\0') {
            int n = snprintf(list + used, sizeof(list) - used, "%s%s", count ? ", " : "",
                             clients[i]->name);
            if (n > 0 && used + (size_t)n < sizeof(list)) {
                used += (size_t)n;
            }
            count++;
        }
    }
    char out[PROTO_MAX_PAYLOAD];
    snprintf(out, sizeof(out), "Connected users (%d): %.1900s", count, list);
    send_to_locked(client, PKT_SERVER_INFO, "server", client->name, out);
    pthread_mutex_unlock(&clients_mutex);
}

static void handle_private(client_t *client, const char *to, const char *text) {
    pthread_mutex_lock(&clients_mutex);
    client_t *dest = find_by_name_locked(to);
    if (dest == NULL) {
        char err[PROTO_MAX_PAYLOAD];
        snprintf(err, sizeof(err), "User '%.31s' not found", to);
        send_to_locked(client, PKT_SERVER_ERROR, "server", client->name, err);
    } else if (dest == client) {
        send_to_locked(client, PKT_SERVER_ERROR, "server", client->name,
                       "Cannot send a private message to yourself");
    } else {
        send_to_locked(dest, PKT_MSG_PRIVATE, client->name, dest->name, text);
        msg_private_count++;
        logger_log(LOG_LEVEL_INFO, "PRIV", "'%s' -> '%s': \"%s\"", client->name, dest->name, text);
        if (monitor_enabled) {
            printf("[PRIV] %s -> %s: %s\n", client->name, dest->name, text);
            fflush(stdout);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

static bool valid_group_name(const char *g) {
    size_t n = strlen(g);
    if (n < 1 || n > PROTO_NAME_LEN - 2) {
        return false;
    }
    for (size_t i = 0; i < n; i++) {
        char c = g[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '_' || c == '-')) {
            return false;
        }
    }
    return true;
}

static void handle_join_group(client_t *client, const char *group) {
    if (!valid_group_name(group)) {
        reply_error(client, "Invalid group name (use letters, digits, '_' or '-')");
        return;
    }
    pthread_mutex_lock(&clients_mutex);
    if (is_member(client, group)) {
        send_to_locked(client, PKT_SERVER_ERROR, "server", client->name,
                       "You are already in that group");
    } else if (client->group_count >= MAX_GROUPS_PER_CLIENT) {
        send_to_locked(client, PKT_SERVER_ERROR, "server", client->name,
                       "Group membership limit reached");
    } else {
        snprintf(client->groups[client->group_count++], PROTO_NAME_LEN, "%s", group);
        char out[PROTO_MAX_PAYLOAD];
        snprintf(out, sizeof(out), "Joined group #%s", group);
        send_to_locked(client, PKT_SERVER_INFO, "server", client->name, out);
        snprintf(out, sizeof(out), "%s joined #%s", client->name, group);
        fanout_locked(PKT_SERVER_INFO, "server", group, out, client->uid, group);
        logger_log(LOG_LEVEL_INFO, "GRP", "'%s' joined #%s", client->name, group);
    }
    pthread_mutex_unlock(&clients_mutex);
}

static void handle_leave_group(client_t *client, const char *group) {
    pthread_mutex_lock(&clients_mutex);
    int idx = -1;
    for (int i = 0; i < client->group_count; i++) {
        if (strcmp(client->groups[i], group) == 0) {
            idx = i;
        }
    }
    if (idx < 0) {
        send_to_locked(client, PKT_SERVER_ERROR, "server", client->name,
                       "You are not in that group");
    } else {
        for (int i = idx; i + 1 < client->group_count; i++) {
            memcpy(client->groups[i], client->groups[i + 1], PROTO_NAME_LEN);
        }
        client->group_count--;
        char out[PROTO_MAX_PAYLOAD];
        snprintf(out, sizeof(out), "Left group #%s", group);
        send_to_locked(client, PKT_SERVER_INFO, "server", client->name, out);
        snprintf(out, sizeof(out), "%s left #%s", client->name, group);
        fanout_locked(PKT_SERVER_INFO, "server", group, out, client->uid, group);
        logger_log(LOG_LEVEL_INFO, "GRP", "'%s' left #%s", client->name, group);
    }
    pthread_mutex_unlock(&clients_mutex);
}

static void handle_group_msg(client_t *client, const char *group, const char *text) {
    pthread_mutex_lock(&clients_mutex);
    if (!is_member(client, group)) {
        send_to_locked(client, PKT_SERVER_ERROR, "server", client->name,
                       "Join the group first with /group <name>");
    } else {
        fanout_locked(PKT_MSG_GROUP, client->name, group, text, client->uid, group);
        msg_group_count++;
        logger_log(LOG_LEVEL_INFO, "GRP", "'%s' -> #%s: \"%s\"", client->name, group, text);
        if (monitor_enabled) {
            printf("[#%s] %s: %s\n", group, client->name, text);
            fflush(stdout);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

/* ---- file transfer relay (issue #14); all helpers need clients_mutex held ---- */

static transfer_t *find_transfer_locked(uint32_t id) {
    for (int i = 0; i < MAX_TRANSFERS; i++) {
        if (transfers[i].used && transfers[i].id == id) {
            return &transfers[i];
        }
    }
    return NULL;
}

/* Tells both peers the transfer is dead (except `skip`, who triggered it) and frees the slot. */
static void abort_transfer_locked(transfer_t *t, const char *reason, const char *skip) {
    uint8_t ctl[PROTO_MAX_PAYLOAD];
    size_t n = fs_ctl_encode(t->id, reason, ctl, sizeof(ctl));
    const char *peers[2] = {t->from, t->to};
    for (int i = 0; i < 2; i++) {
        client_t *c = find_by_name_locked(peers[i]);
        if (c != NULL && (skip == NULL || strcmp(skip, peers[i]) != 0)) {
            send_raw_locked(c, PKT_FILE_ABORT, "server", c->name, ctl, n);
        }
    }
    logger_log(LOG_LEVEL_WARN, "FILE", "Transfer %u '%s' %s -> %s aborted: %s", t->id, t->filename,
               t->from, t->to, reason);
    t->used = false;
}

static void abort_transfers_of_locked(const char *name) {
    for (int i = 0; i < MAX_TRANSFERS; i++) {
        if (transfers[i].used &&
            (strcmp(transfers[i].from, name) == 0 || strcmp(transfers[i].to, name) == 0)) {
            abort_transfer_locked(&transfers[i], "peer disconnected", name);
        }
    }
}

static void handle_file_offer(client_t *client, const packet_header_t *hdr, const char *payload) {
    file_offer_t offer;
    if (hdr->payload_len != sizeof(offer)) {
        reply_error(client, "Malformed file offer");
        return;
    }
    memcpy(&offer, payload, sizeof(offer));
    fs_offer_to_host(&offer);
    char safe[FS_FILENAME_LEN];
    if (!fs_sanitize_filename(offer.filename, safe, sizeof(safe))) {
        reply_error(client, "Invalid file name");
        return;
    }
    if (offer.file_size > FS_MAX_FILE_SIZE) {
        reply_error(client, "File too large (maximum is 50 MB)");
        return;
    }
    pthread_mutex_lock(&clients_mutex);
    client_t *dest = find_by_name_locked(hdr->recipient_or_group);
    transfer_t *slot = NULL;
    for (int i = 0; i < MAX_TRANSFERS && slot == NULL; i++) {
        if (!transfers[i].used) {
            slot = &transfers[i];
        }
    }
    if (dest == NULL || dest == client) {
        send_to_locked(client, PKT_SERVER_ERROR, "server", client->name,
                       dest == NULL ? "Recipient not found" : "Cannot send a file to yourself");
    } else if (slot == NULL) {
        send_to_locked(client, PKT_SERVER_ERROR, "server", client->name,
                       "Too many transfers in progress");
    } else {
        memset(slot, 0, sizeof(*slot));
        slot->used = true;
        slot->id = ++transfer_counter;
        snprintf(slot->from, sizeof(slot->from), "%s", client->name);
        snprintf(slot->to, sizeof(slot->to), "%s", dest->name);
        snprintf(slot->filename, sizeof(slot->filename), "%s", safe);
        slot->size = offer.file_size;

        file_offer_t relay;
        memset(&relay, 0, sizeof(relay));
        snprintf(relay.sender, sizeof(relay.sender), "%s", client->name);
        snprintf(relay.recipient, sizeof(relay.recipient), "%s", dest->name);
        snprintf(relay.filename, sizeof(relay.filename), "%s", safe);
        relay.file_size = offer.file_size;
        relay.checksum_crc32 = offer.checksum_crc32;
        relay.transfer_id = slot->id;
        fs_offer_to_net(&relay);
        send_raw_locked(dest, PKT_FILE_OFFER, client->name, dest->name, &relay, sizeof(relay));
        /* the echo tells the sender which transfer id the server assigned */
        send_raw_locked(client, PKT_FILE_OFFER, client->name, dest->name, &relay, sizeof(relay));
        logger_log(LOG_LEVEL_INFO, "FILE", "'%s' offered '%s' (%llu bytes) to '%s' [id %u]",
                   client->name, safe, (unsigned long long)offer.file_size, dest->name, slot->id);
    }
    pthread_mutex_unlock(&clients_mutex);
}

static void handle_file_response(client_t *client, packet_type_t type, const char *payload,
                                 size_t len) {
    uint32_t id = 0;
    pthread_mutex_lock(&clients_mutex);
    transfer_t *t = fs_ctl_decode(payload, len, &id, NULL) ? find_transfer_locked(id) : NULL;
    if (t == NULL || t->accepted || strcmp(t->to, client->name) != 0) {
        send_to_locked(client, PKT_SERVER_ERROR, "server", client->name,
                       "No such pending transfer");
    } else {
        client_t *sender = find_by_name_locked(t->from);
        uint8_t ctl[8];
        size_t n = fs_ctl_encode(id, NULL, ctl, sizeof(ctl));
        if (sender != NULL) {
            send_raw_locked(sender, type, client->name, sender->name, ctl, n);
        }
        if (type == PKT_FILE_ACCEPT) {
            t->accepted = true;
            logger_log(LOG_LEVEL_INFO, "FILE", "'%s' accepted transfer %u ('%s' from '%s')",
                       client->name, id, t->filename, t->from);
        } else {
            logger_log(LOG_LEVEL_INFO, "FILE", "'%s' rejected transfer %u ('%s' from '%s')",
                       client->name, id, t->filename, t->from);
            t->used = false;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

static void handle_file_chunk(client_t *client, const char *payload, size_t len) {
    fs_chunk_view_t v;
    pthread_mutex_lock(&clients_mutex);
    if (!fs_chunk_decode(payload, len, &v)) {
        send_to_locked(client, PKT_SERVER_ERROR, "server", client->name, "Malformed file chunk");
        pthread_mutex_unlock(&clients_mutex);
        return;
    }
    transfer_t *t = find_transfer_locked(v.transfer_id);
    if (t == NULL || !t->accepted || strcmp(t->from, client->name) != 0) {
        send_to_locked(client, PKT_SERVER_ERROR, "server", client->name,
                       "No active transfer with that id");
    } else if (v.chunk_index != t->next_index || t->relayed + v.chunk_size > t->size ||
               v.is_last != (t->relayed + v.chunk_size == t->size)) {
        abort_transfer_locked(t, "protocol violation (bad chunk sequence or size)", NULL);
    } else {
        client_t *dest = find_by_name_locked(t->to);
        if (dest == NULL) {
            abort_transfer_locked(t, "peer disconnected", client->name);
        } else {
            send_raw_locked(dest, PKT_FILE_CHUNK, client->name, dest->name, payload, len);
            t->relayed += v.chunk_size;
            t->next_index++;
            if (v.is_last) {
                logger_log(LOG_LEVEL_INFO, "FILE",
                           "Transfer %u '%s' %s -> %s completed (%llu bytes)", t->id, t->filename,
                           t->from, t->to, (unsigned long long)t->size);
                t->used = false;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

static void handle_file_abort(client_t *client, const char *payload, size_t len) {
    uint32_t id = 0;
    pthread_mutex_lock(&clients_mutex);
    transfer_t *t = fs_ctl_decode(payload, len, &id, NULL) ? find_transfer_locked(id) : NULL;
    if (t == NULL || (strcmp(t->from, client->name) != 0 && strcmp(t->to, client->name) != 0)) {
        send_to_locked(client, PKT_SERVER_ERROR, "server", client->name, "No such transfer");
    } else {
        abort_transfer_locked(t, "cancelled by peer", client->name);
    }
    pthread_mutex_unlock(&clients_mutex);
}

/* Performs the handshake. Returns true when the client is registered under a unique name. */
static bool do_handshake(client_t *client) {
    packet_header_t hdr;
    char payload[PROTO_MAX_PAYLOAD + 1];
    if (recv_counted(client->sockfd, &hdr, payload) != 1 || hdr.type != PKT_HANDSHAKE) {
        return false;
    }
    char ip[INET_ADDRSTRLEN];
    peer_ip(client, ip, sizeof(ip));
    if (!proto_valid_name(hdr.sender)) {
        logger_log(LOG_LEVEL_WARN, "AUTH", "Invalid username rejected from %s:%d", ip,
                   ntohs(client->address.sin_port));
        reply_error(client, "Invalid name: use 2-30 characters of letters, digits, '_', '-', '.'");
        return false;
    }
    pthread_mutex_lock(&clients_mutex);
    if (find_by_name_locked(hdr.sender) != NULL) {
        send_to_locked(client, PKT_SERVER_ERROR, "server", "", "Name already in use");
        logger_log(LOG_LEVEL_WARN, "AUTH", "Duplicate username rejected: '%s'", hdr.sender);
        pthread_mutex_unlock(&clients_mutex);
        return false;
    }
    snprintf(client->name, sizeof(client->name), "%s", hdr.sender);
    send_to_locked(client, PKT_SERVER_INFO, "server", client->name, "Welcome! Type /help.");
    char out[PROTO_MAX_PAYLOAD];
    snprintf(out, sizeof(out), "%s has joined the chat", client->name);
    fanout_locked(PKT_SERVER_INFO, "server", "", out, client->uid, NULL);
    logger_log(LOG_LEVEL_INFO, "CONN", "Client '%s' connected from %s:%d", client->name, ip,
               ntohs(client->address.sin_port));
    pthread_mutex_unlock(&clients_mutex);
    printf("%s\n", out);
    fflush(stdout);
    return true;
}

static void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    packet_header_t hdr;
    char payload[PROTO_MAX_PAYLOAD + 1];
    bool registered = do_handshake(client);

    while (registered) {
        int r = recv_counted(client->sockfd, &hdr, payload);
        if (r != 1) {
            break; /* disconnect or protocol violation */
        }
        if (hdr.type == PKT_DISCONNECT) {
            break;
        }
        switch (hdr.type) {
        case PKT_MSG_PUBLIC:
            if (hdr.payload_len > 0) {
                pthread_mutex_lock(&clients_mutex);
                fanout_locked(PKT_MSG_PUBLIC, client->name, "", payload, client->uid, NULL);
                msg_public_count++;
                logger_log(LOG_LEVEL_INFO, "MSG", "'%s' -> ALL: \"%s\"", client->name, payload);
                pthread_mutex_unlock(&clients_mutex);
                if (monitor_enabled) {
                    printf("%s: %s\n", client->name, payload);
                    fflush(stdout);
                }
            }
            break;
        case PKT_MSG_PRIVATE:
            handle_private(client, hdr.recipient_or_group, payload);
            break;
        case PKT_MSG_GROUP:
            handle_group_msg(client, hdr.recipient_or_group, payload);
            break;
        case PKT_CMD_USERS:
            handle_users(client);
            break;
        case PKT_FILE_OFFER:
            handle_file_offer(client, &hdr, payload);
            break;
        case PKT_FILE_ACCEPT:
        case PKT_FILE_REJECT:
            handle_file_response(client, (packet_type_t)hdr.type, payload, hdr.payload_len);
            break;
        case PKT_FILE_CHUNK:
            handle_file_chunk(client, payload, hdr.payload_len);
            break;
        case PKT_FILE_ABORT:
            handle_file_abort(client, payload, hdr.payload_len);
            break;
        case PKT_CMD_JOIN_GRP:
            handle_join_group(client, hdr.recipient_or_group);
            break;
        case PKT_CMD_LEAVE_GRP:
            handle_leave_group(client, hdr.recipient_or_group);
            break;
        default:
            logger_log(LOG_LEVEL_WARN, "PROTO", "'%s' sent unsupported packet type %u",
                       client->name, (unsigned)hdr.type);
            reply_error(client, "Unsupported packet type");
            break;
        }
    }

    /* Announce departure, mark inactive so the main/reaper thread can join and free us */
    pthread_mutex_lock(&clients_mutex);
    if (registered) {
        abort_transfers_of_locked(client->name);
        if (!client->kicked && !stop_requested) {
            char out[PROTO_MAX_PAYLOAD];
            snprintf(out, sizeof(out), "%s has left the chat", client->name);
            fanout_locked(PKT_SERVER_INFO, "server", "", out, client->uid, NULL);
            printf("%s\n", out);
            fflush(stdout);
        }
        logger_log(LOG_LEVEL_INFO, "DISC", "Client '%s' disconnected", client->name);
    }
    client->name[0] = '\0';
    client->active = false;
    client_count--;
    close(client->sockfd);
    pthread_mutex_unlock(&clients_mutex);
    return NULL;
}

/* ---- interactive administrative console & graceful teardown (issue #15) ---- */

static void admin_users(void) {
    time_t now = time(NULL);
    printf("%-6s %-20s %-16s %-6s %s\n", "UID", "USERNAME", "IP", "PORT", "DURATION");
    printf("----------------------------------------------------------------\n");
    int count = 0;
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        const client_t *c = clients[i];
        if (c == NULL || !c->active || c->kicked || c->name[0] == '\0') {
            continue;
        }
        char ip[INET_ADDRSTRLEN];
        peer_ip(c, ip, sizeof(ip));
        long d = (long)(now - c->connected_at);
        if (d < 0) {
            d = 0;
        }
        printf("%-6d %-20s %-16s %-6d %02ld:%02ld:%02ld\n", c->uid, c->name, ip,
               (int)ntohs(c->address.sin_port), d / 3600, (d % 3600) / 60, d % 60);
        count++;
    }
    pthread_mutex_unlock(&clients_mutex);
    printf("Total active clients: %d\n", count);
}

static void admin_stats(void) {
    pthread_mutex_lock(&clients_mutex);
    long up = (long)(time(NULL) - server_started_at);
    if (up < 0) {
        up = 0;
    }
    int active = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->active && !clients[i]->kicked &&
            clients[i]->name[0] != '\0') {
            active++;
        }
    }
    int peak = peak_clients;
    uint64_t conns = total_connections;
    uint64_t pub = msg_public_count;
    uint64_t priv = msg_private_count;
    uint64_t grp = msg_group_count;
    uint64_t tx = bytes_tx;
    uint64_t rx = bytes_rx;
    pthread_mutex_unlock(&clients_mutex);

    printf("=== Server Statistics ===\n"
           "Uptime              : %ldd %02ldh %02ldm %02lds\n"
           "Active clients      : %d (peak: %d)\n"
           "Total connections   : %llu\n"
           "Messages relayed    : %llu (broadcast: %llu, private: %llu, group: %llu)\n"
           "Bytes TX / RX       : %llu / %llu\n"
           "Live traffic monitor: %s\n",
           up / 86400, (up % 86400) / 3600, (up % 3600) / 60, up % 60, active, peak,
           (unsigned long long)conns, (unsigned long long)(pub + priv + grp),
           (unsigned long long)pub, (unsigned long long)priv, (unsigned long long)grp,
           (unsigned long long)tx, (unsigned long long)rx, monitor_enabled ? "on" : "off");
}

static void admin_kick(const char *username) {
    if (username[0] == '\0') {
        printf("Usage: kick <username>\n");
        return;
    }
    pthread_mutex_lock(&clients_mutex);
    client_t *c = find_by_name_locked(username);
    if (c == NULL) {
        pthread_mutex_unlock(&clients_mutex);
        printf("User '%s' not found.\n", username);
        return;
    }
    c->kicked = true;
    send_to_locked(c, PKT_SERVER_ERROR, "server", c->name,
                   "You have been kicked by an administrator");
    shutdown(c->sockfd, SHUT_RDWR);
    char notice[PROTO_MAX_PAYLOAD];
    snprintf(notice, sizeof(notice), "%s was kicked by an administrator", c->name);
    fanout_locked(PKT_SERVER_INFO, "server", "", notice, c->uid, NULL);
    logger_log(LOG_LEVEL_WARN, "ADMIN", "Kicked user '%s' (uid %d)", c->name, c->uid);
    pthread_mutex_unlock(&clients_mutex);
    printf("Kicked '%s'.\n", username);
}

static void admin_broadcast(const char *msg) {
    if (msg[0] == '\0') {
        printf("Usage: broadcast <msg>\n");
        return;
    }
    char out[PROTO_MAX_PAYLOAD];
    snprintf(out, sizeof(out), "[ADMIN] %.2000s", msg);
    pthread_mutex_lock(&clients_mutex);
    fanout_locked(PKT_SERVER_INFO, "server", "", out, -1, NULL);
    msg_public_count++;
    logger_log(LOG_LEVEL_INFO, "ADMIN", "Broadcast: \"%s\"", msg);
    pthread_mutex_unlock(&clients_mutex);
    printf("%s\n", out);
}

static void admin_help(void) {
    printf("Admin commands:\n"
           "  help              show this list\n"
           "  users             table of active clients\n"
           "  stats             uptime, connection, message and byte counters\n"
           "  monitor [on|off]  show or set live traffic echoing\n"
           "  kick <username>   disconnect a user\n"
           "  broadcast <msg>   send an announcement to everyone\n"
           "  shutdown | quit   stop the server cleanly\n");
}

static void *admin_console_thread(void *arg) {
    (void)arg;
    char line[PROTO_MAX_PAYLOAD];
    while (!stop_requested) {
        struct pollfd pfd;
        pfd.fd = STDIN_FILENO;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int pr = poll(&pfd, 1, 50);
        if (pr < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (pr == 0) {
            continue;
        }
        if ((pfd.revents & (POLLIN | POLLHUP | POLLERR)) == 0) {
            continue;
        }
        if (fgets(line, (int)sizeof(line), stdin) == NULL) {
            break;
        }
        line[strcspn(line, "\r\n")] = '\0';
        char *cmd = line;
        while (*cmd == ' ') {
            cmd++;
        }
        if (*cmd == '\0') {
            continue;
        }
        char *arg_str = cmd;
        while (*arg_str != '\0' && *arg_str != ' ') {
            arg_str++;
        }
        if (*arg_str != '\0') {
            *arg_str++ = '\0';
            while (*arg_str == ' ') {
                arg_str++;
            }
        }
        if (strcmp(cmd, "help") == 0) {
            admin_help();
        } else if (strcmp(cmd, "users") == 0) {
            admin_users();
        } else if (strcmp(cmd, "stats") == 0) {
            admin_stats();
        } else if (strcmp(cmd, "monitor") == 0) {
            if (strcasecmp(arg_str, "on") == 0) {
                monitor_enabled = 1;
            } else if (strcasecmp(arg_str, "off") == 0) {
                monitor_enabled = 0;
            } else if (arg_str[0] != '\0') {
                printf("Usage: monitor [on|off]\n");
            }
            printf("Traffic monitor is %s.\n", monitor_enabled ? "on" : "off");
        } else if (strcmp(cmd, "kick") == 0) {
            admin_kick(arg_str);
        } else if (strcmp(cmd, "broadcast") == 0) {
            admin_broadcast(arg_str);
        } else if (strcmp(cmd, "shutdown") == 0 || strcmp(cmd, "quit") == 0) {
            logger_log(LOG_LEVEL_INFO, "ADMIN", "Shutdown requested from console");
            request_shutdown();
            break;
        } else {
            printf("Unknown command '%s'. Type 'help'.\n", cmd);
        }
        fflush(stdout);
    }
    return NULL;
}

/* Notifies every client, wakes their threads and joins them before returning. */
static void shutdown_all_clients(void) {
    pthread_mutex_lock(&clients_mutex);
    fanout_locked(PKT_SERVER_INFO, "server", "", "SERVER_SHUTDOWN: Server is shutting down", -1,
                  NULL);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->active) {
            shutdown(clients[i]->sockfd, SHUT_RDWR);
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        pthread_mutex_lock(&clients_mutex);
        client_t *c = clients[i];
        clients[i] = NULL;
        pthread_mutex_unlock(&clients_mutex);
        if (c != NULL) {
            pthread_join(c->tid, NULL);
            free(c);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: %s <IP> <Port> [logfile]\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    const char *log_path = (argc == 4) ? argv[3] : "server.log";

    if (!logger_init(log_path)) {
        fprintf(stderr, "Error: cannot open log file '%s'\n", log_path);
        return EXIT_FAILURE;
    }

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("Error: socket failed");
        logger_close();
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error: setsockopt(SO_REUSEADDR) failed");
        close(listen_fd);
        logger_close();
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Error: invalid IP address '%s'\n", ip);
        close(listen_fd);
        logger_close();
        return EXIT_FAILURE;
    }
    server_addr.sin_port = htons((uint16_t)port);

    signal(SIGPIPE, SIG_IGN);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_stop_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, (socklen_t)sizeof(server_addr)) < 0) {
        perror("Error: bind failed");
        close(listen_fd);
        logger_close();
        return EXIT_FAILURE;
    }

    if (listen(listen_fd, 16) < 0) {
        perror("Error: listen failed");
        close(listen_fd);
        logger_close();
        return EXIT_FAILURE;
    }

    server_started_at = time(NULL);
    logger_log(LOG_LEVEL_INFO, "SERV", "Server started on %s:%d (log: %s)", ip, port, log_path);
    printf("Server listening on %s:%d\n", ip, port);
    fflush(stdout);

    pthread_t console_tid;
    bool has_console = (pthread_create(&console_tid, NULL, admin_console_thread, NULL) == 0);

    while (!stop_requested) {
        struct sockaddr_in client_addr;
        socklen_t client_size = (socklen_t)sizeof(client_addr);
        int connfd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_size);
        if (connfd < 0) {
            if (stop_requested) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            perror("Error: accept failed");
            break;
        }

        client_t *newclient = (client_t *)calloc(1, sizeof(client_t));
        if (!newclient) {
            perror("Error: calloc failed for new client");
            close(connfd);
            continue;
        }
        newclient->address = client_addr;
        newclient->sockfd = connfd;
        newclient->uid = ++uid_counter;
        newclient->connected_at = time(NULL);
        newclient->active = true;

        if (!queue_client(newclient)) {
            fprintf(stderr, "Max clients reached. Rejected connection.\n");
            (void)packet_send(connfd, PKT_SERVER_ERROR, "server", "", "Server full", 11);
            close(connfd);
            free(newclient);
            continue;
        }

        if (pthread_create(&newclient->tid, NULL, handle_client, (void *)newclient) != 0) {
            perror("Error: pthread_create failed");
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == newclient) {
                    clients[i] = NULL;
                    client_count--;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            close(connfd);
            free(newclient);
            continue;
        }
    }

    close(listen_fd);
    listen_fd = -1;
    if (has_console) {
        pthread_join(console_tid, NULL);
    }
    shutdown_all_clients();
    logger_log(LOG_LEVEL_INFO, "SERV", "Server stopped cleanly");
    printf("Server stopped.\n");
    fflush(stdout);
    logger_close();
    return EXIT_SUCCESS;
}
