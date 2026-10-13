#include "pvr2/pvr2.h"
#include "pvr2/pvr2_registers.h"
#include "util/log.h"

#include <string.h>

void pvr2_init(pvr2_t *gpu) {
    memset(gpu, 0, sizeof(*gpu));
    LOG_I("pvr2", "GPU stub initialized (see ROADMAP M3)");
}

void pvr2_reset(pvr2_t *gpu) {
    memset(gpu, 0, sizeof(*gpu));
}

uint32_t pvr2_reg_read(pvr2_t *gpu, uint32_t offset, uint32_t width) {
    (void)width;
    if (offset + 4 > sizeof(gpu->regs)) return 0;
    return gpu->regs[offset / 4];
}

void pvr2_reg_write(pvr2_t *gpu, uint32_t offset, uint32_t width, uint32_t value) {
    (void)width;
    if (offset + 4 > sizeof(gpu->regs)) return;

    uint32_t addr = DC_PVR2_BASE + offset;

    /* The command FIFO: writes here enqueue a 32-bit TA command. The real
     * FIFO can be in one of several states (header, vertex data, end of
     * list), so decoding requires a state machine that we haven't built
     * yet. For now, we count FIFO writes and log the first few. */
    if (addr >= 0x005F8000u && addr < 0x005F8100u) {
        static int logged = 0;
        if (logged++ < 16) {
            LOG_D("pvr2", "FIFO write @0x%08X = 0x%08X (op=%u)",
                  addr, value, value >> 29);
        }
        gpu->polys_submitted++;
        return;
    }

    /* Framebuffer / tile accelerator registers. */
    uint32_t idx = offset / 4;
    if (idx < sizeof(gpu->regs) / sizeof(gpu->regs[0])) {
        gpu->regs[idx] = value;
    }

    if (addr == PVR2_REG_FB_R_CTRL || addr == PVR2_REG_FB_W_CTRL) {
        gpu->dirty_frame = true;
    }
}

void pvr2_end_frame(pvr2_t *gpu, const dc_mem_stores_t *mem) {
    (void)mem;

    /* TODO(impl): M3 milestones:
     *   1. Walk the tile bin lists built up by pvr2_reg_write().
     *   2. Sort each tile's list by z.
     *   3. Rasterize opaque, then punch-through, then translucent passes.
     *   4. Decode textures through the texture cache.
     *   5. Write the finished image into VRAM at the address in FB_W_SOF1.
     *
     * For the scaffold we draw a diagnostic pattern so you can see the
     * emulator is alive: a gray field with a moving 60 Hz scan line. */
    if (mem->vram == NULL) return;
    /* Deliberately leave VRAM alone: the presenter clears to black. */
}

void pvr2_present(pvr2_t *gpu, const dc_mem_stores_t *mem,
                  uint8_t *dst, uint32_t dst_pitch,
                  int dst_w, int dst_h) {
    (void)gpu;
    (void)mem;

    /* Clear to a dark gray so the boot screen is visibly "not a crash". */
    const uint32_t bg = 0x101014FFu; /* RGBA */
    for (int y = 0; y < dst_h; y++) {
        uint32_t *row = (uint32_t *)(dst + (size_t)y * dst_pitch);
        for (int x = 0; x < dst_w; x++) {
            row[x] = bg;
        }
    }
}
