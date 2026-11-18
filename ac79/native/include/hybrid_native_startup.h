/* SPDX-License-Identifier: GPL-3.0-only
 * Standalone SMK-37 HYBRID native startup contract.
 */
#ifndef SMK37_HYBRID_NATIVE_STARTUP_H
#define SMK37_HYBRID_NATIVE_STARTUP_H
#include <stddef.h>
#include <stdint.h>
typedef struct { const uint8_t *load; uint8_t *run; size_t size; } hybrid_copy_region_t;
typedef struct { uint8_t *start; size_t size; } hybrid_zero_region_t;
extern volatile uint32_t hybrid_native_boot_info[6];
extern volatile uint8_t hybrid_native_flash_header[32];
void hybrid_native_memory_init(const hybrid_copy_region_t *copy, size_t copy_count,
                               const hybrid_zero_region_t *zero, size_t zero_count);
void hybrid_native_reset_c(const uint32_t *boot_info);
int hybrid_native_early_board_init(void);
int hybrid_native_main(void);
#endif
