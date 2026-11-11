#define _POSIX_C_SOURCE 200809L
#include "fileshare.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* ---------- CRC-32 ---------- */

uint32_t fs_crc32_update(uint32_t crc, const void *data, size_t len) {
    static uint32_t table[256];
    static int ready = 0;
    if (!ready) { /* benign race: every thread computes the same values */
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) {
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        ready = 1;
    }
    const uint8_t *p = (const uint8_t *)data;
    uint32_t c = ~crc;
    for (size_t i = 0; i < len; i++) {
        c = table[(c ^ p[i]) & 0xFFu] ^ (c >> 8);
    }
    return ~c;
}

/* ---------- names, sizes, progress ---------- */

bool fs_sanitize_filename(const char *in, char *out, size_t out_len) {
    if (in == NULL || out == NULL || out_len < 2) {
        return false;
    }
    const char *base = in;
    for (const char *p = in; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    size_t max = FS_FILENAME_LEN - 1;
    if (out_len - 1 < max) {
        max = out_len - 1;
    }
    size_t n = 0;
    for (const unsigned char *p = (const unsigned char *)base; *p != '\0' && n < max; p++) {
        unsigned char c = *p;
        int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                 c == '.' || c == '_' || c == '-';
        out[n++] = ok ? (char)c : '_';
    }
    out[n] = '\0';
    if (n == 0) {
        return false;
    }
    for (size_t i = 0; i < n && out[i] == '.'; i++) { /* no hidden files, no "." / ".." */
        out[i] = '_';
    }
    return true;
}

void fs_format_size(uint64_t bytes, char *out, size_t out_len) {
    static const char *units[] = {"B", "KB", "MB", "GB"};
    double v = (double)bytes;
    size_t u = 0;
    while (v >= 1024.0 && u < 3) {
        v /= 1024.0;
        u++;
    }
    if (u == 0) {
        snprintf(out, out_len, "%llu B", (unsigned long long)bytes);
    } else {
        snprintf(out, out_len, "%.1f %s", v, units[u]);
    }
}

void fs_progress_bar(uint64_t done, uint64_t total, char *out, size_t out_len) {
    enum { WIDTH = 20 };
    unsigned pct = total == 0 ? 100u : (unsigned)((done > total ? total : done) * 100u / total);
    unsigned filled = pct * WIDTH / 100u;
    char bar[WIDTH + 1];
    for (unsigned i = 0; i < WIDTH; i++) {
        bar[i] = i < filled ? '=' : (i == filled ? '>' : ' ');
    }
    bar[WIDTH] = '\0';
    snprintf(out, out_len, "[%s] %u%%", bar, pct);
}

/* ---------- wire codecs ---------- */

static uint64_t hton64(uint64_t v) {
    return ((uint64_t)htonl((uint32_t)(v & 0xFFFFFFFFu)) << 32) | htonl((uint32_t)(v >> 32));
}

void fs_offer_to_net(file_offer_t *o) {
    o->file_size = hton64(o->file_size);
    o->checksum_crc32 = htonl(o->checksum_crc32);
    o->transfer_id = htonl(o->transfer_id);
}

void fs_offer_to_host(file_offer_t *o) {
    o->file_size = hton64(o->file_size); /* byte swap is its own inverse */
    o->checksum_crc32 = ntohl(o->checksum_crc32);
    o->transfer_id = ntohl(o->transfer_id);
    o->sender[FS_NAME_LEN - 1] = '\0';
    o->recipient[FS_NAME_LEN - 1] = '\0';
    o->filename[FS_FILENAME_LEN - 1] = '\0';
}

size_t fs_chunk_encode(uint32_t id, uint32_t index, const void *data, uint16_t size, bool last,
                       uint8_t *out) {
    uint32_t id_n = htonl(id);
    uint32_t idx_n = htonl(index);
    uint16_t size_n = htons(size);
    memcpy(out, &id_n, 4);
    memcpy(out + 4, &idx_n, 4);
    memcpy(out + 8, &size_n, 2);
    out[10] = last ? 1 : 0;
    if (size > 0) {
        memcpy(out + FS_CHUNK_HEADER_LEN, data, size);
    }
    return FS_CHUNK_HEADER_LEN + size;
}

bool fs_chunk_decode(const void *payload, size_t len, fs_chunk_view_t *out) {
    if (len < FS_CHUNK_HEADER_LEN) {
        return false;
    }
    const uint8_t *p = (const uint8_t *)payload;
    uint32_t id_n;
    uint32_t idx_n;
    uint16_t size_n;
    memcpy(&id_n, p, 4);
    memcpy(&idx_n, p + 4, 4);
    memcpy(&size_n, p + 8, 2);
    out->transfer_id = ntohl(id_n);
    out->chunk_index = ntohl(idx_n);
    out->chunk_size = ntohs(size_n);
    out->is_last = p[10] != 0;
    out->data = p + FS_CHUNK_HEADER_LEN;
    return out->chunk_size <= FS_CHUNK_SIZE && len == FS_CHUNK_HEADER_LEN + out->chunk_size;
}

size_t fs_ctl_encode(uint32_t id, const char *reason, uint8_t *out, size_t out_len) {
    if (out_len < 4) {
        return 0;
    }
    uint32_t id_n = htonl(id);
    memcpy(out, &id_n, 4);
    size_t n = 4;
    if (reason != NULL) {
        size_t r = strlen(reason);
        if (r > out_len - 4) {
            r = out_len - 4;
        }
        memcpy(out + 4, reason, r);
        n += r;
    }
    return n;
}

bool fs_ctl_decode(const void *payload, size_t len, uint32_t *id, const char **reason) {
    if (len < 4) {
        return false;
    }
    uint32_t id_n;
    memcpy(&id_n, payload, 4);
    *id = ntohl(id_n);
    if (reason != NULL) {
        *reason = (const char *)payload + 4; /* packet_recv NUL-terminates payloads */
    }
    return true;
}

/* ---------- sender ---------- */

bool fs_file_info(const char *path, uint64_t *size, uint32_t *crc) {
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        return false;
    }
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return false;
    }
    uint32_t c = 0;
    uint64_t total = 0;
    uint8_t buf[FS_CHUNK_SIZE];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        c = fs_crc32_update(c, buf, n);
        total += n;
    }
    bool ok = !ferror(fp);
    fclose(fp);
    if (!ok) {
        return false;
    }
    *size = total;
    *crc = c;
    return true;
}

