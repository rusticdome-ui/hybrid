/* SPDX-License-Identifier: GPL-3.0-only
 * Standalone SMK-37 HYBRID native startup contract.
 */
#ifndef SMK37_HYBRID_NATIVE_STARTUP_H
#define SMK37_HYBRID_NATIVE_STARTUP_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const uint8_t *load;
    uint8_t *run;
    size_t size;
} hybrid_copy_region_t;

typedef struct {
    uint8_t *start;
    size_t size;
} hybrid_zero_region_t;

/* Host-testable C runtime memory initialization. .noinit is deliberately
 * absent from both tables and therefore survives this initialization. */
void hybrid_native_memory_init(const hybrid_copy_region_t *copy, size_t copy_count,
                               const hybrid_zero_region_t *zero, size_t zero_count);
void hybrid_native_reset_c(void);
void hybrid_native_default_irq(void);
#endif
