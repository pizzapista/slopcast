/* GD-ROM drive.
 *
 * STUB in the scaffold. The interface is final; image parsing is the M5
 * milestone in ROADMAP.md.
 */
#ifndef DC_GDROM_H
#define DC_GDROM_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool     inserted;
    char     image_path[512];
    uint32_t sector_count;
    uint32_t sector_size;    /* 2048 for data, 2352 for raw */
} gdrom_t;

void gdrom_init(gdrom_t *gd);
void gdrom_reset(gdrom_t *gd);

/* Try to mount `path`. Returns true if the image was recognized and its
 * header parsed. Detecting the format (GDI, CDI, CHD) is M5 work; for the
 * scaffold this only records the path. */
bool gdrom_mount(gdrom_t *gd, const char *path);

/* Register read/write from the SH-4 bus at DC_GDROM_BASE. */
uint32_t gdrom_reg_read(gdrom_t *gd, uint32_t offset, uint32_t width);
void     gdrom_reg_write(gdrom_t *gd, uint32_t offset, uint32_t width, uint32_t value);

/* Read a 2048-byte sector into `dst`. Returns bytes copied. */
uint32_t gdrom_read_sector(gdrom_t *gd, uint32_t lba, uint8_t *dst, uint32_t len);

#endif /* DC_GDROM_H */
