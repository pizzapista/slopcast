#include "util/profiler.h"
#include "util/log.h"

#include <stdio.h>
#include <string.h>

#if defined(__vita__)
#  include <psp2/kernel/processmgr.h>
uint64_t profiler_now_us(void) {
    return (uint64_t)sceKernelGetSystemTimeWide();
}
#else
#  include <time.h>
uint64_t profiler_now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000u + (uint64_t)(ts.tv_nsec / 1000);
}
#endif

void profiler_init(profiler_t *p) {
    memset(p, 0, sizeof(*p));
}

int profiler_add(profiler_t *p, const char *name) {
    if (p->section_count >= (uint32_t)(sizeof(p->sections) / sizeof(p->sections[0])))
        return -1;
    uint32_t idx = p->section_count++;
    strncpy(p->sections[idx].name, name, sizeof(p->sections[idx].name) - 1);
    p->sections[idx].name[sizeof(p->sections[idx].name) - 1] = '\0';
    return (int)idx;
}

void profiler_begin(profiler_t *p, int section) {
    if (section < 0) return;
    p->sections[section].start_us = profiler_now_us();
}

void profiler_end(profiler_t *p, int section) {
    if (section < 0) return;
    uint64_t dt = profiler_now_us() - p->sections[section].start_us;
    prof_section_t *s = &p->sections[section];
    s->samples_us[s->head] = dt;
    s->head = (s->head + 1) % PROF_HISTORY;
    if (s->count < PROF_HISTORY) s->count++;
    s->total_us += dt;
}

void profiler_frame_begin(profiler_t *p) {
    p->frame_start_us = profiler_now_us();
}

void profiler_frame_end(profiler_t *p) {
    p->frame_index++;
}

/* Median of the last `count` samples, using insertion sort on <= 120 values.
 * Runs at most once per frame from the UI thread. */
static uint64_t median_u64(const uint64_t *values, uint32_t count) {
    if (count == 0) return 0;
    uint64_t tmp[PROF_HISTORY];
    memcpy(tmp, values, count * sizeof(uint64_t));
    for (uint32_t i = 1; i < count; i++) {
        uint64_t v = tmp[i];
        uint32_t j = i;
        while (j > 0 && tmp[j - 1] > v) { tmp[j] = tmp[j - 1]; j--; }
        tmp[j] = v;
    }
    return tmp[count / 2];
}

uint64_t profiler_median_frame_us(const profiler_t *p) {
    /* Frame time isn't tracked as a section; instead we report the median of
     * the "frame" section if the caller created one. Otherwise 0. */
    for (uint32_t i = 0; i < p->section_count; i++) {
        if (strcmp(p->sections[i].name, "frame") == 0) {
            return median_u64(p->sections[i].samples_us, p->sections[i].count);
        }
    }
    return 0;
}

void profiler_dump(const profiler_t *p) {
    LOG_I("prof", "frame %u, %u sections", p->frame_index, p->section_count);
    for (uint32_t i = 0; i < p->section_count; i++) {
        const prof_section_t *s = &p->sections[i];
        uint64_t med = median_u64(s->samples_us, s->count);
        LOG_I("prof", "  %-20s median %6llu us  total %8llu us  n=%u",
              s->name, (unsigned long long)med,
              (unsigned long long)s->total_us, s->count);
    }
}
