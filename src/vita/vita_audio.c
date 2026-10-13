#include "vita/vita_audio.h"
#include "util/log.h"

#include <string.h>
#include <psp2/audioout.h>
#include <psp2/sysmodule.h>

static int                g_port = -1;
static vita_audio_fill_cb g_fill = NULL;
static void              *g_user = NULL;
static int16_t            g_grain[VITA_AUDIO_GRAIN * 2]; /* L,R interleaved */

bool vita_audio_init(void) {
    /* SCE_SYSMODULE_AUDIO is auto-loaded on recent firmware, but calling
     * the load is idempotent and harmless. */
    sceSysmoduleLoadModule(SCE_SYSMODULE_AUDIO);

    g_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,
                                 VITA_AUDIO_GRAIN,
                                 VITA_AUDIO_SAMPLE_RATE,
                                 SCE_AUDIO_OUT_MODE_STEREO);
    if (g_port < 0) {
        LOG_W("aud", "sceAudioOutOpenPort failed: 0x%08X (audio disabled)", g_port);
        g_port = -1;
        return false;
    }

    /* Ask the kernel to keep the output warm across brief underruns;
     * without this, a slow frame drops the mixer for a full grain. */
    sceAudioOutSetConfig(g_port, -1, VITA_AUDIO_SAMPLE_RATE, SCE_AUDIO_OUT_MODE_STEREO);

    LOG_I("aud", "audio port %d, %d Hz stereo, grain %d",
          g_port, VITA_AUDIO_SAMPLE_RATE, VITA_AUDIO_GRAIN);
    return true;
}

void vita_audio_shutdown(void) {
    if (g_port >= 0) {
        sceAudioOutReleasePort(g_port);
        g_port = -1;
    }
}

void vita_audio_set_fill(vita_audio_fill_cb cb, void *user) {
    g_fill = cb;
    g_user = user;
}

bool vita_audio_available(void) {
    return g_port >= 0;
}

bool vita_audio_output(void) {
    if (g_port < 0) return false;

    if (g_fill) {
        g_fill(g_grain, VITA_AUDIO_GRAIN, g_user);
    } else {
        memset(g_grain, 0, sizeof(g_grain));
    }

    int rc = sceAudioOutOutput(g_port, g_grain);
    if (rc < 0) {
        LOG_W("aud", "sceAudioOutOutput failed: 0x%08X", rc);
        return false;
    }
    return true;
}
