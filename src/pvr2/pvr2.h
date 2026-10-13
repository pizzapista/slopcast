/* PowerVR2 GPU.
 *
 * STUB in the scaffold. The interface is final; the tile-deferred
 * rasterizer is the M3 milestone in ROADMAP.md.
 */
#ifndef DC_PVR2_H
#define DC_PVR2_H

#include "core/memory.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    /* Register file, indexed by (addr - DC_PVR2_BASE) >> 2. */
    uint32_t regs[DC_PVR2_SIZE / 4];

    /* Tile binning state. */
    struct {
        uint32_t tile_lists[16 * 16];  /* 512x512/32x32 = 16x16 tiles max */
        uint32_t tile_count;
    } binning;

    /* Statistics for the profiler. */
    uint32_t polys_submitted;
    uint32_t polys_drawn;
    uint32_t tile_bins_sorted;

    bool     dirty_frame;
} pvr2_t;

void pvr2_init(pvr2_t *gpu);
void pvr2_reset(pvr2_t *gpu);

/* Register read/write from the SH-4 memory bus. Returns the value read;
 * writes are applied to the register file or the command FIFO state. */
uint32_t pvr2_reg_read(pvr2_t *gpu, uint32_t offset, uint32_t width);
void     pvr2_reg_write(pvr2_t *gpu, uint32_t offset, uint32_t width, uint32_t value);

/* Called once per emulated frame, after the SH-4 budget is exhausted and
 * before the platform presents. */
void pvr2_end_frame(pvr2_t *gpu, const dc_mem_stores_t *mem);

/* Blit the current VRAM framebuffer into `dst` (RGBA8888, pitch `dst_pitch`).
 * `scale_x`/`scale_y` are the requested scale factors (1 or 2). */
void pvr2_present(pvr2_t *gpu, const dc_mem_stores_t *mem,
                  uint8_t *dst, uint32_t dst_pitch,
                  int dst_w, int dst_h);

#endif /* DC_PVR2_H */
