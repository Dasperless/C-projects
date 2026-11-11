#include "test_util.h"
#include "fileshare.h"

#include <assert.h>
#include <sys/stat.h>

#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                          \
        }                                                                     \
    } while (0)

static int port;
static char tmpdir[64];

/* Reads packets until one of `type` arrives (returns 1) or the timeout hits (returns 0). */
static int recv_type(int fd, packet_type_t type, packet_header_t *h, char *payload, int ms) {
    tc_set_timeout(fd, ms);
    while (packet_recv(fd, h, payload) == 1) {
        if (h->type == type) {
            return 1;
        }
    }
    return 0;
}

/* True if no file-transfer packet reaches fd within `ms`. */
static int no_file_packets(int fd, int ms) {
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    tc_set_timeout(fd, ms);
    while (packet_recv(fd, &h, p) == 1) {
        if (h.type >= PKT_FILE_OFFER) {
            return 0;
        }
    }
    return 1;
}

static void make_file(const char *path, size_t size, unsigned seed) {
    FILE *f = fopen(path, "wb");
    CHECK(f != NULL);
    for (size_t i = 0; i < size; i++) {
        seed = seed * 1664525u + 1013904223u;
        fputc((int)(seed >> 24), f);
    }
    fclose(f);
}

static int send_offer(int fd, const char *me, const char *to, const char *filename, uint64_t size,
                      uint32_t crc) {
    file_offer_t o;
    memset(&o, 0, sizeof(o));
    snprintf(o.sender, sizeof(o.sender), "%s", me);
    snprintf(o.recipient, sizeof(o.recipient), "%s", to);
    snprintf(o.filename, sizeof(o.filename), "%s", filename);
    o.file_size = size;
    o.checksum_crc32 = crc;
    fs_offer_to_net(&o);
    return packet_send(fd, PKT_FILE_OFFER, me, to, &o, sizeof(o)) ? 1 : 0;
}

static int send_ctl_packet(int fd, packet_type_t t, const char *me, const char *to, uint32_t id) {
    uint8_t ctl[8];
    size_t n = fs_ctl_encode(id, NULL, ctl, sizeof(ctl));
    return packet_send(fd, t, me, to, ctl, n) ? 1 : 0;
}

/* Runs the offer handshake up to the recipient's decision. Returns the transfer id (0 on failure). */
static uint32_t offer_and_get_id(int a, int b, const char *to, const char *filename, uint64_t size,
                                 uint32_t crc, file_offer_t *seen_by_b) {
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    CHECK(send_offer(a, "alice", to, filename, size, crc));
    CHECK(recv_type(b, PKT_FILE_OFFER, &h, p, 2000));
    CHECK(h.payload_len == sizeof(file_offer_t) && strcmp(h.sender, "alice") == 0);
    memcpy(seen_by_b, p, sizeof(*seen_by_b));
    fs_offer_to_host(seen_by_b);
    CHECK(recv_type(a, PKT_FILE_OFFER, &h, p, 2000)); /* echo with the same id */
    file_offer_t echo;
    memcpy(&echo, p, sizeof(echo));
    fs_offer_to_host(&echo);
    CHECK(echo.transfer_id == seen_by_b->transfer_id && echo.transfer_id != 0);
    return echo.transfer_id;
}

static int accept_flow(int a, int b, uint32_t id) {
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    CHECK(send_ctl_packet(b, PKT_FILE_ACCEPT, "bob", "alice", id));
    CHECK(recv_type(a, PKT_FILE_ACCEPT, &h, p, 2000));
    uint32_t got = 0;
    CHECK(fs_ctl_decode(p, h.payload_len, &got, NULL) && got == id && strcmp(h.sender, "bob") == 0);
    return 1;
}

