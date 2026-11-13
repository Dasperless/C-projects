#undef NDEBUG
#define _POSIX_C_SOURCE 200809L
#include "fileshare.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char tmpdir[64];

static void write_file(const char *path, const uint8_t *data, size_t n) {
    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    assert(data != NULL);
    size_t written = fwrite(data, 1, n, f);
    assert(written == n);
    fclose(f);
}

static void test_crc32(void) {
    assert(fs_crc32_update(0, "123456789", 9) == 0xCBF43926u); /* standard check value */
    assert(fs_crc32_update(0, "", 0) == 0);
    uint32_t c = fs_crc32_update(0, "1234", 4);
    c = fs_crc32_update(c, "56789", 5); /* streaming == one shot */
    assert(c == 0xCBF43926u);
}

static void test_sanitize(void) {
    char o[FS_FILENAME_LEN];
    assert(fs_sanitize_filename("photo.png", o, sizeof(o)) && strcmp(o, "photo.png") == 0);
    assert(fs_sanitize_filename("../../etc/passwd", o, sizeof(o)) && strcmp(o, "passwd") == 0);
    assert(fs_sanitize_filename("/etc/shadow", o, sizeof(o)) && strcmp(o, "shadow") == 0);
    assert(fs_sanitize_filename("C:\\Windows\\evil.exe", o, sizeof(o)) &&
           strcmp(o, "evil.exe") == 0);
    assert(fs_sanitize_filename("my file (1).txt", o, sizeof(o)) &&
           strcmp(o, "my_file__1_.txt") == 0);
    assert(fs_sanitize_filename(".bashrc", o, sizeof(o)) && strcmp(o, "_bashrc") == 0);
    assert(fs_sanitize_filename("..", o, sizeof(o)) && strchr(o, '.') == NULL);
    assert(!fs_sanitize_filename("", o, sizeof(o)) && !fs_sanitize_filename("dir/", o, sizeof(o)));
    assert(!fs_sanitize_filename(NULL, o, sizeof(o)) && !fs_sanitize_filename("a", o, 1));
    char longname[200];
    memset(longname, 'a', sizeof(longname));
    longname[199] = '\0';
    assert(fs_sanitize_filename(longname, o, sizeof(o)) && strlen(o) == FS_FILENAME_LEN - 1);
    assert(fs_sanitize_filename("caf\xc3\xa9.txt", o, sizeof(o)) && strchr(o, '/') == NULL);
}

static void test_format(void) {
    char b[32];
    fs_format_size(0, b, sizeof(b));
    assert(strcmp(b, "0 B") == 0);
    fs_format_size(1536 * 1024, b, sizeof(b));
    assert(strcmp(b, "1.5 MB") == 0);
    fs_format_size(50ULL << 20, b, sizeof(b));
    assert(strcmp(b, "50.0 MB") == 0);
    fs_progress_bar(0, 100, b, sizeof(b));
    assert(strcmp(b, "[>                   ] 0%") == 0);
    fs_progress_bar(50, 100, b, sizeof(b));
    assert(strcmp(b, "[==========>         ] 50%") == 0);
    fs_progress_bar(100, 100, b, sizeof(b));
    assert(strcmp(b, "[====================] 100%") == 0);
    fs_progress_bar(0, 0, b, sizeof(b));
    assert(strstr(b, "100%") != NULL);
}

static void test_codecs(void) {
    assert(sizeof(file_offer_t) == 144);
    file_offer_t o;
    memset(&o, 0, sizeof(o));
    strcpy(o.filename, "a.bin");
    o.file_size = 0x0102030405060708ULL;
    o.checksum_crc32 = 0xA1B2C3D4u;
    o.transfer_id = 77;
    fs_offer_to_net(&o);
    const uint8_t *raw = (const uint8_t *)&o;
    assert(raw[128] == 1 && raw[135] == 8); /* big-endian size */
    assert(raw[136] == 0xA1 && raw[139] == 0xD4);
    fs_offer_to_host(&o);
    assert(o.file_size == 0x0102030405060708ULL && o.checksum_crc32 == 0xA1B2C3D4u);
    assert(o.transfer_id == 77);

    uint8_t buf[FS_CHUNK_HEADER_LEN + FS_CHUNK_SIZE];
    uint8_t data[10] = {0, 1, 2, 3, 0, 5, 6, 7, 8, 9};
    size_t n = fs_chunk_encode(9, 3, data, 10, true, buf);
    assert(n == FS_CHUNK_HEADER_LEN + 10);
    fs_chunk_view_t v;
    assert(fs_chunk_decode(buf, n, &v));
    assert(v.transfer_id == 9 && v.chunk_index == 3 && v.chunk_size == 10 && v.is_last);
    assert(memcmp(v.data, data, 10) == 0);
    assert(!fs_chunk_decode(buf, n - 1, &v) && !fs_chunk_decode(buf, 5, &v));
    buf[8] = 0x20; /* claims 8192 bytes: larger than FS_CHUNK_SIZE */
    assert(!fs_chunk_decode(buf, n, &v));

    uint8_t ctl[64];
    size_t cn = fs_ctl_encode(5, "boom", ctl, sizeof(ctl));
    ctl[cn] = '\0';
    uint32_t id = 0;
    const char *reason = NULL;
    assert(cn == 8 && fs_ctl_decode(ctl, cn, &id, &reason) && id == 5 &&
           strcmp(reason, "boom") == 0);
    assert(!fs_ctl_decode(ctl, 3, &id, &reason));
}

