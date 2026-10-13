#ifndef DC_VITA_VIDEO_H
#define DC_VITA_VIDEO_H

#include <stdbool.h>
#include <stdint.h>

#define VITA_SCREEN_W 960
#define VITA_SCREEN_H 544

bool vita_video_init(void);
void vita_video_shutdown(void);

/* Pointer to the currently mapped, CPU-writable framebuffer. Format is
 * RGBA8888 unless vita_video_set_format() was called with RGB565.
 * Pitch is VITA_SCREEN_W * 4 (or * 2) bytes. */
uint8_t *vita_video_back_buffer(void);
uint32_t vita_video_pitch(void);

/* Publish the back buffer to the display for the next vblank. On the first
 * call after a resize this will also handle the pending flip. Cheap; safe
 * to call every frame even if nothing changed (the display just re-presents
 * the same pixels). */
void vita_video_present(void);

/* Mark a rectangle as dirty. The present path can use this to skip the
 * VRAM→FB blit for tiles that haven't changed since the last frame. */
void vita_video_mark_dirty(int x, int y, int w, int h);

#endif /* DC_VITA_VIDEO_H */
