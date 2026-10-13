/* Top-level Dreamcast system. Owns every subsystem and the shared memory
 * backing store. Nothing else in the codebase is allowed to hold a raw
 * pointer to RAM or VRAM.
 */
#ifndef DC_CORE_DC_H
#define DC_CORE_DC_H

#include "core/memory.h"
#include "core/scheduler.h"
#include "sh4/sh4.h"
#include "pvr2/pvr2.h"
#include "aica/aica.h"
#include "gdrom/gdrom.h"
#include "maple/maple.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct dc {
    dc_mem_stores_t  mem;
    dc_scheduler_t   sched;
    sh4_t            sh4;
    pvr2_t           pvr2;
    aica_t           aica;
    gdrom_t          gdrom;
    maple_t          maple;

    bool             bios_loaded;
    bool             disc_loaded;
    uint32_t         frame_rate;   /* 60 or 50 */
} dc_t;

/* Boot sequence: zero everything, wire subsystem pointers at the shared
 * memory store, and reset the CPU. */
void dc_init(dc_t *dc);
void dc_shutdown(dc_t *dc);

/* Load a 2 MB BIOS image. Returns false on size mismatch or read error. */
bool dc_load_bios(dc_t *dc, const char *boot_rom_path, const char *flash_path);

/* Mount a disc image. Parsing is M5 work; for now this records the path. */
bool dc_load_disc(dc_t *dc, const char *image_path);

/* Run one emulated frame's worth of SH-4 cycles. Returns true if the frame
 * budget was consumed normally, false if the CPU faulted. */
bool dc_run_frame(dc_t *dc);

/* Number of frames the emulator has executed since reset. */
uint64_t dc_frame_count(const dc_t *dc);

/* Convenience: current input state is pushed into the Maple bus from the
 * Vita platform layer between frames. */
void dc_push_input(dc_t *dc);

#endif /* DC_CORE_DC_H */
