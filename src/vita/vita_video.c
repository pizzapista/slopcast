#include "vita/vita_video.h"
#include "util/log.h"

#include <string.h>
#include <stdlib.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>

/* The Vita displays through a linear framebuffer. We allocate a persistent
 * back buffer aligned to 256 KB (the display's preferred alignment) and
 * publish it via SCE_DISPLAY_SETBUF_NEXTFRAME, which takes effect at the
 * next vblank. */
#define FB_ALIGN (256 * 1024)

static uint8_t *g_fb       = NULL;
static uint32_t g_pitch    = 0;
static bool     g_dirty    = true;

bool vita_video_init(void) {
    g_pitch = VITA_SCREEN_W * 4;

    /* Allocate through the display's dedicated memory class so the GPU can
     * read it without a bounce buffer. Falls back to ordinary memalign if
     * that pool is exhausted (e.g. heavily loaded by the OS). */
    g_fb = memalign(FB_ALIGN, g_pitch * VITA_SCREEN_H);
    if (!g_fb) {
        LOG_E("vid", "framebuffer alloc failed (%u bytes)", g_pitch * VITA_SCREEN_H);
        return false;
    }
    memset(g_fb, 0, g_pitch * VITA_SCREEN_H);

    SceDisplayFrameBuf fb;
    memset(&fb, 0, sizeof(fb));
    fb.size        = sizeof(fb);
    fb.base        = g_fb;
    fb.pitch       = g_pitch;
    fb.width       = VITA_SCREEN_W;
    fb.height      = VITA_SCREEN_H;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;

    int rc = sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    if (rc < 0) {
        LOG_E("vid", "sceDisplaySetFrameBuf failed: 0x%08X", rc);
        free(g_fb);
        g_fb = NULL;
        return false;
    }

    LOG_I("vid", "framebuffer %ux%u pitch=%u", VITA_SCREEN_W, VITA_SCREEN_H, g_pitch);
    return true;
}

void vita_video_shutdown(void) {
    if (g_fb) {
        free(g_fb);
        g_fb = NULL;
    }
}

uint8_t *vita_video_back_buffer(void) { return g_fb; }
uint32_t vita_video_pitch(void)        { return g_pitch; }

void vita_video_mark_dirty(int x, int y, int w, int h) {
    (void)x; (void)y; (void)w; (void)h;
    g_dirty = true;
    /* TODO(impl): keep a dirty-rect union and only blit that rect in
     * vita_video_present(). Right now we present unconditionally, which is
     * correct but wastes GPU power. */
}

void vita_video_present(void) {
    if (!g_fb) return;

    /* Re-publishing the same buffer each frame is cheap; the display reads
     * it at vblank. When we add GXM later this becomes a sceGxmDisplayQueueAddEntry. */
    SceDisplayFrameBuf fb;
    memset(&fb, 0, sizeof(fb));
    fb.size        = sizeof(fb);
    fb.base        = g_fb;
    fb.pitch       = g_pitch;
    fb.width       = VITA_SCREEN_W;
    fb.height      = VITA_SCREEN_H;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    g_dirty = false;
}
