#define _POSIX_C_SOURCE 200809L
#include "fileshare.h"
#include "protocol.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static volatile sig_atomic_t flag = 0;
static int sockfd = 0;
static char name[PROTO_NAME_LEN];

/* Frames from the send thread and file-sender threads must not interleave on the socket. */
static pthread_mutex_t send_mu = PTHREAD_MUTEX_INITIALIZER;
/* Guards the file-transfer tables below. */
static pthread_mutex_t state_mu = PTHREAD_MUTEX_INITIALIZER;

static bool client_send(packet_type_t type, const char *target, const void *payload, size_t len) {
    pthread_mutex_lock(&send_mu);
    bool ok = packet_send(sockfd, type, name, target, payload, len);
    pthread_mutex_unlock(&send_mu);
    return ok;
}

/* ---- file transfer state ---- */

#define MAX_PENDING 8

typedef struct {
    bool used;
    uint32_t id;
    char sender[PROTO_NAME_LEN];
    char filename[FS_FILENAME_LEN];
    uint64_t size;
    uint32_t crc;
} incoming_t;

typedef struct {
    bool used;
    bool echoed;  /* server assigned an id */
    bool sending; /* chunk thread running */
    volatile bool cancelled;
    uint32_t id;
    char recipient[PROTO_NAME_LEN];
    char filename[FS_FILENAME_LEN];
    char path[FS_PATH_MAX];
    uint64_t size;
} outgoing_t;

typedef struct {
    bool used;
    fs_receiver_t rx;
    char sender[PROTO_NAME_LEN];
    char filename[FS_FILENAME_LEN];
    unsigned last_pct;
} receiving_t;

static incoming_t incoming[MAX_PENDING];
static int last_incoming = -1;
static outgoing_t outgoing[MAX_PENDING];
static receiving_t receiving[MAX_PENDING];

static void print_progress(const char *label, uint64_t done, uint64_t total) {
    char bar[48];
    fs_progress_bar(done, total, bar, sizeof(bar));
    printf("\r%s %s", bar, label);
    fflush(stdout);
}

static void send_ctl(packet_type_t type, const char *target, uint32_t id, const char *reason) {
    uint8_t ctl[128];
    size_t n = fs_ctl_encode(id, reason, ctl, sizeof(ctl));
    (void)client_send(type, target, ctl, n);
}

/* /sendfile: validate locally, then offer the file to the recipient via the server. */
static void start_offer(const char *recipient, const char *path) {
    uint64_t size = 0;
    uint32_t crc = 0;
    if (!fs_file_info(path, &size, &crc)) {
        printf("! Cannot read '%s' (not a readable regular file)\n", path);
        return;
    }
    if (size > FS_MAX_FILE_SIZE) {
        printf("! File too large (maximum is 50 MB)\n");
        return;
    }
    file_offer_t offer;
    memset(&offer, 0, sizeof(offer));
    char safe[FS_FILENAME_LEN];
    if (!fs_sanitize_filename(path, safe, sizeof(safe))) {
        printf("! Invalid file name\n");
        return;
    }
    pthread_mutex_lock(&state_mu);
    outgoing_t *slot = NULL;
    for (int i = 0; i < MAX_PENDING && slot == NULL; i++) {
        if (!outgoing[i].used) {
            slot = &outgoing[i];
        }
    }
    if (slot == NULL) {
        pthread_mutex_unlock(&state_mu);
        printf("! Too many transfers in progress\n");
        return;
    }
    memset(slot, 0, sizeof(*slot));
    slot->used = true;
    snprintf(slot->recipient, sizeof(slot->recipient), "%s", recipient);
    snprintf(slot->filename, sizeof(slot->filename), "%s", safe);
    snprintf(slot->path, sizeof(slot->path), "%s", path);
    slot->size = size;
    pthread_mutex_unlock(&state_mu);

    snprintf(offer.sender, sizeof(offer.sender), "%s", name);
    snprintf(offer.recipient, sizeof(offer.recipient), "%s", recipient);
    snprintf(offer.filename, sizeof(offer.filename), "%s", safe);
    offer.file_size = size;
    offer.checksum_crc32 = crc;
    fs_offer_to_net(&offer);
    if (!client_send(PKT_FILE_OFFER, recipient, &offer, sizeof(offer))) {
        pthread_mutex_lock(&state_mu);
        slot->used = false;
        pthread_mutex_unlock(&state_mu);
    }
}

