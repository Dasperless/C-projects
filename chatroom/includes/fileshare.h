#ifndef CHATROOM_FILESHARE_H
#define CHATROOM_FILESHARE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Chunked binary file transfer (issue #14).
 *
 * Flow (relayed through the server):
 *   sender  --PKT_FILE_OFFER-->  server  --PKT_FILE_OFFER(id)-->  recipient (+ echo to sender)
 *   recipient --PKT_FILE_ACCEPT/REJECT(id)--> server --> sender
 *   sender  --PKT_FILE_CHUNK x N--> server --> recipient       (4096-byte chunks, last flagged)
 *   either side --PKT_FILE_ABORT(id, reason)--> server --> peer
 * All multi-byte integers in payloads are big-endian.
 */

#define FS_CHUNK_SIZE 4096u
#define FS_MAX_FILE_SIZE (50ULL * 1024ULL * 1024ULL) /* 50 MB */
#define FS_NAME_LEN 32
#define FS_FILENAME_LEN 64
#define FS_DOWNLOAD_DIR "downloads"
#define FS_CHUNK_HEADER_LEN 11u
#define FS_PATH_MAX 320

typedef struct __attribute__((packed)) {
    char sender[FS_NAME_LEN];
    char recipient[FS_NAME_LEN];
    char filename[FS_FILENAME_LEN];
    uint64_t file_size;
    uint32_t checksum_crc32;
    uint32_t transfer_id; /* 0 from the sender; assigned by the server */
} file_offer_t;

typedef struct __attribute__((packed)) {
    uint32_t transfer_id;
    uint32_t chunk_index;
    uint16_t chunk_size;
    uint8_t is_last_chunk;
    uint8_t data[FS_CHUNK_SIZE];
} file_chunk_t;

/* ---- pure helpers ---- */

/* IEEE CRC-32. Start with crc = 0 and feed the result back in to continue a running checksum. */
uint32_t fs_crc32_update(uint32_t crc, const void *data, size_t len);

/* Reduces any user-supplied name to a safe single path component: strips directories (/ and \),
 * replaces characters outside [A-Za-z0-9._-] with '_', neutralises leading dots and truncates to
 * FS_FILENAME_LEN-1. Returns false when nothing usable is left (e.g. "", "dir/"). */
bool fs_sanitize_filename(const char *in, char *out, size_t out_len);

/* "1.2 MB" style size and "[=====>    ] 50%" style progress bar. */
void fs_format_size(uint64_t bytes, char *out, size_t out_len);
void fs_progress_bar(uint64_t done, uint64_t total, char *out, size_t out_len);

/* Offer fields <-> network byte order (in place). */
void fs_offer_to_net(file_offer_t *offer);
void fs_offer_to_host(file_offer_t *offer);

/* Chunk payload codec. Encode writes FS_CHUNK_HEADER_LEN + size bytes and returns that length. */
size_t fs_chunk_encode(uint32_t id, uint32_t index, const void *data, uint16_t size, bool last,
                       uint8_t *out);
typedef struct {
    uint32_t transfer_id;
    uint32_t chunk_index;
    uint16_t chunk_size;
    bool is_last;
    const uint8_t *data;
} fs_chunk_view_t;
/* Validates length/size consistency. */
bool fs_chunk_decode(const void *payload, size_t len, fs_chunk_view_t *out);

/* Control payload (accept/reject/abort): 4-byte id followed by an optional reason string. */
size_t fs_ctl_encode(uint32_t id, const char *reason, uint8_t *out, size_t out_len);
bool fs_ctl_decode(const void *payload, size_t len, uint32_t *id, const char **reason);

/* ---- sender ---- */

/* Size and CRC of a regular file. Returns false if it is not a readable regular file. */
bool fs_file_info(const char *path, uint64_t *size, uint32_t *crc);

typedef struct {
    FILE *fp;
    uint32_t id;
    uint64_t size;
    uint64_t sent;
    uint32_t next_index;
    bool done;
} fs_sender_t;

bool fs_sender_open(fs_sender_t *s, const char *path, uint32_t id, uint64_t expected_size);
/* Produces the next chunk payload (>= FS_CHUNK_HEADER_LEN + FS_CHUNK_SIZE bytes of room needed).
 * Returns false on I/O error or if the file changed size. */
bool fs_sender_next(fs_sender_t *s, uint8_t *payload, size_t *payload_len);
void fs_sender_close(fs_sender_t *s);

/* ---- receiver ---- */

typedef enum {
    FS_CHUNK_MORE, /* stored, more expected */
    FS_CHUNK_DONE, /* final chunk stored, size and CRC verified, file moved into place */
    FS_ERR_ORDER,  /* wrong transfer id or out-of-sequence chunk */
    FS_ERR_SIZE,   /* chunk exceeds the declared size / inconsistent last flag */
    FS_ERR_CRC,    /* checksum mismatch (partial file removed) */
    FS_ERR_IO      /* write/rename failure */
} fs_chunk_status_t;

typedef struct {
    FILE *fp;
    uint32_t id;
    uint64_t expected_size;
    uint64_t received;
    uint32_t expected_crc;
    uint32_t crc;
    uint32_t next_index;
    char part_path[FS_PATH_MAX + 8]; /* final_path + ".part" */
    char final_path[FS_PATH_MAX];
} fs_receiver_t;

/* Creates `dir` (mode 0700) if needed and reserves a unique "<name>.part" file inside it.
 * `safe_name` must already be sanitized. Existing files are never overwritten (name.1, name.2 ...).
 */
bool fs_receiver_open(fs_receiver_t *r, const char *dir, uint32_t id, const char *safe_name,
                      uint64_t expected_size, uint32_t expected_crc);
fs_chunk_status_t fs_receiver_chunk(fs_receiver_t *r, const fs_chunk_view_t *chunk);
/* Closes and deletes the partial file. Safe on an already finished receiver. */
void fs_receiver_abort(fs_receiver_t *r);

#ifdef __cplusplus
}
#endif

#endif /* CHATROOM_FILESHARE_H */
