/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include "hybrid_native_startup.h"

int main(void)
{
    const uint8_t load[] = {1, 2, 3, 4};
    uint8_t data[] = {0, 0, 0, 0};
    uint8_t bss[] = {9, 9, 9};
    uint8_t noinit[] = {7, 8, 9};
    const hybrid_copy_region_t copy[] = {{load, data, sizeof data}};
    const hybrid_zero_region_t zero[] = {{bss, sizeof bss}};
    hybrid_native_memory_init(copy, 1, zero, 1);
    assert(data[0] == 1 && data[1] == 2 && data[2] == 3 && data[3] == 4);
    assert(bss[0] == 0 && bss[1] == 0 && bss[2] == 0);
    assert(noinit[0] == 7 && noinit[1] == 8 && noinit[2] == 9);
    return 0;
}
