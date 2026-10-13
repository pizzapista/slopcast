/* Aggregated Vita platform init/teardown. Every module in src/vita/ exposes
 * a self-contained init/deinit pair; the platform module just sequences them. */
#ifndef DC_VITA_PLATFORM_H
#define DC_VITA_PLATFORM_H

#include <stdbool.h>

/* Order matters: video before audio (audio thread may want to log to screen
 * during bring-up), input before the emulator loop, power last so the first
 * frame is measured at the requested clock. */
bool vita_platform_init(void);
void vita_platform_shutdown(void);

/* Called once per emulated frame by the main loop. Keeps the platform's
 * own timing bookkeeping consistent (frame counter, battery sampler). */
void vita_platform_tick(void);

/* True if the user has requested a quit via L+R+Select. */
bool vita_platform_should_quit(void);

#endif /* DC_VITA_PLATFORM_H */
