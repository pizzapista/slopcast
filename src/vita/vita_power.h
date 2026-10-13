#ifndef DC_VITA_POWER_H
#define DC_VITA_POWER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    POWER_PROFILE_PERFORMANCE = 0, /* 444/444/222 MHz */
    POWER_PROFILE_BALANCED,        /* 333/333/166 MHz */
    POWER_PROFILE_ECO              /* 111/222/111 MHz + frame skip */
} power_profile_t;

bool vita_power_init(void);
void vita_power_shutdown(void);

/* Called once per frame. Handles battery polling and, if the caller has
 * opted into adaptive mode, may nudge the profile up or down. */
void vita_power_tick(void);

/* Explicit profile selection; also flips the "adaptive" flag off. */
void vita_power_set_profile(power_profile_t profile);

/* Let the tick() function adjust the clock between the two dynamic profiles
 * based on measured frame time. */
void vita_power_set_adaptive(bool adaptive);

/* 0..100, or -1 if the battery reading failed. */
int  vita_power_battery_percent(void);
bool vita_power_is_charging(void);

/* If true, the caller should skip rendering this frame (used by ECO). */
bool vita_power_should_skip_frame(void);

#endif /* DC_VITA_POWER_H */
