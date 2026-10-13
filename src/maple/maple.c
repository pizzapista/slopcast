#include "maple/maple.h"
#include "util/log.h"

#include <string.h>

void maple_init(maple_t *bus) {
    memset(bus, 0, sizeof(*bus));
    LOG_I("maple", "bus stub initialized (see ROADMAP M6)");
}

void maple_reset(maple_t *bus) {
    memset(bus->port, 0, sizeof(bus->port));
    memset(bus->present, 0, sizeof(bus->present));
    bus->dma_address = 0;
    bus->dma_length  = 0;
}

void maple_set_port_state(maple_t *bus, int port, const maple_controller_t *c) {
    if (port < 0 || port >= 4) return;
    bus->port[port]  = *c;
    bus->present[port] = true;
}

uint32_t maple_reg_read(maple_t *bus, uint32_t offset, uint32_t width) {
    (void)bus; (void)offset; (void)width;
    return 0;
}

void maple_reg_write(maple_t *bus, uint32_t offset, uint32_t width, uint32_t value) {
    /* TODO(impl): decode the Holly DMA registers at DC_HOLLY_BASE. The
     * protocol is: SH-4 writes a command frame to its own RAM, points
     * Holly at it, and Holly DMAs both directions. We only need to
     * implement the subset that a standard controller uses. */
    (void)bus; (void)offset; (void)width; (void)value;
}
