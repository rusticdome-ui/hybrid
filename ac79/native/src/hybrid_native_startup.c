/* SPDX-License-Identifier: GPL-3.0-only
 * C runtime entry for the standalone target.
 *
 * The AC79 ROM/reset ABI and physical vector-table relocation must be supplied
 * by the verified board/SDK adapter before this entry can be used as a
 * flashable reset target. This file intentionally does not guess either.
 */
#include "hybrid_native_startup.h"

extern const hybrid_copy_region_t __hybrid_copy_table_start[];
extern const hybrid_copy_region_t __hybrid_copy_table_end[];
extern const hybrid_zero_region_t __hybrid_zero_table_start[];
extern const hybrid_zero_region_t __hybrid_zero_table_end[];

#if defined(__GNUC__)
__attribute__((weak))
#endif
int hybrid_native_early_board_init(void) { return 0; }

#if defined(__GNUC__)
__attribute__((weak))
#endif
int hybrid_native_main(void) { for (;;) { } }

void hybrid_native_reset_c(void)
{
    size_t copy_count = (size_t)(__hybrid_copy_table_end - __hybrid_copy_table_start);
    size_t zero_count = (size_t)(__hybrid_zero_table_end - __hybrid_zero_table_start);

    hybrid_native_memory_init(__hybrid_copy_table_start, copy_count,
                              __hybrid_zero_table_start, zero_count);

    if (hybrid_native_early_board_init() != 0) {
        for (;;) { /* fail closed: do not start audio with unknown clocks */ }
    }
    (void)hybrid_native_main();
    for (;;) { /* main must never return on bare metal */ }
}

void hybrid_native_default_irq(void)
{
    /* Safe default until every AC79 IRQ source is explicitly registered. */
}