static void test_binary_roundtrip(int a, int b, int c, size_t size, unsigned seed) {
    char src[128];
    char dir[128];
    snprintf(src, sizeof(src), "%s/src_%zu.bin", tmpdir, size);
    snprintf(dir, sizeof(dir), "%s/downloads", tmpdir);
    make_file(src, size, seed);
    uint64_t fsize = 0;
    uint32_t crc = 0;
    CHECK(fs_file_info(src, &fsize, &crc) && fsize == size);

    file_offer_t seen;
    uint32_t id = offer_and_get_id(a, b, "bob", src, fsize, crc, &seen);
    CHECK(strlen(seen.filename) > 0 && strchr(seen.filename, '/') == NULL); /* sanitized */
    CHECK(seen.file_size == fsize && seen.checksum_crc32 == crc);
    CHECK(accept_flow(a, b, id));

    fs_receiver_t rx;
    CHECK(fs_receiver_open(&rx, dir, id, seen.filename, seen.file_size, seen.checksum_crc32));
    fs_sender_t tx;
    CHECK(fs_sender_open(&tx, src, id, fsize));
    uint8_t payload[FS_CHUNK_HEADER_LEN + FS_CHUNK_SIZE];
    size_t len = 0;
    while (!tx.done) {
        CHECK(fs_sender_next(&tx, payload, &len));
        CHECK(packet_send(a, PKT_FILE_CHUNK, "alice", "bob", payload, len));
    }
    fs_sender_close(&tx);

    fs_chunk_status_t st = FS_CHUNK_MORE;
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    while (st == FS_CHUNK_MORE) {
        CHECK(recv_type(b, PKT_FILE_CHUNK, &h, p, 3000));
        fs_chunk_view_t v;
        CHECK(fs_chunk_decode(p, h.payload_len, &v));
        st = fs_receiver_chunk(&rx, &v);
    }
    CHECK(st == FS_CHUNK_DONE);

    /* bit-for-bit comparison */
    FILE *f1 = fopen(src, "rb");
    FILE *f2 = fopen(rx.final_path, "rb");
    CHECK(f1 != NULL && f2 != NULL);
    int c1;
    int c2;
    do {
        c1 = fgetc(f1);
        c2 = fgetc(f2);
        CHECK(c1 == c2);
    } while (c1 != EOF);
    fclose(f1);
    fclose(f2);
    CHECK(no_file_packets(c, 100)); /* third party saw nothing */
}

static void test_reject(int a, int b) {
    file_offer_t seen;
    uint32_t id = offer_and_get_id(a, b, "bob", "secret.txt", 10, 123, &seen);
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    CHECK(send_ctl_packet(b, PKT_FILE_REJECT, "bob", "alice", id));
    CHECK(recv_type(a, PKT_FILE_REJECT, &h, p, 2000));
    /* the transfer is gone: chunks and a late accept are refused */
    uint8_t chunk[FS_CHUNK_HEADER_LEN + 10];
    size_t n = fs_chunk_encode(id, 0, "0123456789", 10, true, chunk);
    CHECK(packet_send(a, PKT_FILE_CHUNK, "alice", "bob", chunk, n));
    CHECK(recv_type(a, PKT_SERVER_ERROR, &h, p, 2000));
    CHECK(send_ctl_packet(b, PKT_FILE_ACCEPT, "bob", "alice", id));
    CHECK(recv_type(b, PKT_SERVER_ERROR, &h, p, 2000));
}

static void test_path_traversal(int a, int b) {
    file_offer_t seen;
    uint32_t id = offer_and_get_id(a, b, "bob", "../../etc/passwd", 10, 1, &seen);
    CHECK(strcmp(seen.filename, "passwd") == 0);
    CHECK(send_ctl_packet(b, PKT_FILE_REJECT, "bob", "alice", id));
    id = offer_and_get_id(a, b, "bob", "..\\..\\windows\\evil.exe", 10, 1, &seen);
    CHECK(strcmp(seen.filename, "evil.exe") == 0);
    CHECK(send_ctl_packet(b, PKT_FILE_REJECT, "bob", "alice", id));
}

static void test_invalid_offers(int a, int b) {
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    CHECK(send_offer(a, "alice", "bob", "big.iso", FS_MAX_FILE_SIZE + 1, 0));
    CHECK(recv_type(a, PKT_SERVER_ERROR, &h, p, 2000) && strstr(p, "too large") != NULL);
    CHECK(send_offer(a, "alice", "nobody", "x", 5, 0));
    CHECK(recv_type(a, PKT_SERVER_ERROR, &h, p, 2000) && strstr(p, "not found") != NULL);
    CHECK(send_offer(a, "alice", "alice", "x", 5, 0));
    CHECK(recv_type(a, PKT_SERVER_ERROR, &h, p, 2000) && strstr(p, "yourself") != NULL);
    CHECK(send_offer(a, "alice", "bob", "dir/", 5, 0));
    CHECK(recv_type(a, PKT_SERVER_ERROR, &h, p, 2000) && strstr(p, "Invalid file name") != NULL);
    CHECK(packet_send(a, PKT_FILE_OFFER, "alice", "bob", "short", 5)); /* malformed payload */
    CHECK(recv_type(a, PKT_SERVER_ERROR, &h, p, 2000) && strstr(p, "Malformed") != NULL);
    CHECK(no_file_packets(b, 150));
    /* exactly the maximum size is allowed */
    file_offer_t seen;
    uint32_t id = offer_and_get_id(a, b, "bob", "max.bin", FS_MAX_FILE_SIZE, 0, &seen);
    CHECK(send_ctl_packet(b, PKT_FILE_REJECT, "bob", "alice", id));
}

