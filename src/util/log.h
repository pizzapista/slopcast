/* Minimal leveled logger. Writes to stderr, which on Vita appears in the
 * kernel log (see `ux0:data/dc-vita/dc-vita.log` if you tee it). */
#ifndef DC_UTIL_LOG_H
#define DC_UTIL_LOG_H

#include <stdarg.h>
#include <stdbool.h>

typedef enum {
    LOG_TRACE = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_NONE
} log_level_t;

void log_set_level(log_level_t level);
log_level_t log_get_level(void);
void log_printf(log_level_t level, const char *tag, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/* Redirect the log to a file (opened lazily on first write). Passing NULL
 * closes any open file and reverts to stderr. */
bool log_set_file(const char *path);

#define LOG_T(...) log_printf(LOG_TRACE, __VA_ARGS__)
#define LOG_D(...) log_printf(LOG_DEBUG, __VA_ARGS__)
#define LOG_I(...) log_printf(LOG_INFO,  __VA_ARGS__)
#define LOG_W(...) log_printf(LOG_WARN,  __VA_ARGS__)
#define LOG_E(...) log_printf(LOG_ERROR, __VA_ARGS__)

/* Rate-limited variants: at most one message per key per interval_ms. */
void log_rate_limited(const char *key, unsigned interval_ms,
                      log_level_t level, const char *tag,
                      const char *fmt, ...)
    __attribute__((format(printf, 5, 6)));

#endif /* DC_UTIL_LOG_H */
