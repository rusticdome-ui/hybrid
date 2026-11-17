/* SPDX-License-Identifier: GPL-3.0-only */
#include "hybrid_native_startup.h"

void hybrid_native_memory_init(const hybrid_copy_region_t *copy, size_t copy_count,
                               const hybrid_zero_region_t *zero, size_t zero_count)
{
    size_t i;
    for (i = 0; i < copy_count; ++i) {
        size_t n;
        if (!copy[i].load || !copy[i].run) continue;
        for (n = 0; n < copy[i].size; ++n) copy[i].run[n] = copy[i].load[n];
    }
    for (i = 0; i < zero_count; ++i) {
        size_t n;
        if (!zero[i].start) continue;
        for (n = 0; n < zero[i].size; ++n) zero[i].start[n] = 0;
    }
}