/* sender -> receiver loopback must reproduce the file bit for bit. */
static void roundtrip(size_t size, unsigned seed) {
    char src[128];
    snprintf(src, sizeof(src), "%s/src_%zu.bin", tmpdir, size);
    uint8_t *data = (uint8_t *)malloc(size + 1);
    assert(data != NULL);
    for (size_t i = 0; i < size; i++) { /* every byte value, including NULs */
        seed = seed * 1664525u + 1013904223u;
        data[i] = (uint8_t)(seed >> 24);
    }
    write_file(src, data, size);

    uint64_t fsize = 0;
    uint32_t crc = 0;
    assert(fs_file_info(src, &fsize, &crc) && fsize == size);

    char dir[128];
    snprintf(dir, sizeof(dir), "%s/downloads", tmpdir);
    fs_receiver_t rx;
    assert(fs_receiver_open(&rx, dir, 1, "out.bin", fsize, crc));
    fs_sender_t tx;
    assert(fs_sender_open(&tx, src, 1, fsize));
    fs_chunk_status_t st = FS_CHUNK_MORE;
    unsigned chunks = 0;
    uint8_t payload[FS_CHUNK_HEADER_LEN + FS_CHUNK_SIZE];
    while (!tx.done) {
        size_t len = 0;
        assert(fs_sender_next(&tx, payload, &len));
        fs_chunk_view_t v;
        assert(fs_chunk_decode(payload, len, &v));
        st = fs_receiver_chunk(&rx, &v);
        chunks++;
        assert(st == FS_CHUNK_MORE || st == FS_CHUNK_DONE);
    }
    assert(st == FS_CHUNK_DONE);
    assert(chunks == (size == 0 ? 1 : (size + FS_CHUNK_SIZE - 1) / FS_CHUNK_SIZE));
    fs_sender_close(&tx);

    FILE *f = fopen(rx.final_path, "rb");
    assert(f != NULL);
    uint8_t *back = (uint8_t *)malloc(size + 1);
    assert(back != NULL);
    size_t read_bytes = fread(back, 1, size, f);
    assert(read_bytes == size);
    int next_ch = fgetc(f);
    assert(next_ch == EOF);
    fclose(f);
    assert(memcmp(back, data, size) == 0);
    assert(access(rx.part_path, F_OK) != 0); /* .part moved into place */
    free(back);
    free(data);
}

static void test_roundtrips(void) {
    const size_t sizes[] = {0, 1, 4095, 4096, 4097, 8192, 100000};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        roundtrip(sizes[i], (unsigned)i + 1);
    }
}

