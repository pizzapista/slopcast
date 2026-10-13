#include "vita/vita_input.h"
#include "util/log.h"

#include <string.h>
#include <psp2/ctrl.h>

static vita_input_state_t g_state;
static uint32_t           g_prev_buttons = 0;

bool vita_input_init(void) {
    memset(&g_state, 0, sizeof(g_state));
    /* Analog mode gives us the stick; without it we only get the d-pad. */
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
    LOG_I("inp", "input ready");
    return true;
}

void vita_input_shutdown(void) {
    /* Nothing to release; SceCtrl is a kernel-managed singleton. */
}

void vita_input_poll(void) {
    SceCtrlData pad;
    memset(&pad, 0, sizeof(pad));
    if (sceCtrlPeekBufferPositive(0, &pad, 1) < 0) {
        return;
    }

    g_state.buttons_down    = pad.buttons;
    g_state.buttons_pressed = pad.buttons & ~g_prev_buttons;
    g_state.lx              = (int8_t)(pad.lx - 128);
    g_state.ly              = (int8_t)(pad.ly - 128);
    g_state.rx              = (int8_t)(pad.rx - 128);
    g_state.ry              = (int8_t)(pad.ry - 128);
    g_prev_buttons          = pad.buttons;
}

const vita_input_state_t *vita_input_state(void) { return &g_state; }
uint32_t vita_input_raw_buttons(void)            { return g_state.buttons_down; }