/* Streams the file in 4 KB chunks once the recipient accepted. */
static void *file_sender_thread(void *arg) {
    outgoing_t *o = (outgoing_t *)arg;
    fs_sender_t tx;
    bool ok = fs_sender_open(&tx, o->path, o->id, o->size);
    uint8_t payload[FS_CHUNK_HEADER_LEN + FS_CHUNK_SIZE];
    size_t len = 0;
    unsigned last_pct = 101;
    while (ok && !o->cancelled && !tx.done) {
        ok = fs_sender_next(&tx, payload, &len) &&
             client_send(PKT_FILE_CHUNK, o->recipient, payload, len);
        unsigned pct = o->size == 0 ? 100u : (unsigned)(tx.sent * 100u / o->size);
        if (ok && (pct != last_pct) && (pct % 5 == 0)) {
            last_pct = pct;
            print_progress(o->filename, tx.sent, o->size);
        }
    }
    bool cancelled = o->cancelled;
    fs_sender_close(&tx);
    if (cancelled) {
        printf("\n* Transfer of '%s' cancelled\n", o->filename);
    } else if (!ok) {
        send_ctl(PKT_FILE_ABORT, o->recipient, o->id, "sender I/O error");
        printf("\n! Failed to send '%s'\n", o->filename);
    } else {
        printf("\n* Sent '%s' to %s\n", o->filename, o->recipient);
    }
    fflush(stdout);
    pthread_mutex_lock(&state_mu);
    o->used = false;
    o->sending = false;
    pthread_mutex_unlock(&state_mu);
    return NULL;
}

static void answer_offer(uint32_t id, bool accept) {
    pthread_mutex_lock(&state_mu);
    incoming_t *in = NULL;
    for (int i = 0; i < MAX_PENDING; i++) {
        if (incoming[i].used && incoming[i].id == id) {
            in = &incoming[i];
        }
    }
    if (in == NULL) {
        pthread_mutex_unlock(&state_mu);
        printf("! No pending file offer with id %u\n", id);
        return;
    }
    char sender[PROTO_NAME_LEN];
    snprintf(sender, sizeof(sender), "%s", in->sender);
    bool ok = accept;
    if (accept) {
        receiving_t *slot = NULL;
        for (int i = 0; i < MAX_PENDING && slot == NULL; i++) {
            if (!receiving[i].used) {
                slot = &receiving[i];
            }
        }
        ok = slot != NULL &&
             fs_receiver_open(&slot->rx, FS_DOWNLOAD_DIR, id, in->filename, in->size, in->crc);
        if (ok) {
            slot->used = true;
            slot->last_pct = 101;
            snprintf(slot->sender, sizeof(slot->sender), "%s", in->sender);
            snprintf(slot->filename, sizeof(slot->filename), "%s", in->filename);
        } else {
            printf("! Cannot create ./%s/ for the download; rejecting\n", FS_DOWNLOAD_DIR);
        }
    }
    in->used = false;
    if (last_incoming >= 0 && !incoming[last_incoming].used) {
        last_incoming = -1;
        for (int i = 0; i < MAX_PENDING; i++) {
            if (incoming[i].used) {
                last_incoming = i;
            }
        }
    }
    pthread_mutex_unlock(&state_mu);
    send_ctl(ok ? PKT_FILE_ACCEPT : PKT_FILE_REJECT, sender, id, NULL);
    printf("* %s file %u from %s\n", ok ? "Accepted" : "Rejected", id, sender);
    fflush(stdout);
}

/* Bare "y"/"n" answers the most recent pending offer; returns true if consumed. */
static bool try_quick_answer(const char *line) {
    char word[8];
    size_t n = strcspn(line, "\r\n");
    if (n == 0 || n >= sizeof(word)) {
        return false;
    }
    memcpy(word, line, n);
    word[n] = '\0';
    bool yes = strcasecmp(word, "y") == 0 || strcasecmp(word, "yes") == 0;
    bool no = strcasecmp(word, "n") == 0 || strcasecmp(word, "no") == 0;
    if (!yes && !no) {
        return false;
    }
    pthread_mutex_lock(&state_mu);
    int idx = last_incoming;
    uint32_t id = idx >= 0 ? incoming[idx].id : 0;
    pthread_mutex_unlock(&state_mu);
    if (idx < 0) {
        return false;
    }
    answer_offer(id, yes);
    return true;
}

