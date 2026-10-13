#include "core/dc.h"
#include "util/log.h"
#include "util/profiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Static profiler; visible to the whole emulator via dc_profiler(). */
static profiler_t g_prof;
static int        g_prof_frame_id = -1;
static int        g_prof_sh4_id   = -1;
static int        g_prof_pvr2_id  = -1;
static int        g_prof_audio_id = -1;

profiler_t *dc_profiler(void) { return &g_prof; }

void dc_init(dc_t *dc) {
    memset(dc, 0, sizeof(*dc));

    /* Register defaults. */
    dc->frame_rate = 60;

    /* Wire the shared memory store into the CPU and every peripheral. */
    sh4_init(&dc->sh4, &dc->mem);
    pvr2_init(&dc->pvr2);
    aica_init(&dc->aica, &dc->mem);
    gdrom_init(&dc->gdrom);
    maple_init(&dc->maple);
    dc_scheduler_init(&dc->sched, dc->frame_rate);

    profiler_init(&g_prof);
    g_prof_frame_id = profiler_add(&g_prof, "frame");
    g_prof_sh4_id   = profiler_add(&g_prof, "sh4");
    g_prof_pvr2_id  = profiler_add(&g_prof, "pvr2");
    g_prof_audio_id = profiler_add(&g_prof, "audio");

    LOG_I("dc", "system initialized");
}

void dc_shutdown(dc_t *dc) {
    sh4_dynarec_shutdown();
    profiler_dump(&g_prof);
    (void)dc;
}

bool dc_load_bios(dc_t *dc, const char *boot_rom_path, const char *flash_path) {
    FILE *f = fopen(boot_rom_path, "rb");
    if (!f) {
        LOG_E("dc", "cannot open BIOS: %s", boot_rom_path);
        return false;
    }
    size_t n = fread(dc->mem.boot_rom, 1, DC_BOOT_ROM_SIZE, f);
    fclose(f);
    if (n != DC_BOOT_ROM_SIZE) {
        LOG_E("dc", "BIOS size mismatch: got %zu, expected %u",
              n, DC_BOOT_ROM_SIZE);
        return false;
    }

    /* Flash is 128 KB on the Dreamcast. We only check it opens; the
     * scaffold doesn't yet emulate flash access. */
    if (flash_path) {
        FILE *fl = fopen(flash_path, "rb");
        if (!fl) {
            LOG_W("dc", "flash not found: %s (continuing without)", flash_path);
        } else {
            fseek(fl, 0, SEEK_END);
            long sz = ftell(fl);
            fclose(fl);
            LOG_I("dc", "flash %s (%ld bytes)", flash_path, sz);
        }
    }

    dc->bios_loaded = true;
    LOG_I("dc", "BIOS loaded (%u bytes)", DC_BOOT_ROM_SIZE);
    return true;
}

bool dc_load_disc(dc_t *dc, const char *image_path) {
    if (!gdrom_mount(&dc->gdrom, image_path)) {
        LOG_E("dc", "disc mount failed: %s", image_path);
        return false;
    }
    dc->disc_loaded = true;
    return true;
}

bool dc_run_frame(dc_t *dc) {
    profiler_frame_begin(&g_prof);
    profiler_begin(&g_prof, g_prof_frame_id);

    dc_scheduler_begin_frame(&dc->sched, dc->frame_rate);

    /* Run the CPU until the frame budget is exhausted or it faults. The
     * scheduler raises the VBlank IRQ at the boundary; the interpreter
     * picks it up on its next instruction when we implement IRQ entry. */
    profiler_begin(&g_prof, g_prof_sh4_id);
    uint64_t budget = dc_scheduler_cycles_left(&dc->sched);
    uint64_t used   = sh4_run(&dc->sh4, budget);
    profiler_end(&g_prof, g_prof_sh4_id);

    dc_scheduler_advance(&dc->sched, dc, used);

    profiler_begin(&g_prof, g_prof_pvr2_id);
    pvr2_end_frame(&dc->pvr2, &dc->mem);
    profiler_end(&g_prof, g_prof_pvr2_id);

    profiler_end(&g_prof, g_prof_frame_id);
    profiler_frame_end(&g_prof);
    return true;
}

uint64_t dc_frame_count(const dc_t *dc) {
    return dc->sched.frame_number;
}

void dc_push_input(dc_t *dc) {
    /* The Vita platform layer calls this with the current input state.
     * Wiring it into the Maple DMA protocol is M6 work; for the scaffold
     * we just stash the state so a future controller implementation can
     * read it. */
    (void)dc;
}
