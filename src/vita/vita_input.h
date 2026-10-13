#ifndef DC_VITA_INPUT_H
#define DC_VITA_INPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    /* Raw SCE_CTRL_* button bitmask, already edge-classified. */
    uint32_t buttons_down;    /* held this poll */
    uint32_t buttons_pressed; /* newly down since last poll */

    /* Left analog stick, signed 8-bit, [-128, 127]. */
    int8_t lx, ly;
    /* Right stick (Vita only; Dreamcast has one stick). */
    int8_t rx, ry;
} vita_input_state_t;

bool vita_input_init(void);
void vita_input_shutdown(void);

/* Called once per frame by vita_platform_tick(). */
void vita_input_poll(void);

const vita_input_state_t *vita_input_state(void);

/* Exposed for the menu hotkey check. */
uint32_t vita_input_raw_buttons(void);

#endif /* DC_VITA_INPUT_H */