static void on_file_offer(const packet_header_t *hdr, const char *payload) {
    file_offer_t offer;
    if (hdr->payload_len != sizeof(offer)) {
        return;
    }
    memcpy(&offer, payload, sizeof(offer));
    fs_offer_to_host(&offer);
    pthread_mutex_lock(&state_mu);
    if (strcmp(hdr->sender, name) == 0) { /* echo of our own offer: learn the id */
        for (int i = 0; i < MAX_PENDING; i++) {
            outgoing_t *o = &outgoing[i];
            if (o->used && !o->echoed && strcmp(o->recipient, offer.recipient) == 0 &&
                strcmp(o->filename, offer.filename) == 0 && o->size == offer.file_size) {
                o->id = offer.transfer_id;
                o->echoed = true;
                printf("* Offered '%s' to %s (id %u), waiting for response...\n", o->filename,
                       o->recipient, o->id);
                break;
            }
        }
        pthread_mutex_unlock(&state_mu);
        fflush(stdout);
        return;
    }
    char safe[FS_FILENAME_LEN];
    incoming_t *slot = NULL;
    for (int i = 0; i < MAX_PENDING && slot == NULL; i++) {
        if (!incoming[i].used) {
            slot = &incoming[i];
            if (fs_sanitize_filename(offer.filename, safe, sizeof(safe)) &&
                offer.file_size <= FS_MAX_FILE_SIZE) {
                slot->used = true;
                slot->id = offer.transfer_id;
                snprintf(slot->sender, sizeof(slot->sender), "%s", hdr->sender);
                snprintf(slot->filename, sizeof(slot->filename), "%s", safe);
                slot->size = offer.file_size;
                slot->crc = offer.checksum_crc32;
                last_incoming = i;
            } else {
                slot = NULL;
                break;
            }
        }
    }
    if (slot == NULL) { /* invalid or no room: reject automatically */
        pthread_mutex_unlock(&state_mu);
        send_ctl(PKT_FILE_REJECT, hdr->sender, offer.transfer_id, NULL);
        printf("* Auto-rejected an invalid or excess file offer from %s\n", hdr->sender);
        fflush(stdout);
        return;
    }
    char size_str[32];
    fs_format_size(slot->size, size_str, sizeof(size_str));
    printf(
        "* User '%s' wants to send '%s' (%s). Accept? [y/n]  (/fileaccept %u or /filereject %u)\n",
        slot->sender, slot->filename, size_str, slot->id, slot->id);
    pthread_mutex_unlock(&state_mu);
    fflush(stdout);
}

static void on_file_response(packet_type_t type, const char *payload, size_t len,
                             const char *from) {
    uint32_t id = 0;
    if (!fs_ctl_decode(payload, len, &id, NULL)) {
        return;
    }
    pthread_mutex_lock(&state_mu);
    for (int i = 0; i < MAX_PENDING; i++) {
        outgoing_t *o = &outgoing[i];
        if (!o->used || !o->echoed || o->id != id) {
            continue;
        }
        if (type == PKT_FILE_ACCEPT && !o->sending) {
            o->sending = true;
            printf("* %s accepted '%s', sending...\n", from, o->filename);
            pthread_t t;
            if (pthread_create(&t, NULL, file_sender_thread, o) == 0) {
                pthread_detach(t);
            } else {
                o->sending = false;
                o->used = false;
                printf("! Could not start the file sender thread\n");
            }
        } else if (type == PKT_FILE_REJECT) {
            printf("* %s rejected '%s'\n", from, o->filename);
            o->used = false;
        }
    }
    pthread_mutex_unlock(&state_mu);
    fflush(stdout);
}

