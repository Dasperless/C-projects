#ifndef CHATROOM_LOGGER_H
#define CHATROOM_LOGGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Thread-safe, append-only audit logger.
 *
 * Line format:
 *   [YYYY-MM-DDTHH:MM:SSZ] [LEVEL] [CATEGORY] message
 * Every call writes one complete line under a mutex and flushes it, so concurrent threads can
 * never interleave characters and entries survive an abnormal exit. Control characters in the
 * message (newlines etc.) are escaped so a user cannot forge extra log lines.
 */

typedef enum { LOG_LEVEL_DEBUG, LOG_LEVEL_INFO, LOG_LEVEL_WARN, LOG_LEVEL_ERROR } log_level_t;

/* Opens `filepath` in append mode (never truncated). Re-initialising closes the previous file.
 * Returns false if the file cannot be opened. */
bool logger_init(const char *filepath);

/* Appends one entry. A no-op if the logger is not initialised. */
void logger_log(log_level_t level, const char *category, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/* Flushes and closes the log file. Safe to call repeatedly. */
void logger_close(void);

#ifdef __cplusplus
}
#endif

#endif /* CHATROOM_LOGGER_H */
