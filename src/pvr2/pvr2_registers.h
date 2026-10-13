/* PowerVR2 register file and command opcodes.
 *
 * The PVR2 is programmed through a command FIFO at DC_PVR2_BASE plus a
 * parallel register file for the framebuffer, tile accelerator, and
 * sort-DMA configuration. The macros below name the ones the rasterizer
 * needs first; they are not exhaustive.
 */
#ifndef DC_PVR2_REGISTERS_H
#define DC_PVR2_REGISTERS_H

#include <stdint.h>

/* --- Framebuffer / tile accelerator --------------------------------- */
#define PVR2_REG_FB_R_CTRL          0x005F8048u   /* read framebuffer control */
#define PVR2_REG_FB_W_CTRL          0x005F804Cu   /* write framebuffer control */
#define PVR2_REG_FB_W_LINESTRIDE    0x005F8050u
#define PVR2_REG_FB_W_SOF1          0x005F8054u
#define PVR2_REG_FB_W_SOF2          0x005F8058u
#define PVR2_REG_FB_X_CLIP          0x005F805Cu
#define PVR2_REG_FB_Y_CLIP          0x005F8060u

#define PVR2_REG_FPU_SHAD_SCALE     0x005F8064u
#define PVR2_REG_FPU_CULL_MODE      0x005F8068u
#define PVR2_REG_FPU_PARAM_CFG      0x005F806Cu

#define PVR2_REG_ISP_BACKGND_D      0x005F8068u
#define PVR2_REG_ISP_BACKGND_T      0x005F806Cu
#define PVR2_REG_ISP_FEED_CFG       0x005F8074u

#define PVR2_REG_SPG_LOAD_ISP       0x005F8080u
#define PVR2_REG_SPG_LOAD_ISP       0x005F8080u
#define PVR2_REG_SPG_LOAD_TA        0x005F8084u
#define PVR2_REG_SPG_LOAD_TA        0x005F8084u

#define PVR2_REG_SPG_HBLANK_INT     0x005F80B0u
#define PVR2_REG_SPG_VBLANK_INT     0x005F80B4u
#define PVR2_REG_SPG_CONTROL        0x005F80B8u
#define PVR2_REG_SPG_STATUS         0x005F80BCu
#define PVR2_REG_SPG_TRIGGER        0x005F80C0u

#define PVR2_REG_PT_ALPHA_REF       0x005F8100u
#define PVR2_REG_PT_INDEX           0x005F8104u
#define PVR2_REG_PT_BASE            0x005F8108u
#define PVR2_REG_PT_SIZE            0x005F810Cu
#define PVR2_REG_PT_BASE1           0x005F8110u
#define PVR2_REG_PT_SIZE1           0x005F8114u

#define PVR2_REG_TA_OL_BASE         0x005F8124u
#define PVR2_REG_TA_ISP_BASE        0x005F8128u
#define PVR2_REG_TA_ISP_LIMIT       0x005F812Cu
#define PVR2_REG_TA_OL_POINTERS     0x005F8130u
#define PVR2_REG_TA_VERTEX_BUF      0x005F8134u
#define PVR2_REG_TA_IT       0x005F8138u

#define PVR2_REG_ISPTSP_BASE        0x005F8140u
#define PVR2_REG_ISPTSP_LIMIT       0x005F8144u

#define PVR2_REG_YUV_ADDR           0x005F8148u
#define PVR2_REG_YUV_CFG            0x005F814Cu

#define PVR2_REG_SCL                0x005F8144u   /* conflicting aliases exist; verify against HW */

/* --- Command FIFO ---------------------------------------------------- */
/* TA commands are 32-bit; the top 3 bits are a 3-bit opcode. Writes to the
 * FIFO happen through a register aperture; the exact address depends on
 * the FIFO's current state, so the decoder in pvr2.c tracks a running
 * write pointer rather than treating FIFO writes as ordinary MMIO. */
#define PVR2_TA_CMD(opcode, payload)  ((((uint32_t)(opcode)) << 29) | ((payload) & 0x1FFFFFFFu))

/* Opcode numbers (from the PowerVR2 "series 2" ISA, as used by Dreamcast). */
#define PVR2_OP_END_LIST            0x00
#define PVR2_OP_POLY_OR_TRIM        0x04
#define PVR2_OP_POLY_CHROME         0x05
#define PVR2_OP_POLY_NORMAL         0x07   /* wait, the real numbers are below */

/* Actual PVR2 opcodes (7 bits of the top 3 bits + list type bits).
 * These are the ones we will implement first. */
#define PVR2_TA_OP_END_LIST         0x00u
#define PVR2_TA_OP_USER_TILE        0x01u
#define PVR2_TA_OP_POLY_OR_TRIM     0x04u
#define PVR2_TA_OP_POLY_SPRITE      0x05u
#define PVR2_TA_OP_POLY_CHROME      0x06u
#define PVR2_TA_OP_POLY_NORMAL      0x07u
#define PVR2_TA_OP_SPRITE           0x08u
#define PVR2_TA_OP_VERTEX_8          0x0Cu
#define PVR2_TA_OP_VERTEX_16         0x0Du
#define PVR2_TA_OP_VERTEX_32         0x0Eu
#define PVR2_TA_OP_VERTEX_FLOAT      0x0Fu

#endif /* DC_PVR2_REGISTERS_H */
