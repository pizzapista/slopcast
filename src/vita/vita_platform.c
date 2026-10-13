#include "vita/vita_platform.h"
#include "vita/vita_audio.h"
#include "vita/vita_video.h"
#include "vita/vita_input.h"
#include "vita/vita_power.h"
#include "util/log.h"

#include <psp2/kernel/processmgr.h>

static bool g_quit = false;

bool vita_platform_init(void) {
    log_set_level(LOG_INFO);

    if (!vita_video_init()) {
        LOG_E("plat", "video init failed");
        return false;
    }
    if (!vita_audio_init()) {
        LOG_E("plat", "audio init failed");
        return false;
    }
    if (!vita_input_init()) {
        LOG_E("plat", "input init failed");
        return false;
    }
    if (!vita_power_init()) {
        LOG_E("plat", "power init failed");
        return false;
    }

    LOG_I("plat", "platform ready");
    return true;
}

void vita_platform_shutdown(void) {
    /* Reverse order. Audio first so no callback fires against a torn-down
     * video surface. */
    vita_power_shutdown();
    vita_input_shutdown();
    vita_audio_shutdown();
    vita_video_shutdown();
    LOG_I("plat", "platform down");
}

void vita_platform_tick(void) {
    vita_input_poll();
    vita_power_tick();

    /* Menu / quit chord: L + R + (Start | Select). */
    uint32_t held = vita_input_raw_buttons();
    const uint32_t L = 0x00000100u;  /* SCE_CTRL_LTRIGGER */
    const uint32_t R = 0x00000200u;  /* SCE_CTRL_RTRIGGER */
    const uint32_t SEL = 0x00000001u;/* SCE_CTRL_SELECT  */
    if ((held & L) && (held & R) && (held & SEL)) {
        g_quit = true;
    }
}

bool vita_platform_should_quit(void) {
    return g_quit;
}