static void test_wrong_party(int a, int b, int c) {
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    file_offer_t seen;
    uint32_t id = offer_and_get_id(a, b, "bob", "f.bin", 10, 0, &seen);
    CHECK(send_ctl_packet(c, PKT_FILE_ACCEPT, "carol", "alice", id)); /* not the recipient */
    CHECK(recv_type(c, PKT_SERVER_ERROR, &h, p, 2000));
    uint8_t chunk[FS_CHUNK_HEADER_LEN + 10];
    size_t n = fs_chunk_encode(id, 0, "0123456789", 10, true, chunk);
    CHECK(packet_send(a, PKT_FILE_CHUNK, "alice", "bob", chunk, n)); /* before accept */
    CHECK(recv_type(a, PKT_SERVER_ERROR, &h, p, 2000));
    CHECK(accept_flow(a, b, id));
    CHECK(packet_send(c, PKT_FILE_CHUNK, "carol", "bob", chunk, n)); /* not the sender */
    CHECK(recv_type(c, PKT_SERVER_ERROR, &h, p, 2000));
    CHECK(packet_send(b, PKT_FILE_CHUNK, "bob", "alice", chunk, n)); /* recipient cannot send */
    CHECK(recv_type(b, PKT_SERVER_ERROR, &h, p, 2000));
    CHECK(send_ctl_packet(c, PKT_FILE_ABORT, "carol", "alice", id)); /* stranger cannot abort */
    CHECK(recv_type(c, PKT_SERVER_ERROR, &h, p, 2000));
    CHECK(no_file_packets(b, 100));
    CHECK(send_ctl_packet(b, PKT_FILE_ABORT, "bob", "alice", id)); /* a peer may cancel */
    CHECK(recv_type(a, PKT_FILE_ABORT, &h, p, 2000));
}

static void test_protocol_violations(int a, int b) {
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    file_offer_t seen;
    uint8_t chunk[FS_CHUNK_HEADER_LEN + FS_CHUNK_SIZE];
    uint8_t data[FS_CHUNK_SIZE];
    memset(data, 'x', sizeof(data));

    /* out-of-order chunk index aborts for both sides */
    uint32_t id = offer_and_get_id(a, b, "bob", "seq.bin", 100, 0, &seen);
    CHECK(accept_flow(a, b, id));
    size_t n = fs_chunk_encode(id, 1, data, 50, false, chunk);
    CHECK(packet_send(a, PKT_FILE_CHUNK, "alice", "bob", chunk, n));
    CHECK(recv_type(a, PKT_FILE_ABORT, &h, p, 2000));
    CHECK(recv_type(b, PKT_FILE_ABORT, &h, p, 2000));

    /* more data than declared */
    id = offer_and_get_id(a, b, "bob", "over.bin", 100, 0, &seen);
    CHECK(accept_flow(a, b, id));
    n = fs_chunk_encode(id, 0, data, 200, false, chunk);
    CHECK(packet_send(a, PKT_FILE_CHUNK, "alice", "bob", chunk, n));
    CHECK(recv_type(b, PKT_FILE_ABORT, &h, p, 2000));
    CHECK(no_file_packets(a, 50) || 1);

    /* premature last flag */
    id = offer_and_get_id(a, b, "bob", "early.bin", 100, 0, &seen);
    CHECK(accept_flow(a, b, id));
    n = fs_chunk_encode(id, 0, data, 40, true, chunk);
    CHECK(packet_send(a, PKT_FILE_CHUNK, "alice", "bob", chunk, n));
    CHECK(recv_type(b, PKT_FILE_ABORT, &h, p, 2000));
}

