/* Vita Dreamcast emulator — entry point.
 *
 * Responsibilities:
 *   1. Bring up the platform (video, audio, input, power).
 *   2. Locate BIOS and disc images under ux0:data/dc-vita/.
 *   3. Run the frame loop, pushing the Vita audio callback into the AICA
 *      mixer and the Vita framebuffer into the PVR2 presenter.
 *   4. Tear down on quit (L+R+Select).
 */
#include "vita/vita_platform.h"
#include "vita/vita_video.h"
#include "vita/vita_audio.h"
#include "vita/vita_input.h"
#include "vita/vita_power.h"
#include "core/dc.h"
#include "util/log.h"
#include "util/profiler.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>

#define DC_VITA_DATA_ROOT   "ux0:data/dc-vita"
#define DC_VITA_BIOS_DIR    DC_VITA_DATA_ROOT "/bios"
#define DC_VITA_ROMS_DIR    DC_VITA_DATA_ROOT "/roms"
#define DC_VITA_LOG_PATH    DC_VITA_DATA_ROOT "/dc-vita.log"

static dc_t g_dc;
static aica_t *g_aica_for_audio = NULL;

/* Audio callback: runs on the Vita audio thread. We only touch the AICA
 * mixer here, never SH-4 state. */
static void audio_fill(int16_t *out, int frames, void *user) {
    aica_fill_audio((aica_t *)user, out, frames);
}

/* ------------------------------------------------------------------ */
/* Image discovery                                                     */
/* ------------------------------------------------------------------ */

static bool find_first_in_dir(const char *dir, const char *const *exts,
                              size_t ext_count, char *out, size_t out_len) {
    DIR *d = opendir(dir);
    if (!d) return false;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        const char *dot = strrchr(ent->d_name, '.');
        if (!dot) continue;
        for (size_t i = 0; i < ext_count; i++) {
            if (strcasecmp(dot, exts[i]) == 0) {
                snprintf(out, out_len, "%s/%s", dir, ent->d_name);
                closedir(d);
                return true;
            }
        }
    }
    closedir(d);
    return false;
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    /* Best-effort log file; ignore failure (card may be read-only). */
    log_set_file(DC_VITA_LOG_PATH);
    log_set_level(LOG_INFO);

    LOG_I("main", "dc-vita starting");

    if (!vita_platform_init()) {
        LOG_E("main", "platform init failed");
        sceKernelExitProcess(1);
        return 1;
    }

    /* Wire the audio callback now that the platform port exists. */
    dc_init(&g_dc);
    g_aica_for_audio = &g_dc.aica;
    vita_audio_set_fill(audio_fill, g_aica_for_audio);

    /* BIOS: required. Try both flat and nested layouts. */
    char boot_path[512];
    char flash_path[512];
    snprintf(boot_path,  sizeof(boot_path),  DC_VITA_BIOS_DIR "/dc_boot.bin");
    snprintf(flash_path, sizeof(flash_path), DC_VITA_BIOS_DIR "/dc_flash.bin");

    if (!dc_load_bios(&g_dc, boot_path, flash_path)) {
        LOG_E("main", "BIOS missing; place dc_boot.bin and dc_flash.bin in " DC_VITA_BIOS_DIR);
        /* Still run the frame loop so the user sees a stable dark screen
         * instead of a crash. The BIOS area is zeroed, so the SH-4 will
         * trap on an unimplemented opcode and the loop will present a
         * still frame. */
    }

    /* Disc: optional. */
    static const char *const disc_exts[] = { ".gdi", ".cdi", ".chd" };
    char disc_path[512];
    if (find_first_in_dir(DC_VITA_ROMS_DIR, disc_exts,
                          sizeof(disc_exts) / sizeof(disc_exts[0]),
                          disc_path, sizeof(disc_path))) {
        LOG_I("main", "disc: %s", disc_path);
        dc_load_disc(&g_dc, disc_path);
    } else {
        LOG_W("main", "no disc image in " DC_VITA_ROMS_DIR);
    }

    /* Frame loop. Each iteration:
     *   1. Poll platform (input, power governor).
     *   2. Run one SH-4 frame.
     *   3. Present (skip in ECO on odd frames).
     *   4. Pump one audio grain. sceAudioOutOutput blocks until the port
     *      is ready, which paces the loop and prevents busy-waiting.
     */
    uint64_t last_frame_us = profiler_now_us();

    while (!vita_platform_should_quit()) {
        vita_platform_tick();

        dc_run_frame(&g_dc);

        if (!vita_power_should_skip_frame()) {
            pvr2_present(&g_dc.pvr2, &g_dc.mem,
                         vita_video_back_buffer(), vita_video_pitch(),
                         VITA_SCREEN_W, VITA_SCREEN_H);
            vita_video_present();
        }

        /* Audio: one grain per iteration. If audio is unavailable (no port),
         * skip; the frame loop still runs. */
        if (vita_audio_available()) {
            vita_audio_output();
        }

        uint64_t now = profiler_now_us();
        uint32_t dt  = (uint32_t)(now - last_frame_us);
        last_frame_us = now;
        vita_power_frame_time(dt);
    }

    LOG_I("main", "quit requested after %llu frames",
          (unsigned long long)dc_frame_count(&g_dc));

    vita_platform_shutdown();
    dc_shutdown(&g_dc);

    sceKernelExitProcess(0);
    return 0;
}
