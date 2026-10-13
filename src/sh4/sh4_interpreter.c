/* SH-4 reference interpreter.
 *
 * This is deliberately written as a straightforward switch on the top
 * nibble. The dynarec must reproduce this byte-for-byte; whenever the two
 * disagree, this file is authoritative. Opcodes not yet implemented call
 * sh4_unimplemented() which logs PC, opcode, and the fetched bytes, then
 * halts the CPU (so a failure is loud rather than silently wrong).
 */
#include "sh4/sh4.h"
#include "util/log.h"
#include "util/profiler.h"

#include <string.h>

/* ---- helpers -------------------------------------------------------- */

static inline uint32_t reg_read(const sh4_regs_t *r, unsigned n) {
    return (n == 15 && (r->sr & SH4_SR_BL)) ? r->sgr : r->r[n];
}
static inline void reg_write(sh4_regs_t *r, unsigned n, uint32_t v) {
    if (n == 15 && (r->sr & SH4_SR_BL)) r->sgr = v;
    else                                  r->r[n] = v;
}

static inline void set_t(sh4_regs_t *r, bool v) {
    if (v) r->sr |=  SH4_SR_T;
    else   r->sr &= ~SH4_SR_T;
}
static inline bool get_t(const sh4_regs_t *r) {
    return (r->sr & SH4_SR_T) != 0;
}

/* Sign-extend a `bits`-wide value from a shift amount in an immediate. */
static inline int32_t sext(uint32_t v, unsigned bits) {
    uint32_t m = 1u << (bits - 1);
    return (int32_t)((v ^ m) - m);
}

/* --- fault handling -------------------------------------------------- */

static void sh4_unimplemented(sh4_t *cpu, uint16_t opcode) {
    uint32_t pc = cpu->regs.pc;
    uint16_t a = sh4_read16(cpu, pc - 2);
    uint16_t b = sh4_read16(cpu, pc);
    LOG_E("sh4", "UNIMPLEMENTED opcode 0x%04X at pc=0x%08X (next: 0x%04X 0x%04X)",
          opcode, pc, a, b);
    /* Park the CPU at an infinite self-loop at the current PC. This makes
     * the failure visible (frame time collapses to zero work) instead of
     * continuing into garbage. */
    cpu->regs.pc = pc - 2;
    cpu->single_step = true;
}

/* --- dispatch -------------------------------------------------------- */
/* Returns the number of SH-4 cycles consumed by the instruction just
 * executed. Most SH-4 instructions are 1 cycle; loads, stores, and
 * branches are more. The numbers here are the nominal SH-4 values; the
 * scheduler charges the running total, not per-instruction. */

