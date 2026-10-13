#include "core/scheduler.h"
#include "core/dc.h"
#include "sh4/sh4.h"
#include "util/log.h"

void dc_scheduler_init(dc_scheduler_t *s, uint32_t frame_rate) {
    s->frame_rate      = frame_rate;
    s->frame_number    = 0;
    s->vblank_pending  = false;
    s->cycles_remaining = (frame_rate == 60)
        ? DC_CYCLES_PER_FRAME_NTSC
        : DC_CYCLES_PER_FRAME_PAL;
}

void dc_scheduler_reset(dc_scheduler_t *s) {
    dc_scheduler_init(s, s->frame_rate);
}

uint64_t dc_scheduler_cycles_left(const dc_scheduler_t *s) {
    return s->cycles_remaining;
}

void dc_scheduler_advance(dc_scheduler_t *s, dc_t *dc, uint64_t n) {
    if (n >= s->cycles_remaining) {
        /* Budget exhausted: raise vblank and let the main loop present. */
        s->cycles_remaining = 0;
        s->vblank_pending   = true;
        sh4_raise_interrupt(&dc->sh4, SH4_INT_VBLANK);
    } else {
        s->cycles_remaining -= n;
    }
}

void dc_scheduler_begin_frame(dc_scheduler_t *s, uint32_t frame_rate) {
    s->frame_rate       = frame_rate;
    s->cycles_remaining = (frame_rate == 60)
        ? DC_CYCLES_PER_FRAME_NTSC
        : DC_CYCLES_PER_FRAME_PAL;
    s->frame_number++;
    s->vblank_pending   = false;
}
