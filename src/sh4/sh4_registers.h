/* SH-4 architectural register file.
 *
 * The layout here is chosen so that the interpreter can access R0..R15 as
 * a contiguous array (helping the compiler keep them in registers on ARM),
 * with the control/status registers grouped separately. The dynarec will
 * pin R0..R7, SP, and PR into ARM registers and spill the rest back to
 * this struct at block boundaries.
 */
#ifndef DC_SH4_REGISTERS_H
#define DC_SH4_REGISTERS_H

#include <stdint.h>

typedef struct {
    /* General-purpose registers. R0..R7 are banked; the "current" bank
     * is selected by SR.RB. We keep both banks and mirror the active pair
     * into r[0..7] for convenience. */
    union {
        uint32_t r[16];
        struct {
            uint32_t r0,  r1,  r2,  r3,  r4,  r5,  r6,  r7;
            uint32_t r8,  r9,  r10, r11, r12, r13, r14, sp;
        };
    };

    uint32_t r_bank[2][8];   /* [bank][index 0..7] */

    uint32_t pc;             /* program counter */
    uint32_t pr;             /* procedure link register (return address) */

    /* Status / control registers. */
    uint32_t sr;             /* status register */
    uint32_t gbr;            /* global base register */
    uint32_t vbr;            /* vector base register */
    uint32_t ssr;            /* saved status register */
    uint32_t spc;            /* saved program counter */
    uint32_t sgr;            /* saved general register (banked R15) */
    uint32_t dbr;            /* debug base register */

    /* Multiplier-accumulator. */
    uint32_t mach;
    uint32_t macl;

    /* FPU. */
    uint32_t fpscr;
    uint32_t fpul;
    union {
        uint32_t fr_bits[16];
        float    fr[16];
        struct {
            float fr0,  fr1,  fr2,  fr3;
            float fr4,  fr5,  fr6,  fr7;
            float fr8,  fr9,  fr10, fr11;
            float fr12, fr13, fr14, fr15;
        };
    };
    union {
        uint32_t xf_bits[16];
        float    xf[16];
    };

    /* Set when the interpreter must take an interrupt before the next
     * instruction. Set by the scheduler; cleared on entry to the handler. */
    uint32_t pending_irq_mask;
} sh4_regs_t;

/* SR bit positions. */
#define SH4_SR_T    (1u << 0)   /* true / carry */
#define SH4_SR_S    (1u << 1)   /* saturation */
#define SH4_SR_Q    (1u << 8)   /* signed overflow */
#define SH4_SR_M    (1u << 9)   /* division step */
#define SH4_SR_FD   (1u << 15)  /* FPU disable */
#define SH4_SR_BL   (1u << 28)  /* banked R15 (SGR) valid */
#define SH4_SR_RB   (1u << 29)  /* register bank select */
#define SH4_SR_MD   (1u << 30)  /* processing mode: 0=user, 1=privileged */
#define SH4_SR_BL_MASK (1u << 28)
#define SH4_SR_IMASK   (0xFu << 4)

#endif /* DC_SH4_REGISTERS_H */