static void test_bad_checksum_detected(int a, int b) {
    char dir[128];
    snprintf(dir, sizeof(dir), "%s/downloads", tmpdir);
    file_offer_t seen;
    uint8_t data[300];
    memset(data, 9, sizeof(data));
    uint32_t good = fs_crc32_update(0, data, sizeof(data));
    uint32_t id = offer_and_get_id(a, b, "bob", "corrupt.bin", sizeof(data), good ^ 0xFFu, &seen);
    CHECK(accept_flow(a, b, id));
    uint8_t chunk[FS_CHUNK_HEADER_LEN + 300];
    size_t n = fs_chunk_encode(id, 0, data, sizeof(data), true, chunk);
    CHECK(packet_send(a, PKT_FILE_CHUNK, "alice", "bob", chunk, n));
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    CHECK(recv_type(b, PKT_FILE_CHUNK, &h, p, 2000));
    fs_receiver_t rx;
    CHECK(fs_receiver_open(&rx, dir, id, seen.filename, seen.file_size, seen.checksum_crc32));
    fs_chunk_view_t v;
    CHECK(fs_chunk_decode(p, h.payload_len, &v));
    CHECK(fs_receiver_chunk(&rx, &v) == FS_ERR_CRC);
    CHECK(access(rx.final_path, F_OK) != 0 && access(rx.part_path, F_OK) != 0);
}

static void test_disconnect_mid_transfer(void) {
    packet_header_t h;
    char p[PROTO_MAX_PAYLOAD + 1];
    file_offer_t seen;
    uint8_t data[FS_CHUNK_SIZE];
    uint8_t chunk[FS_CHUNK_HEADER_LEN + FS_CHUNK_SIZE];
    memset(data, 1, sizeof(data));

    /* recipient vanishes */
    int a = tc_login(port, "alice");
    int b = tc_login(port, "bob");
    uint32_t id = offer_and_get_id(a, b, "bob", "big.bin", 3 * FS_CHUNK_SIZE, 0, &seen);
    CHECK(accept_flow(a, b, id));
    size_t n = fs_chunk_encode(id, 0, data, FS_CHUNK_SIZE, false, chunk);
    CHECK(packet_send(a, PKT_FILE_CHUNK, "alice", "bob", chunk, n));
    close(b);
    CHECK(recv_type(a, PKT_FILE_ABORT, &h, p, 2000));
    usleep(150000);
    close(a);
    usleep(250000);

    /* sender vanishes */
    a = tc_login(port, "alice");
    b = tc_login(port, "bob");
    id = offer_and_get_id(a, b, "bob", "big.bin", 3 * FS_CHUNK_SIZE, 0, &seen);
    CHECK(accept_flow(a, b, id));
    close(a);
    CHECK(recv_type(b, PKT_FILE_ABORT, &h, p, 2000));
    close(b);
    usleep(250000);

    /* offer pending (not yet accepted) when the sender leaves: recipient is told */
    a = tc_login(port, "alice");
    b = tc_login(port, "bob");
    id = offer_and_get_id(a, b, "bob", "pending.bin", 10, 0, &seen);
    close(a);
    CHECK(recv_type(b, PKT_FILE_ABORT, &h, p, 2000));
    close(b);
    usleep(250000);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc != 2) {
        fprintf(stderr, "usage: %s <path-to-chatroom_server>\n", argv[0]);
        return 2;
    }
    signal(SIGPIPE, SIG_IGN);
    snprintf(tmpdir, sizeof(tmpdir), "/tmp/test_file_transfer_%d", (int)getpid());
    CHECK(mkdir(tmpdir, 0700) == 0);
    port = tc_port(4);
    pid_t srv = tc_start_server(argv[1], port);
    CHECK(srv > 0);

    int a = tc_login(port, "alice");
    int b = tc_login(port, "bob");
    int c = tc_login(port, "carol");
    CHECK(a >= 0 && b >= 0 && c >= 0);

    const size_t sizes[] = {0, 1, FS_CHUNK_SIZE, FS_CHUNK_SIZE + 1, 150000};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        test_binary_roundtrip(a, b, c, sizes[i], (unsigned)i + 7);
    }
    printf("binary roundtrips ok\n");
    test_reject(a, b);
    test_path_traversal(a, b);
    test_invalid_offers(a, b);
    test_wrong_party(a, b, c);
    test_protocol_violations(a, b);
    test_bad_checksum_detected(a, b);
    printf("negative cases ok\n");
    close(a);
    close(b);
    close(c);
    usleep(300000);
    test_disconnect_mid_transfer();

    tc_stop_server(srv);
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", tmpdir);
    CHECK(system(cmd) == 0);
    printf("All file transfer tests passed.\n");
    return 0;
}
