#undef NDEBUG
#define _POSIX_C_SOURCE 200809L
#include "logger.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define THREADS 10
#define LINES_PER_THREAD 300

static char path[64];

static void *worker(void *arg) {
    int id = (int)(long)arg;
    for (int i = 0; i < LINES_PER_THREAD; i++) {
        logger_log(LOG_LEVEL_INFO, "MSG", "thread=%d seq=%d payload=%s", id, i,
                   "0123456789abcdef0123456789abcdef0123456789abcdef");
    }
    return NULL;
}

static size_t read_lines(char lines[][512], size_t max) {
    FILE *f = fopen(path, "r");
    assert(f != NULL);
    size_t n = 0;
    while (n < max && fgets(lines[n], 512, f) != NULL) {
        n++;
    }
    fclose(f);
    return n;
}

/* "[2026-01-02T03:04:05Z] [INFO] [MSG]  rest" */
static int well_formed(const char *l) {
    if (l[0] != '[' || l[20] != 'Z' || l[21] != ']' || l[22] != ' ' || l[23] != '[') {
        return 0;
    }
    for (int i = 1; i < 20; i++) {
        const char c = l[i];
        const int digit = c >= '0' && c <= '9';
        if (!(digit || (i == 5 && c == '-') || (i == 8 && c == '-') || (i == 11 && c == 'T') ||
              (i == 14 && c == ':') || (i == 17 && c == ':'))) {
            return 0;
        }
    }
    return l[strlen(l) - 1] == '\n';
}

static void test_no_interleaving(void) {
    assert(logger_init(path));
    pthread_t t[THREADS];
    for (long i = 0; i < THREADS; i++) {
        pthread_create(&t[i], NULL, worker, (void *)i);
    }
    for (int i = 0; i < THREADS; i++) {
        pthread_join(t[i], NULL);
    }
    logger_close();

    static char lines[THREADS * LINES_PER_THREAD + 10][512];
    size_t n = read_lines(lines, THREADS * LINES_PER_THREAD + 10);
    assert(n == THREADS * LINES_PER_THREAD);
    static int seen[THREADS][LINES_PER_THREAD];
    for (size_t i = 0; i < n; i++) {
        assert(well_formed(lines[i]));
        int id = -1;
        int seq = -1;
        char tail[64] = "";
        const char *body = strstr(lines[i], "thread=");
        assert(body != NULL);
        int parsed = sscanf(body, "thread=%d seq=%d payload=%63s", &id, &seq, tail);
        assert(parsed == 3);
        assert(strcmp(tail, "0123456789abcdef0123456789abcdef0123456789abcdef") == 0);
        assert(id >= 0 && id < THREADS && seq >= 0 && seq < LINES_PER_THREAD);
        seen[id][seq]++;
    }
    for (int a = 0; a < THREADS; a++) {
        for (int b = 0; b < LINES_PER_THREAD; b++) {
            assert(seen[a][b] == 1); /* every entry exactly once */
        }
    }
}

static void test_append_across_restart(void) {
    assert(logger_init(path));
    logger_log(LOG_LEVEL_WARN, "AUTH", "after restart");
    logger_close();
    logger_close(); /* idempotent */
    static char lines[THREADS * LINES_PER_THREAD + 10][512];
    size_t n = read_lines(lines, THREADS * LINES_PER_THREAD + 10);
    assert(n == THREADS * LINES_PER_THREAD + 1); /* old entries preserved, new one appended */
    assert(strstr(lines[n - 1], "[WARN] [AUTH] after restart") != NULL);
}

static void test_format_and_escaping(void) {
    remove(path);
    assert(logger_init(path));
    logger_log(LOG_LEVEL_INFO, "CONN", "Client '%s' connected from %s:%d", "alice", "10.0.0.1", 5);
    logger_log(LOG_LEVEL_INFO, "MSG",
               "'a' -> ALL: \"x\"\n[2000-01-01T00:00:00Z] [ERROR] [FAKE] forged");
    logger_log(LOG_LEVEL_ERROR, "DISC", "ctrl\x01\x02 done");
    logger_log(LOG_LEVEL_DEBUG, "GRP", "debug");
    logger_close();

    char lines[16][512];
    size_t n = read_lines(lines, 16);
    assert(n == 4); /* the forged newline did not create an extra entry */
    assert(strstr(lines[0], "[INFO] [CONN] Client 'alice' connected from 10.0.0.1:5") != NULL);
    assert(strstr(lines[1], "[INFO] [MSG]  'a' -> ALL") != NULL); /* aligned column */
    assert(strstr(lines[1], "\\n[2000-01-01") != NULL);
    assert(strstr(lines[2], "[ERROR] [DISC] ctrl?? done") != NULL);
    assert(strstr(lines[3], "[DEBUG] [GRP]  debug") != NULL);
    for (size_t i = 0; i < n; i++) {
        assert(well_formed(lines[i]));
    }
}

static void test_failure_and_noop(void) {
    logger_close();
    logger_log(LOG_LEVEL_INFO, "X", "not initialised: must not crash");
    assert(!logger_init("/nonexistent-dir/x/server.log"));
    assert(!logger_init(NULL));
    logger_log(LOG_LEVEL_INFO, "X", "still no crash");
}

int main(void) {
    snprintf(path, sizeof(path), "/tmp/test_logger_%d.log", (int)getpid());
    remove(path);
    test_no_interleaving();
    test_append_across_restart();
    test_format_and_escaping();
    test_failure_and_noop();
    remove(path);
    printf("All logger tests passed.\n");
    return 0;
}