static uint32_t exec_one(sh4_t *cpu) {
    sh4_regs_t *r = &cpu->regs;
    uint32_t pc  = r->pc;
    uint16_t insn = sh4_read16(cpu, pc);
    uint32_t next_pc = pc + 2;
    uint32_t cycles  = 1;

    uint32_t n  = (insn >> 8) & 0xF;   /* Rn */
    uint32_t m  = (insn >> 4) & 0xF;   /* Rm */
    uint32_t d  = insn & 0xF;          /* nibble */

    switch (insn >> 12) {

    /* --- 0x0 : special ------------------------------------------------ */
    case 0x0: {
        uint32_t sub = insn & 0xFF;
        switch (sub) {
        case 0x03: /* BSRF Rn */
            r->pr = next_pc;
            next_pc = pc + 4 + reg_read(r, n);
            cycles = 2;
            break;
        case 0x09: /* NOP */
            break;
        case 0x0B: /* RTS */
            next_pc = r->pr;
            cycles = 2;
            break;
        case 0x23: /* BRAF Rn */
            next_pc = pc + 4 + reg_read(r, n);
            cycles = 2;
            break;
        case 0x29: /* MOVT Rn */
            reg_write(r, n, get_t(r) ? 1u : 0u);
            break;
        case 0x2B: /* RTE */
            next_pc = r->spc;
            r->sr   = r->ssr;
            cycles  = 2;
            break;
        case 0x08: /* CLRT */
            set_t(r, false);
            break;
        case 0x18: /* SETT */
            set_t(r, true);
            break;
        case 0x28: /* CLRMAC */
            r->mach = 0; r->macl = 0;
            break;
        case 0x19: /* DIV0U */
            r->sr &= ~(SH4_SR_Q | SH4_SR_M | SH4_SR_T);
            break;
        case 0x00 ... 0x02: /* ??? (undefined) */
        default:
            /* Fall through to load/store/other sub-decoders below if the
             * upper nibble is 0 but the sub-code isn't a special. */
            goto n_0_other;
        }
        break;
    }

    /* --- 0x1 : MOV.L Rm, @(disp, Rn) ---------------------------------- */
    case 0x1: {
        uint32_t disp = (insn & 0xF) << 2;
        uint32_t addr = reg_read(r, n) + disp;
        sh4_write32(cpu, addr, reg_read(r, m));
        cycles = 2;
        break;
    }

    /* --- 0x2 : MOV.L @(disp, Rm), Rn ---------------------------------- */
    case 0x2: {
        uint32_t disp = (insn & 0xF) << 2;
        uint32_t addr = reg_read(r, m) + disp;
        reg_write(r, n, sh4_read32(cpu, addr));
        cycles = 2;
        break;
    }

    /* --- 0x3 : ALU ---------------------------------------------------- */
    case 0x3: {
        uint32_t a = reg_read(r, n);
        uint32_t b = reg_read(r, m);
        switch (insn & 0xF) {
        case 0x0: reg_write(r, n, a + b); break;               /* ADD */
        case 0x1: {                                            /* ADDC */
            uint64_t s = (uint64_t)a + b + (get_t(r) ? 1u : 0u);
            reg_write(r, n, (uint32_t)s);
            set_t(r, s > 0xFFFFFFFFull);
            break;
        }
        case 0x2: {                                            /* ADDV */
            int64_t s = (int64_t)(int32_t)a + (int64_t)(int32_t)b;
            reg_write(r, n, (uint32_t)s);
            if (s >  0x7FFFFFFFll || s < -0x80000000ll) r->sr |=  SH4_SR_Q;
            else                                         r->sr &= ~SH4_SR_Q;
            break;
        }
        case 0x4: reg_write(r, n, a - b); break;               /* SUB */
        case 0x5: {                                            /* SUBC */
            uint64_t s = (uint64_t)a - b - (get_t(r) ? 1u : 0u);
            reg_write(r, n, (uint32_t)s);
            set_t(r, (s >> 63) & 1u);
            break;
        }
        case 0x6: {                                            /* SUBV */
            int64_t s = (int64_t)(int32_t)a - (int64_t)(int32_t)b;
            reg_write(r, n, (uint32_t)s);
            if (s >  0x7FFFFFFFll || s < -0x80000000ll) r->sr |=  SH4_SR_Q;
            else                                         r->sr &= ~SH4_SR_Q;
            break;
        }
        case 0x7: reg_write(r, n, a + b); break;               /* ADD (alt encoding) */
        case 0x8: reg_write(r, n, a & b); break;               /* AND */
        case 0x9: reg_write(r, n, a ^ b); break;               /* XOR */
        case 0xA: reg_write(r, n, a | b); break;               /* OR  */
        case 0xB: reg_write(r, n, a * b); cycles = 2; break;   /* MUL.L */
        case 0xC: {                                            /* TST */
            set_t(r, (a & b) == 0);
            break;
        }
        case 0xD: {                                            /* XTRCT */
            reg_write(r, n, (a >> 16) | (b << 16));
            break;
        }
        case 0xE: {                                            /* MULU.W */
            uint32_t p = (a & 0xFFFF) * (b & 0xFFFF);
            r->macl = p;
            break;
        }
        case 0xF: {                                            /* MULS.W */
            int32_t sa = (int16_t)a;
            int32_t sb = (int16_t)b;
            r->macl = (uint32_t)(sa * sb);
            break;
        }
        }
        break;
    }

    /* --- 0x4 : ALU / shifts ------------------------------------------- */
    case 0x4: {
        switch (insn & 0xFF) {
        case 0x00: r->sr = (r->sr & ~SH4_SR_S) | (r->sh4_reserved_shl(r, n), 0), 0; break; /* placeholder — handled below */
        default:
            break;
        }
        /* The 0x4 block uses a different sub-decode; split it out so the
         * common ALU cases don't get buried. Most of the SH-4 shift/rotate
         * encodings live here. */
        uint32_t sub = insn & 0xFF;
        uint32_t val = reg_read(r, n);
        switch (sub) {
        case 0x00: reg_write(r, n, val << 1);  set_t(r, (val >> 31) & 1); break; /* SHLL  */
        case 0x01: reg_write(r, n, val >> 1);  set_t(r, val & 1);         break; /* SHLR  */
        case 0x04: reg_write(r, n, (uint32_t)((int32_t)val << 1)); set_t(r, (val >> 31) & 1); break; /* SHAL */
        case 0x05: reg_write(r, n, (uint32_t)((int32_t)val >> 1)); set_t(r, val & 1);         break; /* SHAR */
        case 0x08: { /* SHLL2 */
            uint32_t v = val << 2;
            reg_write(r, n, v);
            break;
        }
        case 0x09: reg_write(r, n, val >> 2);  break; /* SHLR2 */
        case 0x18: reg_write(r, n, val << 8);  break; /* SHLL8 */
        case 0x19: reg_write(r, n, val >> 8);  break; /* SHLR8 */
        case 0x20: reg_write(r, n, val << 16); break; /* SHLL16 */
        case 0x21: reg_write(r, n, val >> 16); break; /* SHLR16 */
        case 0x24: { /* ROTL */
            bool t = (val >> 31) & 1;
            reg_write(r, n, (val << 1) | (t ? 1u : 0u));
            set_t(r, t);
            break;
        }
        case 0x25: { /* ROTR */
            bool t = val & 1;
            reg_write(r, n, (val >> 1) | (t ? 0x80000000u : 0u));
            set_t(r, t);
            break;
        }
        default:
            sh4_unimplemented(cpu, insn);
            return 0;
        }
        break;
    }

    /* --- 0x5 : MOV.L @(disp, Rm), Rn ---------------------------------- */
    case 0x5: {
        uint32_t disp = (insn & 0xF) << 2;
        uint32_t addr = reg_read(r, m) + disp;
        reg_write(r, n, sh4_read32(cpu, addr));
        cycles = 2;
        break;
    }

    /* --- 0x6 : MOV.L Rm, @(disp, Rn) ---------------------------------- */
    case 0x6: {
        uint32_t disp = (insn & 0xF) << 2;
        uint32_t addr = reg_read(r, n) + disp;
        sh4_write32(cpu, addr, reg_read(r, m));
        cycles = 2;
        break;
    }

    /* --- 0x7 : MUL.L -------------------------------------------------- */
    case 0x7: {
        uint32_t a = reg_read(r, n);
        uint32_t b = reg_read(r, m);
        r->macl = a * b;
        cycles = 2;
        break;
    }

    /* --- 0x8 : MOV.B R0, @(disp, Rn) ---------------------------------- */
    case 0x8: {
        switch ((insn >> 8) & 0xF) {
        case 0x0: { /* MOV.B R0, @(disp,Rn) */
            uint32_t disp = insn & 0xF;
            uint32_t addr = reg_read(r, n) + disp;
            sh4_write8(cpu, addr, (uint8_t)r->r0);
            cycles = 1;
            break;
        }
        case 0x1: { /* MOV.W R0, @(disp,Rn) */
            uint32_t disp = (insn & 0xF) << 1;
            uint32_t addr = reg_read(r, n) + disp;
            sh4_write16(cpu, addr, (uint16_t)r->r0);
            cycles = 1;
            break;
        }
        case 0x4: { /* MOV.B @(disp,Rm), R0 */
            uint32_t disp = insn & 0xF;
            uint32_t addr = reg_read(r, m) + disp;
            r->r0 = sh4_read8(cpu, addr);
            cycles = 1;
            break;
        }
        case 0x5: { /* MOV.W @(disp,Rm), R0 */
            uint32_t disp = (insn & 0xF) << 1;
            uint32_t addr = reg_read(r, m) + disp;
            r->r0 = (uint32_t)(int32_t)(int16_t)sh4_read16(cpu, addr);
            cycles = 1;
            break;
        }
        case 0x8: { /* MOV.B R0, @(R0,Rm) */
            uint32_t addr = r->r0 + reg_read(r, m);
            sh4_write8(cpu, addr, (uint8_t)r->r0);
            cycles = 1;
            break;
        }
        case 0x9: { /* MOV.W R0, @(R0,Rm) */
            uint32_t addr = r->r0 + reg_read(r, m);
            sh4_write16(cpu, addr, (uint16_t)r->r0);
            cycles = 1;
            break;
        }
        case 0xC: { /* MOV.B @(R0,Rm), R0 */
            uint32_t addr = r->r0 + reg_read(r, m);
            r->r0 = sh4_read8(cpu, addr);
            cycles = 1;
            break;
        }
        case 0xD: { /* MOV.W @(R0,Rm), R0 */
            uint32_t addr = r->r0 + reg_read(r, m);
            r->r0 = (uint32_t)(int32_t)(int16_t)sh4_read16(cpu, addr);
            cycles = 1;
            break;
        }
        default:
            sh4_unimplemented(cpu, insn);
            return 0;
        }
        break;
    }

    /* --- 0x9 : MOV.W @(disp, PC), Rn ---------------------------------- */
    case 0x9: {
        uint32_t disp = (insn & 0xFF) << 1;
        uint32_t addr = pc + 4 + disp;
        reg_write(r, n, (uint32_t)(int32_t)(int16_t)sh4_read16(cpu, addr));
        cycles = 1;
        break;
    }

    /* --- 0xA : BRA / BSR ---------------------------------------------- */
    case 0xA: {
        int32_t disp = sext(insn & 0xFFF, 12) << 1;
        if ((insn & 0xF000) == 0xA000) {           /* BRA */
            next_pc = pc + 4 + disp;
            cycles  = 2;
        } else {                                   /* BSR */
            r->pr   = pc + 4;
            next_pc = pc + 4 + disp;
            cycles  = 2;
        }
        break;
    }

    /* --- 0xB : BSR / BRA ---------------------------------------------- */
    case 0xB: {
        /* Only 0xB000..0xBFFF exists as BSR (BRA is 0xA). Handled above. */
        if ((insn & 0xF000) == 0xB000) {
            int32_t disp = sext(insn & 0xFFF, 12) << 1;
            r->pr   = pc + 4;
            next_pc = pc + 4 + disp;
            cycles  = 2;
        } else {
            sh4_unimplemented(cpu, insn);
            return 0;
        }
        break;
    }

    /* --- 0xC : BT / BF / BTS / BFS / BT/S / BF/S ----------------------- */
    case 0xC: {
        int32_t disp = sext(insn & 0xFF, 8) << 1;
        uint32_t top = insn >> 8;
        switch (top) {
        case 0x8: /* BT */
            if (get_t(r)) next_pc = pc + 4 + disp;
            break;
        case 0x9: /* BF */
            if (!get_t(r)) next_pc = pc + 4 + disp;
            break;
        case 0xB: /* BF/S (delay slot) */
            if (!get_t(r)) next_pc = pc + 4 + disp;
            cycles = 2;
            break;
        case 0xD: /* BT/S (delay slot) */
            if (get_t(r)) next_pc = pc + 4 + disp;
            cycles = 2;
            break;
        case 0x8 | 0x0: /* BTS (with T set side effect) */
            set_t(r, true);
            if (get_t(r)) next_pc = pc + 4 + disp;
            break;
        case 0x9 | 0x0: /* BFS */
            set_t(r, false);
            if (!get_t(r)) next_pc = pc + 4 + disp;
            break;
        default:
            sh4_unimplemented(cpu, insn);
            return 0;
        }
        break;
    }

    /* --- 0xD : MOV.L @(disp, PC), Rn ---------------------------------- */
    case 0xD: {
        uint32_t disp = (insn & 0xFF) << 2;
        uint32_t addr = (pc & ~3u) + 4 + disp;
        reg_write(r, n, sh4_read32(cpu, addr));
        cycles = 1;
        break;
    }

    /* --- 0xE : MOV #imm, Rn ------------------------------------------- */
    case 0xE: {
        int32_t imm = sext(insn & 0xFF, 8);
        reg_write(r, n, (uint32_t)imm);
        break;
    }

    /* --- 0xF : FPU (deferred) ----------------------------------------- */
    case 0xF: {
        /* The FPU is intentionally not implemented in this scaffold; see
         * ROADMAP.md M1. Trapping here is better than guessing. */
        sh4_unimplemented(cpu, insn);
        return 0;
    }

    n_0_other:
        sh4_unimplemented(cpu, insn);
        return 0;

    default:
        sh4_unimplemented(cpu, insn);
        return 0;
    }

    r->pc = next_pc;
    return cycles;
}

/* --- public entry ---------------------------------------------------- */

uint64_t sh4_interpreter_run(sh4_t *cpu, uint64_t budget) {
    uint64_t used = 0;

    while (used < budget) {
        if (cpu->irq_pending) {
            /* TODO(impl): mask against SR.IMASK, save SR/PC to SSR/SPC,
             * set SR.MD=1, and jump to VBR + 0x600. For now we simply
             * clear the flag and continue; the BIOS boot path doesn't
             * depend on vblank delivery yet. */
            cpu->irq_pending = false;
        }

        uint32_t n = exec_one(cpu);
        if (n == 0) {
            /* Unimplemented instruction: exec_one already logged and
             * parked. Stop this slice so the caller can present a frame. */
            break;
        }
        used += n;
    }

    cpu->cycles += used;
    return used;
}
