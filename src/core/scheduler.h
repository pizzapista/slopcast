/* Cycle-budget scheduler.
 *
 * The Dreamcast runs three clocks that have to stay coherent:
 *   - SH-4 CPU:   200 MHz
 *   - AICA ARM7:  22.5792 MHz  (44.1 kHz × 512)
 *   - PVR2 pixel: 27 MHz pixel clock
 *
 * Rather than model each independently, we tick the CPU in fixed-size
 * slices and derive the other two from it. This keeps the code linear and
 * makes the frame time analytically computable, which matters for the
 * adaptive power governor.
 */
#ifndef DC_CORE_SCHEDULER_H
#define DC_CORE_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

#define DC_SH4_CLOCK_HZ     200000000ull
#define DC_FRAME_RATE_NTSC  60
#define DC_FRAME_RATE_PAL   50

/* SH-4 cycles per emulated frame. Truncated; the remainder is carried
 * across frames by the scheduler. */
#define DC_CYCLES_PER_FRAME_NTSC (DC_SH4_CLOCK_HZ / DC_FRAME_RATE_NTSC)
#define DC_CYCLES_PER_FRAME_PAL  (DC_SH4_CLOCK_HZ / DC_FRAME_RATE_PAL)

typedef struct dc dc_t;

typedef struct {
    uint64_t cycles_remaining;   /* budget for the current frame */
    uint32_t frame_rate;         /* 60 or 50 */
    uint32_t frame_number;
    bool     vblank_pending;     /* IRQ has been raised but not yet taken */
} dc_scheduler_t;

void dc_scheduler_init(dc_scheduler_t *s, uint32_t frame_rate);
void dc_scheduler_reset(dc_scheduler_t *s);

/* Called by the interpreter between basic blocks. Returns the number of
 * cycles the caller may still consume before yielding to the scheduler. */
uint64_t dc_scheduler_cycles_left(const dc_scheduler_t *s);

/* Consume `n` cycles. When the budget crosses zero the VBlank IRQ is
 * raised on the SH-4 (via dc_raise_vblank). */
void dc_scheduler_advance(dc_scheduler_t *s, dc_t *dc, uint64_t n);

/* Called once per presented frame to reset the budget. */
void dc_scheduler_begin_frame(dc_scheduler_t *s, uint32_t frame_rate);

#endif /* DC_CORE_SCHEDULER_H */
