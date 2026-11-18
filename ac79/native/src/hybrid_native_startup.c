/* SPDX-License-Identifier: GPL-3.0-only
 * C runtime entry after the m-bootloader application hand-off.
 */
#include "hybrid_native_startup.h"
extern const hybrid_copy_region_t __hybrid_copy_table_start[];
extern const hybrid_copy_region_t __hybrid_copy_table_end[];
extern const hybrid_zero_region_t __hybrid_zero_table_start[];
extern const hybrid_zero_region_t __hybrid_zero_table_end[];
volatile uint32_t hybrid_native_boot_info[6];
volatile uint8_t hybrid_native_flash_header[32];
#if defined(__GNUC__)
__attribute__((weak))
#endif
int hybrid_native_early_board_init(void) { return 0; }
#if defined(__GNUC__)
__attribute__((weak))
#endif
int hybrid_native_main(void) { for (;;) { __asm__ volatile("idle"); } }
void hybrid_native_reset_c(const uint32_t *boot_info)
{
    size_t copy_count=(size_t)(__hybrid_copy_table_end-__hybrid_copy_table_start);
    size_t zero_count=(size_t)(__hybrid_zero_table_end-__hybrid_zero_table_start);
    unsigned i;
    hybrid_native_memory_init(__hybrid_copy_table_start,copy_count,
                              __hybrid_zero_table_start,zero_count);
    /* Preserve boot hand-off data before IRQ vectors reuse 0x01c7fe00. */
    if(boot_info) {
        for(i=0;i<6u;i++) hybrid_native_boot_info[i]=boot_info[i];
        if(boot_info[0]==0x01c7fe20u) {
            const volatile uint8_t *src=(const volatile uint8_t *)(uintptr_t)boot_info[0];
            for(i=0;i<32u;i++) hybrid_native_flash_header[i]=src[i];
        }
    }
    if(hybrid_native_early_board_init()!=0) for(;;) { __asm__ volatile("idle"); }
    (void)hybrid_native_main();
    for(;;) { __asm__ volatile("idle"); }
}