static void on_file_abort(const char *payload, size_t len) {
    uint32_t id = 0;
    const char *reason = "";
    if (!fs_ctl_decode(payload, len, &id, &reason)) {
        return;
    }
    pthread_mutex_lock(&state_mu);
    for (int i = 0; i < MAX_PENDING; i++) {
        if (outgoing[i].used && outgoing[i].echoed && outgoing[i].id == id) {
            printf("\n! Transfer of '%s' aborted: %s\n", outgoing[i].filename, reason);
            outgoing[i].cancelled = true;
            if (!outgoing[i].sending) {
                outgoing[i].used = false;
            }
        }
        if (receiving[i].used && receiving[i].rx.id == id) {
            fs_receiver_abort(&receiving[i].rx);
            receiving[i].used = false;
            printf("\n! Download of '%s' aborted: %s\n", receiving[i].filename, reason);
        }
        if (incoming[i].used && incoming[i].id == id) {
            incoming[i].used = false;
            printf("* Offer of '%s' from %s was withdrawn\n", incoming[i].filename,
                   incoming[i].sender);
        }
    }
    pthread_mutex_unlock(&state_mu);
    fflush(stdout);
}

static void on_file_chunk(const char *payload, size_t len) {
    fs_chunk_view_t v;
    if (!fs_chunk_decode(payload, len, &v)) {
        return;
    }
    pthread_mutex_lock(&state_mu);
    receiving_t *r = NULL;
    for (int i = 0; i < MAX_PENDING; i++) {
        if (receiving[i].used && receiving[i].rx.id == v.transfer_id) {
            r = &receiving[i];
        }
    }
    if (r == NULL) {
        pthread_mutex_unlock(&state_mu);
        return;
    }
    fs_chunk_status_t st = fs_receiver_chunk(&r->rx, &v);
    if (st == FS_CHUNK_MORE) {
        unsigned pct = r->rx.expected_size == 0
                           ? 100u
                           : (unsigned)(r->rx.received * 100u / r->rx.expected_size);
        if (pct != r->last_pct && pct % 5 == 0) {
            r->last_pct = pct;
            print_progress(r->filename, r->rx.received, r->rx.expected_size);
        }
    } else if (st == FS_CHUNK_DONE) {
        print_progress(r->filename, r->rx.expected_size, r->rx.expected_size);
        printf("\n* Saved %s (%llu bytes, checksum OK)\n", r->rx.final_path,
               (unsigned long long)r->rx.expected_size);
        r->used = false;
    } else {
        static const char *why[] = {"",
                                    "",
                                    "chunk out of sequence",
                                    "invalid chunk size",
                                    "checksum mismatch, file discarded",
                                    "disk write error"};
        printf("\n! Download of '%s' failed: %s\n", r->filename, why[st]);
        uint32_t id = r->rx.id;
        char sender[PROTO_NAME_LEN];
        snprintf(sender, sizeof(sender), "%s", r->sender);
        fs_receiver_abort(&r->rx);
        r->used = false;
        pthread_mutex_unlock(&state_mu);
        send_ctl(PKT_FILE_ABORT, sender, id, why[st]);
        fflush(stdout);
        return;
    }
    pthread_mutex_unlock(&state_mu);
    fflush(stdout);
}

static void str_trim_lf(char *arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n' || arr[i] == '\r') {
            arr[i] = '\0';
            break;
        }
    }
}

static void print_help(void) {
    printf("Commands:\n"
           "  <text>              send to everyone\n"
           "  /msg <user> <text>  private message\n"
           "  /group <name>       join a group channel\n"
           "  #<name> <text>      message a group you joined\n"
           "  /leave <name>       leave a group\n"
           "  /users              list connected users\n"
           "  /sendfile <user> <path>  offer a file (max 50 MB)\n"
           "  y / n               accept / reject the latest file offer\n"
           "  /fileaccept <id>    accept a file offer\n"
           "  /filereject <id>    reject a file offer\n"
           "  /help               show this help\n"
           "  /exit               disconnect\n");
    fflush(stdout);
}

static void catch_sigint(int sig) {
    (void)sig;
    flag = 1;
    if (sockfd > 0) {
        shutdown(sockfd, SHUT_RDWR);
    }
}

