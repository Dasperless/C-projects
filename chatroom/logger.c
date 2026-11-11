#define _POSIX_C_SOURCE 200809L
#include "logger.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define LOG_MSG_MAX 3072
#define LOG_LINE_MAX 3200

static FILE *log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *level_name(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG:
            return "DEBUG";
        case LOG_LEVEL_INFO:
            return "INFO";
        case LOG_LEVEL_WARN:
            return "WARN";
        case LOG_LEVEL_ERROR:
            return "ERROR";
    }
    return "INFO";
}

bool logger_init(const char *filepath) {
    if (filepath == NULL) {
        return false;
    }
    FILE *f = fopen(filepath, "a");
    if (f == NULL) {
        return false;
    }
    pthread_mutex_lock(&log_mutex);
    if (log_file != NULL) {
        fclose(log_file);
    }
    log_file = f;
    pthread_mutex_unlock(&log_mutex);
    return true;
}

/* Copies src to dst replacing control characters so one call always yields exactly one line. */
static void sanitize(char *dst, size_t dst_size, const char *src) {
    size_t o = 0;
    for (const unsigned char *p = (const unsigned char *)src; *p != '\0' && o + 2 < dst_size; p++) {
        if (*p == '\n' || *p == '\r') {
            dst[o++] = '\\';
            dst[o++] = (*p == '\n') ? 'n' : 'r';
        } else if (*p < 0x20 || *p == 0x7f) {
            dst[o++] = '?';
        } else {
            dst[o++] = (char)*p;
        }
    }
    dst[o] = '\0';
}

void logger_log(log_level_t level, const char *category, const char *fmt, ...) {
    char raw[LOG_MSG_MAX];
    char msg[LOG_MSG_MAX * 2];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(raw, sizeof(raw), fmt, ap);
    va_end(ap);
    sanitize(msg, sizeof(msg), raw);

    char ts[32];
    time_t now = time(NULL);
    struct tm tm_utc;
    gmtime_r(&now, &tm_utc);
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);

    char cat[16];
    char cat_clean[8];
    sanitize(cat_clean, sizeof(cat_clean), category != NULL ? category : "");
    snprintf(cat, sizeof(cat), "[%s]", cat_clean);

    char line[LOG_LINE_MAX];
    /* Category column is padded to 6 chars ("[CONN] ", "[MSG]  ") for readable alignment. */
    snprintf(line, sizeof(line), "[%s] [%s] %-6s %.3000s\n", ts, level_name(level), cat, msg);

    pthread_mutex_lock(&log_mutex);
    if (log_file != NULL) {
        fputs(line, log_file);
        fflush(log_file);
    }
    pthread_mutex_unlock(&log_mutex);
}

void logger_close(void) {
    pthread_mutex_lock(&log_mutex);
    if (log_file != NULL) {
        fflush(log_file);
        fclose(log_file);
        log_file = NULL;
    }
    pthread_mutex_unlock(&log_mutex);
}
