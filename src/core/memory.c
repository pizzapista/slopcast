#include "core/memory.h"
#include "util/log.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* Region table                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    uint32_t    base;
    uint32_t    size;
    dc_region_t region;
} region_desc_t;

/* Ordered by base address. Overlaps are resolved by first match, which is
 * correct for the Dreamcast layout (nothing overlaps in practice). */
static const region_desc_t k_regions[] = {
    { DC_BOOT_ROM_BASE,  DC_BOOT_ROM_SIZE,  DC_REGION_BOOT_ROM  },
    { DC_HOLLY_BASE,     DC_HOLLY_SIZE,     DC_REGION_HOLLY     },
    { DC_SYSCTL_BASE,    DC_SYSCTL_SIZE,    DC_REGION_SYSCTL    },
    { DC_PVR2_BASE,      DC_PVR2_SIZE,      DC_REGION_PVR2      },
    { DC_GDROM_BASE,     DC_GDROM_SIZE,     DC_REGION_GDROM     },
    { DC_AICA_REG_BASE,  DC_AICA_REG_SIZE,  DC_REGION_AICA_REG  },
    { DC_AICA_RAM_BASE,  DC_AICA_RAM_SIZE,  DC_REGION_AICA_RAM  },
    { DC_RAM_BASE,       DC_RAM_SIZE,       DC_REGION_RAM       },
    { DC_VRAM_BASE,              DC_VRAM_SIZE / 4, DC_REGION_VRAM_64 },
    { DC_VRAM_BASE + 0x00800000, DC_VRAM_SIZE / 4, DC_REGION_VRAM_32 },
    { DC_VRAM_BASE + 0x01000000, DC_VRAM_SIZE / 4, DC_REGION_VRAM_16 },
    { DC_VRAM_BASE + 0x01800000, DC_VRAM_SIZE / 4, DC_REGION_VRAM_8  },
    { DC_SH4_ONCHIP_BASE, DC_SH4_ONCHIP_SIZE, DC_REGION_SH4_ONCHIP },
};

#define REGION_COUNT ((int)(sizeof(k_regions) / sizeof(k_regions[0])))

dc_region_t dc_mem_decode(uint32_t phys, uint32_t *out_offset) {
    for (int i = 0; i < REGION_COUNT; i++) {
        uint32_t base = k_regions[i].base;
        uint32_t size = k_regions[i].size;
        if (phys >= base && phys < base + size) {
            *out_offset = phys - base;
            return k_regions[i].region;
        }
    }
    *out_offset = 0;
    return DC_REGION_NONE;
}

bool dc_mem_is_writable(uint32_t phys) {
    uint32_t off;
    switch (dc_mem_decode(phys, &off)) {
        case DC_REGION_RAM:
        case DC_REGION_VRAM_64:
        case DC_REGION_VRAM_32:
        case DC_REGION_VRAM_16:
        case DC_REGION_VRAM_8:
        case DC_REGION_AICA_RAM:
            return true;
        default:
            return false;
    }
}

/* ------------------------------------------------------------------ */
/* Generic peripheral accessors                                        */
/* ------------------------------------------------------------------ */
/* These forward to the dc_t callback table set up by dc.c. For the
 * scaffold the table is a no-op reader/writer that logs the first N
 * accesses and returns zero. Real handlers get installed per region. */

/* We deliberately keep a weak default here so the interpreter can link
 * without dc.c being complete; dc.c overrides the symbols at init. */
__attribute__((weak))
uint32_t dc_peripheral_read(uint32_t phys, uint32_t width) {
    static int logged = 0;
    if (logged++ < 32) {
        LOG_W("mem", "unhandled peripheral read  phys=0x%08X width=%u",
              phys, width);
    }
    return 0;
}

__attribute__((weak))
void dc_peripheral_write(uint32_t phys, uint32_t width, uint32_t value) {
    static int logged = 0;
    if (logged++ < 32) {
        LOG_W("mem", "unhandled peripheral write phys=0x%08X width=%u val=0x%X",
              phys, width, value);
    }
}

/* ------------------------------------------------------------------ */
/* Read/write entry points                                             */
/* ------------------------------------------------------------------ */

uint32_t dc_mem_read8(dc_mem_stores_t *m, uint32_t phys) {
    uint8_t *p = dc_mem_ptr(m, phys, 1);
    if (p) return *p;
    return dc_peripheral_read(phys, 8);
}

uint32_t dc_mem_read16(dc_mem_stores_t *m, uint32_t phys) {
    uint8_t *p = dc_mem_ptr(m, phys, 2);
    if (p) {
        /* Dreamcast is little-endian; SH-4 too. */
        uint16_t v;
        memcpy(&v, p, 2);
        return v;
    }
    return dc_peripheral_read(phys, 16);
}

uint32_t dc_mem_read32(dc_mem_stores_t *m, uint32_t phys) {
    uint8_t *p = dc_mem_ptr(m, phys, 4);
    if (p) {
        uint32_t v;
        memcpy(&v, p, 4);
        return v;
    }
    return dc_peripheral_read(phys, 32);
}

void dc_mem_write8(dc_mem_stores_t *m, uint32_t phys, uint32_t v) {
    uint8_t *p = dc_mem_ptr(m, phys, 1);
    if (p) { *p = (uint8_t)v; return; }
    dc_peripheral_write(phys, 8, v);
}

void dc_mem_write16(dc_mem_stores_t *m, uint32_t phys, uint32_t v) {
    uint8_t *p = dc_mem_ptr(m, phys, 2);
    if (p) { uint16_t x = (uint16_t)v; memcpy(p, &x, 2); return; }
    dc_peripheral_write(phys, 16, v);
}

void dc_mem_write32(dc_mem_stores_t *m, uint32_t phys, uint32_t v) {
    uint8_t *p = dc_mem_ptr(m, phys, 4);
    if (p) { memcpy(p, &v, 4); return; }
    dc_peripheral_write(phys, 32, v);
}
