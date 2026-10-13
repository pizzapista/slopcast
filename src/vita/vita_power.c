#include "vita/vita_power.h"
#include "util/log.h"
#include "util/profiler.h"

#include <psp2/power.h>

static power_profile_t g_profile  = POWER_PROFILE_BALANCED;
static bool            g_adaptive = false;
static int             g_battery  = -1;
static uint32_t        g_battery_tick = 0;

static const struct {
    int cpu_mhz;
    int gpu_mhz;
    int bus_mhz;
} k_profiles[] = {
    [POWER_PROFILE_PERFORMANCE] = { 444, 444, 222 },
    [POWER_PROFILE_BALANCED]    = { 333, 333, 166 },
    [POWER_PROFILE_ECO]         = { 111, 222, 111 },
};

static void apply_profile(power_profile_t p) {
    if (p == g_profile) return;
    int rc = scePowerSetClockFrequency(k_profiles[p].cpu_mhz,
                                       k_profiles[p].gpu_mhz,
                                       k_profiles[p].bus_mhz);
    if (rc < 0) {
        LOG_W("pwr", "scePowerSetClockFrequency(%d) failed: 0x%08X",
              k_profiles[p].cpu_mhz, rc);
        return;
    }
    LOG_I("pwr", "profile -> %d (%d/%d/%d MHz)",
          p, k_profiles[p].cpu_mhz, k_profiles[p].gpu_mhz, k_profiles[p].bus_mhz);
    g_profile = p;
}

bool vita_power_init(void) {
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);

    /* Start at BALANCED; the adaptive tick will push to PERFORMANCE if the
     * first few frames come in under budget. */
    apply_profile(POWER_PROFILE_BALANCED);
    g_battery = scePowerGetBatteryLifePercent();
    LOG_I("pwr", "battery %d%%, profile %d", g_battery, g_profile);
    return true;
}

void vita_power_shutdown(void) {
    /* Return the SoC to a safe default so a crashed emulator doesn't leave
     * the whole system underclocked. */
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
}

void vita_power_set_profile(power_profile_t profile) {
    g_adaptive = false;
    apply_profile(profile);
}

void vita_power_set_adaptive(bool adaptive) {
    g_adaptive = adaptive;
}

int vita_power_battery_percent(void) { return g_battery; }

bool vita_power_is_charging(void) {
    return scePowerIsBatteryCharging() != 0;
}

static uint32_t g_last_frame_us = 16667; /* seeded to 60 Hz */
static uint32_t g_slow_streak = 0;
static uint32_t g_fast_streak = 0;

void vita_power_frame_time(uint32_t us) {
    g_last_frame_us = us;
}

void vita_power_tick(void) {
    /* Poll battery once every 60 frames. */
    if ((g_battery_tick++ % 60u) == 0u) {
        int b = scePowerGetBatteryLifePercent();
        if (b != g_battery) {
            LOG_I("pwr", "battery %d%%", b);
            g_battery = b;
        }
    }

    if (!g_adaptive) return;

    /* Adaptive policy: if we're consistently over 20 ms/frame, back off to
     * ECO. If we're consistently under 14 ms, try to climb back up.
     * Hysteresis via streak counters prevents oscillation. */
    if (g_last_frame_us > 20000u) {
        g_slow_streak++;
        g_fast_streak = 0;
    } else if (g_last_frame_us < 14000u) {
        g_fast_streak++;
        g_slow_streak = 0;
    } else {
        g_slow_streak = g_fast_streak = 0;
    }

    if (g_slow_streak >= 30) {
        if (g_profile == POWER_PROFILE_PERFORMANCE)
            apply_profile(POWER_PROFILE_BALANCED);
        else if (g_profile == POWER_PROFILE_BALANCED)
            apply_profile(POWER_PROFILE_ECO);
        g_slow_streak = 0;
    } else if (g_fast_streak >= 120) {
        if (g_profile == POWER_PROFILE_ECO)
            apply_profile(POWER_PROFILE_BALANCED);
        else if (g_profile == POWER_PROFILE_BALANCED)
            apply_profile(POWER_PROFILE_PERFORMANCE);
        g_fast_streak = 0;
    }
}

bool vita_power_should_skip_frame(void) {
    /* In ECO, present every other frame. The emulated vblank still runs at
     * full rate; only the display update is throttled. */
    static uint32_t counter = 0;
    if (g_profile != POWER_PROFILE_ECO) return false;
    counter++;
    return (counter & 1u) != 0u;
}
