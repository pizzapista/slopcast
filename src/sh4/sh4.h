/* SH-4 CPU front-end: register file, reset, interrupt entry, and the
 * dispatch entry point that the scheduler calls. The actual decode and
 * execute lives in sh4_interpreter.c; the dynarec in sh4_dynarec.c.
 */
#ifndef DC_SH4_H
#define DC_SH4_H

#include "sh4/sh4_registers.h"
#include "core/memory.h"

#include <stdint.h>
#include <stdbool.h>

/* Interrupt priority levels (mapped onto SR.IMASK). VBlank is the highest
 * priority the emulator currently generates. */
typedef enum {
    SH4_INT_NONE   = 0,
    SH4_INT_VBLANK = 1,
    SH4_INT_HBLANK = 2,
    SH4_INT_GDROM  = 3,
    SH4_INT_AICA   = 4,
    SH4_INT_MAPLE  = 5,
} sh4_int_t;

typedef struct dc dc_t;

typedef struct {
    sh4_regs_t regs;

    /* Backing store pointer is stable for the emulator's lifetime. */
    dc_mem_stores_t *mem;

    /* Running total, updated by the interpreter. Used by the scheduler to
     * charge the frame budget and by the dynarec for block profiling. */
    uint64_t cycles;

    /* When true, the interpreter executes one instruction at a time and
     * checks the pending_irq_mask before each. The dynarec uses this as a
     * fallback whenever a block cannot be translated. */
    bool single_step;

    /* Set by sh4_raise_interrupt; the interpreter checks it between
     * instructions when single_step, or at block boundaries otherwise. */
    bool irq_pending;
    uint32_t irq_source;
} sh4_t;

void sh4_init(sh4_t *cpu, dc_mem_stores_t *mem);
void sh4_reset(sh4_t *cpu);

/* Signal an interrupt from a peripheral. The actual delivery is deferred
 * to the interpreter at the next SR-checked boundary. */
void sh4_raise_interrupt(sh4_t *cpu, sh4_int_t source);

/* Execute `budget` SH-4 cycles (or until the CPU raises a fault the
 * emulator must service). Returns the number of cycles actually consumed. */
uint64_t sh4_run(sh4_t *cpu, uint64_t budget);

/* Direct memory helpers used by the interpreter and the peripheral
 * handlers. They go through the region decoder in core/memory.c. */
uint32_t sh4_read32(sh4_t *cpu, uint32_t va);
uint16_t sh4_read16(sh4_t *cpu, uint32_t va);
uint8_t  sh4_read8 (sh4_t *cpu, uint32_t va);
void     sh4_write32(sh4_t *cpu, uint32_t va, uint32_t v);
void     sh4_write16(sh4_t *cpu, uint32_t va, uint16_t v);
void     sh4_write8 (sh4_t *cpu, uint32_t va, uint8_t  v);

#endif /* DC_SH4_H */