static void test_receiver_rejections(void) {
    char dir[128];
    snprintf(dir, sizeof(dir), "%s/downloads", tmpdir);
    uint8_t buf[FS_CHUNK_HEADER_LEN + FS_CHUNK_SIZE];
    uint8_t data[100];
    memset(data, 7, sizeof(data));
    uint32_t good_crc = fs_crc32_update(0, data, sizeof(data));
    fs_chunk_view_t v;
    fs_receiver_t rx;

    /* bad checksum: partial file removed, nothing delivered */
    assert(fs_receiver_open(&rx, dir, 2, "bad.bin", 100, good_crc ^ 1u));
    size_t n = fs_chunk_encode(2, 0, data, 100, true, buf);
    assert(fs_chunk_decode(buf, n, &v) && fs_receiver_chunk(&rx, &v) == FS_ERR_CRC);
    assert(access(rx.part_path, F_OK) != 0 && access(rx.final_path, F_OK) != 0);

    /* wrong id, out-of-order index */
    assert(fs_receiver_open(&rx, dir, 3, "ord.bin", 100, good_crc));
    n = fs_chunk_encode(99, 0, data, 100, true, buf);
    assert(fs_chunk_decode(buf, n, &v) && fs_receiver_chunk(&rx, &v) == FS_ERR_ORDER);
    n = fs_chunk_encode(3, 1, data, 100, true, buf);
    assert(fs_chunk_decode(buf, n, &v) && fs_receiver_chunk(&rx, &v) == FS_ERR_ORDER);
    fs_receiver_abort(&rx);
    fs_receiver_abort(&rx); /* idempotent */
    assert(access(rx.part_path, F_OK) != 0);

    /* more data than declared / premature last flag */
    assert(fs_receiver_open(&rx, dir, 4, "big.bin", 50, good_crc));
    n = fs_chunk_encode(4, 0, data, 100, false, buf);
    assert(fs_chunk_decode(buf, n, &v) && fs_receiver_chunk(&rx, &v) == FS_ERR_SIZE);
    assert(fs_receiver_open(&rx, dir, 5, "early.bin", 100, good_crc));
    n = fs_chunk_encode(5, 0, data, 40, true, buf);
    assert(fs_chunk_decode(buf, n, &v) && fs_receiver_chunk(&rx, &v) == FS_ERR_SIZE);
    assert(access(rx.part_path, F_OK) != 0);

    /* chunk after completion / on aborted receiver */
    assert(fs_receiver_open(&rx, dir, 6, "once.bin", 100, good_crc));
    n = fs_chunk_encode(6, 0, data, 100, true, buf);
    assert(fs_chunk_decode(buf, n, &v) && fs_receiver_chunk(&rx, &v) == FS_CHUNK_DONE);
    assert(fs_receiver_chunk(&rx, &v) == FS_ERR_ORDER);
}

static void test_no_overwrite(void) {
    char dir[128];
    snprintf(dir, sizeof(dir), "%s/downloads", tmpdir);
    fs_receiver_t a;
    fs_receiver_t b;
    assert(fs_receiver_open(&a, dir, 10, "same.txt", 1, 0));
    assert(fs_receiver_open(&b, dir, 11, "same.txt", 1, 0)); /* both reserved concurrently */
    assert(strcmp(a.final_path, b.final_path) != 0);
    fs_receiver_abort(&a);
    fs_receiver_abort(&b);

    write_file(a.final_path, (const uint8_t *)"keep", 4); /* pre-existing file survives */
    fs_receiver_t c;
    assert(fs_receiver_open(&c, dir, 12, "same.txt", 1, 0));
    assert(strcmp(c.final_path, a.final_path) != 0);
    fs_receiver_abort(&c);
    assert(fs_receiver_open(&c, "/proc/definitely/not/writable", 1, "x", 1, 0) == false);
}

static void test_sender_errors(void) {
    uint64_t size;
    uint32_t crc;
    assert(!fs_file_info("/nonexistent/file", &size, &crc));
    assert(!fs_file_info("/tmp", &size, &crc)); /* directory is not a regular file */
    fs_sender_t s;
    assert(!fs_sender_open(&s, "/nonexistent/file", 1, 1));

    /* file shrinks after the offer was made */
    char path[128];
    snprintf(path, sizeof(path), "%s/shrink.bin", tmpdir);
    write_file(path, (const uint8_t *)"abc", 3);
    assert(fs_sender_open(&s, path, 1, 10));
    uint8_t payload[FS_CHUNK_HEADER_LEN + FS_CHUNK_SIZE];
    size_t len;
    assert(!fs_sender_next(&s, payload, &len));
    fs_sender_close(&s);
    fs_sender_close(&s);
}

int main(void) {
    snprintf(tmpdir, sizeof(tmpdir), "/tmp/test_fileshare_%d", (int)getpid());
    assert(mkdir(tmpdir, 0700) == 0);
    test_crc32();
    test_sanitize();
    test_format();
    test_codecs();
    test_roundtrips();
    test_receiver_rejections();
    test_no_overwrite();
    test_sender_errors();
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", tmpdir);
    int rc = system(cmd);
    assert(rc == 0);
    printf("All fileshare tests passed.\n");
    return 0;
}
