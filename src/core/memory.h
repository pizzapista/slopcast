/* Dreamcast physical address decode.
 *
 * The SH-4 exposes a 32-bit virtual space; the Dreamcast maps system
 * peripherals into the low 16 MB and the RAM/VRAM into distinct upper
 * ranges. This module is the single owner of that mapping: nothing else in
 * the codebase is allowed to know a literal physical address.
 */
#ifndef DC_CORE_MEMORY_H
#define DC_CORE_MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* --- Well-known region bases and sizes ------------------------------- */

#define DC_BOOT_ROM_BASE   0x00000000u
#define DC_BOOT_ROM_SIZE   (2u * 1024u * 1024u)

#define DC_HOLLY_BASE      0x005F6800u
#define DC_HOLLY_SIZE      0x00000800u

#define DC_SYSCTL_BASE     0x005F7000u
#define DC_SYSCTL_SIZE     0x00001000u

#define DC_PVR2_BASE       0x005F8000u
#define DC_PVR2_SIZE       0x00002000u

#define DC_GDROM_BASE      0x00600000u
#define DC_GDROM_SIZE      0x00000800u

#define DC_AICA_REG_BASE   0x00700000u
#define DC_AICA_REG_SIZE   0x00000800u

#define DC_AICA_RAM_BASE   0x00800000u
#define DC_AICA_RAM_SIZE   (2u * 1024u * 1024u)

#define DC_RAM_BASE        0x0C000000u
#define DC_RAM_SIZE        (16u * 1024u * 1024u)

#define DC_VRAM_BASE       0x10000000u
#define DC_VRAM_SIZE       (8u * 1024u * 1024u)

#define DC_SH4_ONCHIP_BASE 0xFF000000u
#define DC_SH4_ONCHIP_SIZE 0x01000000u

/* --- Region classification ------------------------------------------- */

typedef enum {
    DC_REGION_NONE = 0,
    DC_REGION_BOOT_ROM,
    DC_REGION_HOLLY,
    DC_REGION_SYSCTL,
    DC_REGION_PVR2,
    DC_REGION_GDROM,
    DC_REGION_AICA_REG,
    DC_REGION_AICA_RAM,
    DC_REGION_RAM,
    DC_REGION_VRAM_64,
    DC_REGION_VRAM_32,
    DC_REGION_VRAM_16,
    DC_REGION_VRAM_8,
    DC_REGION_SH4_ONCHIP,
} dc_region_t;

/* Translate an SH-4 physical address into a region and an offset within it.
 * `out_offset` receives the byte offset within the region's backing store. */
dc_region_t dc_mem_decode(uint32_t phys, uint32_t *out_offset);

/* True if the address is writable without hitting a read-only region. */
bool dc_mem_is_writable(uint32_t phys);

/* --- Backing stores --------------------------------------------------- */
/* These are owned by dc_t; this module only resolves addresses. */

typedef struct {
    uint8_t boot_rom[DC_BOOT_ROM_SIZE];
    uint8_t ram[DC_RAM_SIZE];
    uint8_t vram[DC_VRAM_SIZE];
    uint8_t aica_ram[DC_AICA_RAM_SIZE];
} dc_mem_stores_t;

/* --- Accessors -------------------------------------------------------- */
/* These are the fast paths. They are inlined into the interpreter; the
 * region decoder is used only on the slow path (peripheral accesses). */

static inline uint8_t *dc_mem_ptr(dc_mem_stores_t *m, uint32_t phys, uint32_t len) {
    /* Fast path: main RAM covers the overwhelming majority of SH-4 traffic. */
    if (phys >= DC_RAM_BASE && phys + len <= DC_RAM_BASE + DC_RAM_SIZE) {
        return m->ram + (phys - DC_RAM_BASE);
    }
    if (phys >= DC_VRAM_BASE && phys + len <= DC_VRAM_BASE + DC_VRAM_SIZE) {
        return m->vram + (phys - DC_VRAM_BASE);
    }
    if (phys >= DC_BOOT_ROM_BASE && phys + len <= DC_BOOT_ROM_BASE + DC_BOOT_ROM_SIZE) {
        return m->boot_rom + (phys - DC_BOOT_ROM_BASE);
    }
    if (phys >= DC_AICA_RAM_BASE && phys + len <= DC_AICA_RAM_BASE + DC_AICA_RAM_SIZE) {
        return m->aica_ram + (phys - DC_AICA_RAM_BASE);
    }
    return NULL;
}

/* --- Load/store through the region decoder --------------------------- */
/* Peripheral accesses go through these. They are slower but handle every
 * register-range read/write via the dc_region_t callback table (see dc.h). */

uint32_t dc_mem_read8 (dc_mem_stores_t *m, uint32_t phys);
uint32_t dc_mem_read16(dc_mem_stores_t *m, uint32_t phys);
uint32_t dc_mem_read32(dc_mem_stores_t *m, uint32_t phys);
void     dc_mem_write8 (dc_mem_stores_t *m, uint32_t phys, uint32_t v);
void     dc_mem_write16(dc_mem_stores_t *m, uint32_t phys, uint32_t v);
void     dc_mem_write32(dc_mem_stores_t *m, uint32_t phys, uint32_t v);

#endif /* DC_CORE_MEMORY_H */
