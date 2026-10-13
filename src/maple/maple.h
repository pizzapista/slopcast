/* Maple bus: controller and peripheral protocol.
 *
 * Partial in the scaffold: enough of the port enumeration is present that
 * a future controller can be attached without changing the interface.
 */
#ifndef DC_MAPLE_H
#define DC_MAPLE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t buttons;    /* active-low bitmask, matching the Dreamcast layout */
    uint8_t  ltrigger;   /* 0..255 */
    uint8_t  rtrigger;
    int8_t   joy_x;
    int8_t   joy_y;
    int8_t   joy2_x;     /* unused on the standard controller */
    int8_t   joy2_y;
} maple_controller_t;

typedef struct {
    /* Four physical ports; each may or may not have a peripheral. */
    maple_controller_t port[4];
    bool               present[4];

    /* DMA state. The SH-4 writes the DMA address and length to Holly and
     * we process the transfers in maple_reg_write. */
    uint32_t dma_address;
    uint32_t dma_length;
} maple_t;

void maple_init(maple_t *bus);
void maple_reset(maple_t *bus);

/* The emulator pushes the current input state into the bus before each
 * frame's SH-4 budget; the bus handles the DMA protocol on the SH-4 side. */
void maple_set_port_state(maple_t *bus, int port, const maple_controller_t *c);

/* Register read/write at DC_MAPLE_BASE (Holly window). */
uint32_t maple_reg_read(maple_t *bus, uint32_t offset, uint32_t width);
void     maple_reg_write(maple_t *bus, uint32_t offset, uint32_t width, uint32_t value);

#endif /* DC_MAPLE_H */
