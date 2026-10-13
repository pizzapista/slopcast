#include "gdrom/gdrom.h"
#include "util/log.h"

#include <string.h>

void gdrom_init(gdrom_t *gd) {
    memset(gd, 0, sizeof(*gd));
    gd->sector_size = 2048;
}

void gdrom_reset(gdrom_t *gd) {
    bool inserted = gd->inserted;
    char path[sizeof(gd->image_path)];
    strncpy(path, gd->image_path, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';

    memset(gd, 0, sizeof(*gd));
    gd->sector_size = 2048;
    gd->inserted    = inserted;
    strncpy(gd->image_path, path, sizeof(gd->image_path) - 1);
}

bool gdrom_mount(gdrom_t *gd, const char *path) {
    /* TODO(impl): M5 milestones:
     *   - sniff the first 4 bytes to distinguish GDI (text), CDI ("CDI"),
     *     and CHD ("MComPrHD")
     *   - for GDI, parse track table lines "<track> <lba> <type> <sectors> <file> <offset>"
     *   - for CDI, parse the header at 0x0000
     *   - for CHD, hand off to the CHD reader (M5)
     *   - build an LBA->(file, offset) index so read_sector is O(1)
     * For now we record the path and mark the drive as "inserted but empty",
     * which lets the BIOS boot into its no-disc menu rather than crashing. */
    strncpy(gd->image_path, path, sizeof(gd->image_path) - 1);
    gd->inserted     = true;
    gd->sector_count = 0;
    LOG_W("gd", "image '%s' recorded but not parsed (ROADMAP M5)", path);
    return true;
}

uint32_t gdrom_reg_read(gdrom_t *gd, uint32_t offset, uint32_t width) {
    (void)gd; (void)offset; (void)width;
    return 0;
}

void gdrom_reg_write(gdrom_t *gd, uint32_t offset, uint32_t width, uint32_t value) {
    (void)gd; (void)offset; (void)width; (void)value;
}

uint32_t gdrom_read_sector(gdrom_t *gd, uint32_t lba, uint8_t *dst, uint32_t len) {
    (void)gd; (void)lba;
    memset(dst, 0, len);
    return len;
}
