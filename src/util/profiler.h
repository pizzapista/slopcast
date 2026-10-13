/* Frame-time profiler. Cheap enough to leave on in release builds; the
 * hot path is two clock reads and an array store. */
#ifndef DC_UTIL_PROFILER_H
#define DC_UTIL_PROFILER_H

#include <stdint.h>
#include <stdbool.h>

#define PROF_HISTORY 120   /* 2 seconds at 60 Hz */

typedef struct {
    char     name[24];
    uint64_t start_us;
    uint64_t samples_us[PROF_HISTORY];
    uint32_t head;
    uint32_t count;
    uint64_t total_us;
} prof_section_t;

typedef struct {
    prof_section_t sections[16];
    uint32_t       section_count;
    uint64_t       frame_start_us;
    uint32_t       frame_index;
} profiler_t;

void     profiler_init(profiler_t *p);
int      profiler_add(profiler_t *p, const char *name);
void     profiler_begin(profiler_t *p, int section);
void     profiler_end(profiler_t *p, int section);
void     profiler_frame_begin(profiler_t *p);
void     profiler_frame_end(profiler_t *p);

uint64_t profiler_now_us(void);

/* Summary written on demand (menu, shutdown). Returns the median frame time
 * over the last window, or 0 if no frames have been recorded. */
uint64_t profiler_median_frame_us(const profiler_t *p);
void     profiler_dump(const profiler_t *p);

#endif /* DC_UTIL_PROFILER_H */