bool fs_sender_open(fs_sender_t *s, const char *path, uint32_t id, uint64_t expected_size) {
    memset(s, 0, sizeof(*s));
    s->fp = fopen(path, "rb");
    if (s->fp == NULL) {
        return false;
    }
    s->id = id;
    s->size = expected_size;
    return true;
}

bool fs_sender_next(fs_sender_t *s, uint8_t *payload, size_t *payload_len) {
    if (s->fp == NULL || s->done) {
        return false;
    }
    uint8_t data[FS_CHUNK_SIZE];
    uint64_t remaining = s->size - s->sent;
    size_t want = remaining > FS_CHUNK_SIZE ? FS_CHUNK_SIZE : (size_t)remaining;
    size_t got = want == 0 ? 0 : fread(data, 1, want, s->fp);
    if (got != want) {
        return false; /* file truncated while sending */
    }
    s->sent += got;
    bool last = s->sent == s->size;
    *payload_len = fs_chunk_encode(s->id, s->next_index++, data, (uint16_t)got, last, payload);
    s->done = last;
    return true;
}

void fs_sender_close(fs_sender_t *s) {
    if (s->fp != NULL) {
        fclose(s->fp);
        s->fp = NULL;
    }
}

/* ---------- receiver ---------- */

static bool exists(const char *path) {
    struct stat st;
    return lstat(path, &st) == 0;
}

bool fs_receiver_open(fs_receiver_t *r, const char *dir, uint32_t id, const char *safe_name,
                      uint64_t expected_size, uint32_t expected_crc) {
    memset(r, 0, sizeof(*r));
    if (mkdir(dir, 0700) != 0 && errno != EEXIST) {
        return false;
    }
    for (int attempt = 0; attempt < 1000; attempt++) {
        char candidate[FS_PATH_MAX];
        int n = attempt == 0 ? snprintf(candidate, sizeof(candidate), "%s/%s", dir, safe_name)
                             : snprintf(candidate, sizeof(candidate), "%s/%s.%d", dir, safe_name,
                                        attempt);
        if (n < 0 || (size_t)n + 6 >= sizeof(candidate) || exists(candidate)) {
            continue;
        }
        char part[FS_PATH_MAX + 8];
        snprintf(part, sizeof(part), "%s.part", candidate);
        int fd = open(part, O_WRONLY | O_CREAT | O_EXCL, 0600); /* atomic reservation */
        if (fd < 0) {
            continue;
        }
        r->fp = fdopen(fd, "wb");
        if (r->fp == NULL) {
            close(fd);
            unlink(part);
            return false;
        }
        snprintf(r->final_path, sizeof(r->final_path), "%s", candidate);
        snprintf(r->part_path, sizeof(r->part_path), "%s", part);
        r->id = id;
        r->expected_size = expected_size;
        r->expected_crc = expected_crc;
        return true;
    }
    return false;
}

void fs_receiver_abort(fs_receiver_t *r) {
    if (r->fp != NULL) {
        fclose(r->fp);
        r->fp = NULL;
        unlink(r->part_path);
    }
}

fs_chunk_status_t fs_receiver_chunk(fs_receiver_t *r, const fs_chunk_view_t *c) {
    if (r->fp == NULL || c->transfer_id != r->id || c->chunk_index != r->next_index) {
        return FS_ERR_ORDER;
    }
    if (c->chunk_size > FS_CHUNK_SIZE || r->received + c->chunk_size > r->expected_size ||
        (c->is_last != (r->received + c->chunk_size == r->expected_size))) {
        fs_receiver_abort(r);
        return FS_ERR_SIZE;
    }
    if (c->chunk_size > 0 && fwrite(c->data, 1, c->chunk_size, r->fp) != c->chunk_size) {
        fs_receiver_abort(r);
        return FS_ERR_IO;
    }
    r->received += c->chunk_size;
    r->crc = fs_crc32_update(r->crc, c->data, c->chunk_size);
    r->next_index++;
    if (!c->is_last) {
        return FS_CHUNK_MORE;
    }
    if (fflush(r->fp) != 0 || fclose(r->fp) != 0) {
        r->fp = NULL;
        unlink(r->part_path);
        return FS_ERR_IO;
    }
    r->fp = NULL;
    if (r->crc != r->expected_crc) {
        unlink(r->part_path);
        return FS_ERR_CRC;
    }
    if (rename(r->part_path, r->final_path) != 0) {
        unlink(r->part_path);
        return FS_ERR_IO;
    }
    return FS_CHUNK_DONE;
}
