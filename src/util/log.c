#include "util/log.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* Host-vs-Vita: on Vita we use the kernel clock; on desktop we fall back to
 * clock_gettime. We only need millisecond resolution. */
#if defined(__vita__)
#  include <psp2/kernel/processmgr.h>
#  define NOW_MS() ((uint64_t)(sceKernelGetSystemTimeWide() / 1000))
typedef uint64_t log_now_t;
#else
#  include <time.h>
static uint64_t now_ms_host(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000);
}
#  define NOW_MS() now_ms_host()
typedef uint64_t log_now_t;
#endif

static log_level_t g_level = LOG_INFO;
static FILE       *g_out   = NULL;   /* NULL => stderr */

static const char *level_tag(log_level_t l) {
    switch (l) {
        case LOG_TRACE: return "TRC";
        case LOG_DEBUG: return "DBG";
        case LOG_INFO:  return "INF";
        case LOG_WARN:  return "WRN";
        case LOG_ERROR: return "ERR";
        default:        return "???";
    }
}

void log_set_level(log_level_t level) { g_level = level; }
log_level_t log_get_level(void)        { return g_level; }

bool log_set_file(const char *path) {
    if (g_out) { fclose(g_out); g_out = NULL; }
    if (!path) return true;
    g_out = fopen(path, "a");
    return g_out != NULL;
}

void log_printf(log_level_t level, const char *tag, const char *fmt, ...) {
    if (level < g_level) return;
    FILE *out = g_out ? g_out : stderr;

    fprintf(out, "[%8llu] %s %-6s ",
            (unsigned long long)NOW_MS(), level_tag(level), tag ? tag : "-");

    va_list ap;
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);

    fputc('\n', out);
    fflush(out);
}

/* ------------------------------------------------------------------ */
/* Rate-limited logging                                                */
/* ------------------------------------------------------------------ */

#define RATE_SLOTS 32

typedef struct {
    char     key[48];
    uint64_t last_ms;
    uint32_t count;
} rate_slot_t;

static rate_slot_t g_slots[RATE_SLOTS];
static unsigned    g_next_slot = 0;

void log_rate_limited(const char *key, unsigned interval_ms,
                      log_level_t level, const char *tag,
                      const char *fmt, ...) {
    if (level < g_level) return;
    if (!key) { key = "?"; }

    uint64_t now = NOW_MS();
    rate_slot_t *slot = NULL;

    for (unsigned i = 0; i < RATE_SLOTS; i++) {
        if (strncmp(g_slots[i].key, key, sizeof(g_slots[i].key)) == 0) {
            slot = &g_slots[i];
            break;
        }
    }
    if (!slot) {
        slot = &g_slots[g_next_slot];
        g_next_slot = (g_next_slot + 1) % RATE_SLOTS;
        strncpy(slot->key, key, sizeof(slot->key) - 1);
        slot->key[sizeof(slot->key) - 1] = '\0';
        slot->last_ms = now;
        slot->count   = 0;
    }

    if (now - slot->last_ms < interval_ms) {
        slot->count++;
        return;
    }

    uint32_t suppressed = slot->count;
    slot->count   = 0;
    slot->last_ms = now;

    FILE *out = g_out ? g_out : stderr;
    fprintf(out, "[%8llu] %s %-6s ",
            (unsigned long long)now, level_tag(level), tag ? tag : "-");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);
    if (suppressed) fprintf(out, " (+%u suppressed)", suppressed);
    fputc('\n', out);
    fflush(out);
}