static void *send_msg_handler(void *arg) {
    (void)arg;
    char line[PROTO_MAX_PAYLOAD + 64];

    while (!flag) {
        if (fgets(line, (int)sizeof(line), stdin) == NULL) {
            break;
        }
        if (try_quick_answer(line)) {
            continue;
        }
        parsed_input_t in;
        proto_parse_input(line, &in);
        if (in.action == INPUT_EXIT) {
            break;
        }
        if (in.action == INPUT_HELP) {
            print_help();
        } else if (in.action == INPUT_ERROR) {
            printf("! %s\n", in.error);
            fflush(stdout);
        } else if (in.action == INPUT_SENDFILE) {
            start_offer(in.target, in.text);
        } else if (in.action == INPUT_FILE_ACCEPT || in.action == INPUT_FILE_REJECT) {
            char *end = NULL;
            unsigned long id = strtoul(in.text, &end, 10);
            if (end == in.text || *end != '\0') {
                printf("! Invalid transfer id '%s'\n", in.text);
            } else {
                answer_offer((uint32_t)id, in.action == INPUT_FILE_ACCEPT);
            }
            fflush(stdout);
        } else if (in.action == INPUT_SEND) {
            if (!client_send(in.type, in.target, in.text, strlen(in.text))) {
                break;
            }
        }
    }

    flag = 1;
    (void)client_send(PKT_DISCONNECT, "", NULL, 0);
    shutdown(sockfd, SHUT_RDWR);
    return NULL;
}

static void *receive_msg_handler(void *arg) {
    (void)arg;
    packet_header_t hdr;
    char payload[PROTO_MAX_PAYLOAD + 1];

    while (!flag) {
        if (packet_recv(sockfd, &hdr, payload) != 1) {
            break;
        }
        switch (hdr.type) {
        case PKT_MSG_PUBLIC:
            printf("%s: %s\n", hdr.sender, payload);
            break;
        case PKT_MSG_PRIVATE:
            printf("[private from %s] %s\n", hdr.sender, payload);
            break;
        case PKT_MSG_GROUP:
            printf("[#%s] %s: %s\n", hdr.recipient_or_group, hdr.sender, payload);
            break;
        case PKT_SERVER_ERROR:
            printf("! %s\n", payload);
            break;
        case PKT_FILE_OFFER:
            on_file_offer(&hdr, payload);
            break;
        case PKT_FILE_ACCEPT:
        case PKT_FILE_REJECT:
            on_file_response((packet_type_t)hdr.type, payload, hdr.payload_len, hdr.sender);
            break;
        case PKT_FILE_CHUNK:
            on_file_chunk(payload, hdr.payload_len);
            break;
        case PKT_FILE_ABORT:
            on_file_abort(payload, hdr.payload_len);
            break;
        default:
            printf("* %s\n", payload);
            break;
        }
        fflush(stdout);
    }

    flag = 1;
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <Port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = catch_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    printf("Enter your name: ");
    fflush(stdout);
    if (fgets(name, (int)sizeof(name), stdin) == NULL) {
        return EXIT_FAILURE;
    }
    str_trim_lf(name, (int)strlen(name));

    while (!proto_valid_name(name)) {
        printf(
            "Name must be 2-%d characters of letters, digits, '_', '-' or '.'.\nEnter your name: ",
            PROTO_NAME_LEN - 2);
        fflush(stdout);
        if (fgets(name, (int)sizeof(name), stdin) == NULL) {
            return EXIT_FAILURE;
        }
        str_trim_lf(name, (int)strlen(name));
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Error: invalid IP address '%s'\n", ip);
        return EXIT_FAILURE;
    }
    server_addr.sin_port = htons((uint16_t)port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error: socket creation failed");
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, (socklen_t)sizeof(server_addr)) == -1) {
        perror("Error: connect to server failed");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (!client_send(PKT_HANDSHAKE, "", NULL, 0)) {
        perror("Error: failed to send handshake");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("=== Connected to server ===\n");
    fflush(stdout);

    pthread_t recv_thread;
    pthread_t send_thread;
    if (pthread_create(&recv_thread, NULL, receive_msg_handler, NULL) != 0) {
        perror("Error: pthread_create for receive failed");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (pthread_create(&send_thread, NULL, send_msg_handler, NULL) != 0) {
        perror("Error: pthread_create for send failed");
        flag = 1;
        shutdown(sockfd, SHUT_RDWR);
        pthread_join(recv_thread, NULL);
        close(sockfd);
        return EXIT_FAILURE;
    }
    /* The send thread may be blocked in fgets() if the server hung up; do not wait for it. */
    pthread_detach(send_thread);

    pthread_join(recv_thread, NULL);

    printf("\nDisconnected from server.\n");
    close(sockfd);
    return EXIT_SUCCESS;
}
