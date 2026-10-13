/* SH-4 -> ARMv7 dynamic recompiler.
 *
 * STUB in the scaffold: the dynarec currently just forwards to the
 * interpreter. The interface is final; the implementation is the M2
 * milestone in ROADMAP.md. */
#ifndef DC_SH4_DYNAREC_H
#define DC_SH4_DYNAREC_H

#include "sh4/sh4.h"

#include <stdint.h>

void sh4_dynarec_init(void);
void sh4_dynarec_shutdown(void);

/* Drop every cached block. Called on reset and after any write to a page
 * that was translated. */
void sh4_dynarec_flush_all(void);
void sh4_dynarec_flush_page(uint32_t phys_page);

/* Execute up to `budget` SH-4 cycles. Returns cycles consumed. The
 * reference implementation routes this straight into the interpreter. */
uint64_t sh4_dynarec_run(sh4_t *cpu, uint64_t budget);

/* Interpreter entry point (implemented in sh4_interpreter.c). */
uint64_t sh4_interpreter_run(sh4_t *cpu, uint64_t budget);

#endif /* DC_SH4_DYNAREC_H */
