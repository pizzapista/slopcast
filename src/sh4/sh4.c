#include "sh4/sh4.h"
#include "sh4/sh4_dynarec.h"
#include "util/log.h"

#include <string.h>

void sh4_init(sh4_t *cpu, dc_mem_stores_t *mem) {
    memset(cpu, 0, sizeof(*cpu));
    cpu->mem = mem;
    sh4_reset(cpu);
    sh4_dynarec_init();
}

void sh4_reset(sh4_t *cpu) {
    memset(&cpu->regs, 0, sizeof(cpu->regs));
    /* Real hardware comes out of reset with SR.MD=1 (privileged), IMASK=0,
     * and PC pointing into the reset vector. The BIOS sets the real PC. */
    cpu->regs.sr  = SH4_SR_MD;
    cpu->regs.pc  = 0xA0000000u;   /* reset vector in the BIOS area */
    cpu->cycles   = 0;
    cpu->irq_pending = false;
    cpu->irq_source  = SH4_INT_NONE;
    sh4_dynarec_flush_all();
}

void sh4_raise_interrupt(sh4_t *cpu, sh4_int_t source) {
    /* Only the highest-priority source is tracked; lower ones are dropped
     * because the scheduler will raise them again next frame. */
    if ((int)source <= (int)cpu->irq_source) return;
    cpu->irq_source  = (uint32_t)source;
    cpu->irq_pending = true;
}

uint64_t sh4_run(sh4_t *cpu, uint64_t budget) {
    /* The dynarec, once implemented, will service almost all of these
     * cycles without returning here. For now everything goes through the
     * interpreter, which is the reference implementation. */
    return sh4_dynarec_run(cpu, budget);
}

/* --- Memory helpers --------------------------------------------------- */

uint32_t sh4_read32(sh4_t *cpu, uint32_t va) {
    return dc_mem_read32(cpu->mem, va);
}
uint16_t sh4_read16(sh4_t *cpu, uint32_t va) {
    return (uint16_t)dc_mem_read16(cpu->mem, va);
}
uint8_t sh4_read8(sh4_t *cpu, uint32_t va) {
    return (uint8_t)dc_mem_read8(cpu->mem, va);
}
void sh4_write32(sh4_t *cpu, uint32_t va, uint32_t v) {
    dc_mem_write32(cpu->mem, va, v);
}
void sh4_write16(sh4_t *cpu, uint32_t va, uint16_t v) {
    dc_mem_write16(cpu->mem, va, v);
}
void sh4_write8(sh4_t *cpu, uint32_t va, uint8_t v) {
    dc_mem_write8(cpu->mem, va, v);
}
