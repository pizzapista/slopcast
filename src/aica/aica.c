#include "aica/aica.h"
#include "util/log.h"

#include <string.h>

void aica_init(aica_t *aica, dc_mem_stores_t *mem) {
    (void)mem;
    memset(aica, 0, sizeof(*aica));
    aica->audio.sample_rate = 44100;
    LOG_I("aica", "audio stub initialized (see ROADMAP M4)");
}

void aica_reset(aica_t *aica) {
    memset(aica->regs, 0, sizeof(aica->regs));
    aica->audio.armed = false;
}

uint32_t aica_reg_read(aica_t *aica, uint32_t offset, uint32_t width) {
    (void)width;
    if (offset >= sizeof(aica->regs)) return 0;
    uint32_t v = 0;
    memcpy(&v, aica->regs + offset, 4);
    return v;
}

void aica_reg_write(aica_t *aica, uint32_t offset, uint32_t width, uint32_t value) {
    (void)width;
    if (offset + 4 > sizeof(aica->regs)) return;
    memcpy(aica->regs + offset, &value, 4);
    aica->audio.armed = true; /* any write wakes the mixer; a real impl checks ARM reset */
}

void aica_fill_audio(aica_t *aica, int16_t *out, int frames) {
    (void)aica;
    /* Silence. Replaced by the real 64-channel mixer in M4. */
    memset(out, 0, (size_t)frames * 2 * sizeof(int16_t));
}
