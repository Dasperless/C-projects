#define _POSIX_C_SOURCE 200809L
#include "logger.h"
#include "protocol.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CLIENTS 50
#define MAX_GROUPS_PER_CLIENT 8

typedef struct {
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[PROTO_NAME_LEN]; /* empty until the handshake succeeds */
    char groups[MAX_GROUPS_PER_CLIENT][PROTO_NAME_LEN];
    int group_count;
} client_t;

static int client_count = 0;
static int uid_counter = 10;
static client_t *clients[MAX_CLIENTS] = {NULL};
/* Guards the clients table and every socket write (so frames never interleave). */
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ---- helpers: *_locked functions require clients_mutex to be held ---- */

static void send_to_locked(const client_t *to, packet_type_t type, const char *sender,
                           const char *target, const char *text) {
    (void)packet_send(to->sockfd, type, sender, target, text, strlen(text));
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

static void queue_client(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == NULL) {
            clients[i] = cl;
            client_count++;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

static void remove_client(int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->uid == uid) {
            clients[i] = NULL;
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
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
        logger_log(LOG_LEVEL_INFO, "PRIV", "'%s' -> '%s': \"%s\"", client->name, dest->name, text);
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
        logger_log(LOG_LEVEL_INFO, "GRP", "'%s' -> #%s: \"%s\"", client->name, group, text);
    }
    pthread_mutex_unlock(&clients_mutex);
}

/* Performs the handshake. Returns true when the client is registered under a unique name. */
static bool do_handshake(client_t *client) {
    packet_header_t hdr;
    char payload[PROTO_MAX_PAYLOAD + 1];
    if (packet_recv(client->sockfd, &hdr, payload) != 1 || hdr.type != PKT_HANDSHAKE) {
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
        int r = packet_recv(client->sockfd, &hdr, payload);
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
                logger_log(LOG_LEVEL_INFO, "MSG", "'%s' -> ALL: \"%s\"", client->name, payload);
                pthread_mutex_unlock(&clients_mutex);
                printf("%s: %s\n", client->name, payload);
                fflush(stdout);
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

    /* Announce departure, deregister, release resources */
    if (registered) {
        char out[PROTO_MAX_PAYLOAD];
        snprintf(out, sizeof(out), "%s has left the chat", client->name);
        pthread_mutex_lock(&clients_mutex);
        fanout_locked(PKT_SERVER_INFO, "server", "", out, client->uid, NULL);
        logger_log(LOG_LEVEL_INFO, "DISC", "Client '%s' disconnected", client->name);
        pthread_mutex_unlock(&clients_mutex);
        printf("%s\n", out);
        fflush(stdout);
    }
    remove_client(client->uid);
    close(client->sockfd);
    free(client);
    return NULL;
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

    int listefd = socket(AF_INET, SOCK_STREAM, 0);
    if (listefd < 0) {
        perror("Error: socket failed");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(listefd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error: setsockopt(SO_REUSEADDR) failed");
        close(listefd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Error: invalid IP address '%s'\n", ip);
        close(listefd);
        return EXIT_FAILURE;
    }
    server_addr.sin_port = htons((uint16_t)port);

    signal(SIGPIPE, SIG_IGN);

    if (bind(listefd, (struct sockaddr *)&server_addr, (socklen_t)sizeof(server_addr)) < 0) {
        perror("Error: bind failed");
        close(listefd);
        return EXIT_FAILURE;
    }

    if (listen(listefd, 10) < 0) {
        perror("Error: listen failed");
        close(listefd);
        return EXIT_FAILURE;
    }

    logger_log(LOG_LEVEL_INFO, "SERV", "Server started on %s:%d (log: %s)", ip, port, log_path);
    printf("Server listening on %s:%d\n", ip, port);
    fflush(stdout);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_size = (socklen_t)sizeof(client_addr);
        int connfd = accept(listefd, (struct sockaddr *)&client_addr, &client_size);
        if (connfd < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("Error: accept failed");
            break;
        }

        pthread_mutex_lock(&clients_mutex);
        if (client_count >= MAX_CLIENTS) {
            pthread_mutex_unlock(&clients_mutex);
            fprintf(stderr, "Max clients reached. Rejected connection.\n");
            (void)packet_send(connfd, PKT_SERVER_ERROR, "server", "", "Server full", 11);
            close(connfd);
            continue;
        }
        pthread_mutex_unlock(&clients_mutex);

        client_t *newclient = (client_t *)calloc(1, sizeof(client_t));
        if (!newclient) {
            perror("Error: calloc failed for new client");
            close(connfd);
            continue;
        }
        newclient->address = client_addr;
        newclient->sockfd = connfd;
        newclient->uid = ++uid_counter;

        queue_client(newclient);

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *)newclient) != 0) {
            perror("Error: pthread_create failed");
            remove_client(newclient->uid);
            close(connfd);
            free(newclient);
            continue;
        }
        pthread_detach(tid);
    }

    close(listefd);
    logger_log(LOG_LEVEL_INFO, "SERV", "Server stopped");
    logger_close();
    return EXIT_SUCCESS;
}
