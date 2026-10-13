/* AICA: audio subsystem (ARM7TDMI + 64-channel sample mixer + DSP).
 *
 * STUB in the scaffold. The interface is final; the ARM7 interpreter and
 * mixer are the M4 milestone in ROADMAP.md.
 */
#ifndef DC_AICA_H
#define DC_AICA_H

#include "core/memory.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    /* AICA register file at DC_AICA_REG_BASE. */
    uint8_t  regs[DC_AICA_REG_SIZE];

    /* The ARM7 CPU would live here; for now only the audio path is
     * represented so the mixer can be brought up before the CPU. */
    struct {
        bool     armed;
        uint32_t sample_rate;  /* 44100 */
    } audio;
} aica_t;

void aica_init(aica_t *aica, dc_mem_stores_t *mem);
void aica_reset(aica_t *aica);

/* MMIO from the SH-4 bus (AICA register window). */
uint32_t aica_reg_read(aica_t *aica, uint32_t offset, uint32_t width);
void     aica_reg_write(aica_t *aica, uint32_t offset, uint32_t width, uint32_t value);

/* Fill `frames` stereo samples. Called from the Vita audio callback; runs
 * on the audio thread, so it must not touch SH-4 state. */
void aica_fill_audio(aica_t *aica, int16_t *out, int frames);

#endif /* DC_AICA_H */
