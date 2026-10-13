#include "sh4/sh4_dynarec.h"
#include "util/log.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* Block cache                                                         */
/* ------------------------------------------------------------------ */
/* Even in the stub we keep the structure so the interface is stable. */

#define BLOCK_CACHE_SLOTS 4096

typedef struct {
    uint32_t   guest_pc;      /* start of the translated range, 0 = empty */
    uint32_t   guest_end;     /* exclusive */
    uint32_t   host_code_off; /* offset into the code arena */
    uint32_t   host_code_len;
} block_entry_t;

static block_entry_t g_blocks[BLOCK_CACHE_SLOTS];
static uint32_t      g_block_count;

void sh4_dynarec_init(void) {
    memset(g_blocks, 0, sizeof(g_blocks));
    g_block_count = 0;
    LOG_I("drc", "block cache: %u slots (dynarec is a stub; interp fallback in use)",
          BLOCK_CACHE_SLOTS);
}

void sh4_dynarec_shutdown(void) {
    g_block_count = 0;
}

void sh4_dynarec_flush_all(void) {
    memset(g_blocks, 0, sizeof(g_blocks));
    g_block_count = 0;
}

void sh4_dynarec_flush_page(uint32_t phys_page) {
    /* TODO(impl): invalidate blocks whose guest_pc falls in this page.
     * For the stub we just flush everything on any write into translated
     * memory, which is correct but slow. */
    (void)phys_page;
}

/* ------------------------------------------------------------------ */
/* Execution                                                           */
/* ------------------------------------------------------------------ */

uint64_t sh4_dynarec_run(sh4_t *cpu, uint64_t budget) {
    /* TODO(impl): implement the M2 milestones:
     *   - discover basic blocks starting at cpu->regs.pc
     *   - look up a translated block in g_blocks
     *   - if absent, emit ARMv7 into the code arena and cache it
     *   - jump into the emitted code with the SH-4 register file as a
     *     struct pointer
     *   - return when the block ends, the budget is exhausted, or a
     *     guest write hits a page we must invalidate
     *
     * Until then, the interpreter is the only execution path. Every call
     * to sh4_run() therefore takes the slow route; that is honest, not a
     * hidden fast path.
     */
    return sh4_interpreter_run(cpu, budget);
}
