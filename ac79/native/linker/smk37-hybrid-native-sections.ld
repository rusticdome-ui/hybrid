/* SPDX-License-Identifier: GPL-3.0-only
 * HYBRID-owned section layout fragment.
 *
 * The verified AC79 memory map must define FLASH, RAM and POOL MEMORY regions
 * before this fragment is included. Do not copy addresses from an upstream
 * board script: obtain and review the exact SMK-37 Pro boot/SDK map first.
 */
SECTIONS
{
    .hybrid_reset : ALIGN(4)
    {
        KEEP(*(.hybrid_reset))
    } > FLASH

    .hybrid_text : ALIGN(4)
    {
        *(.hybrid_text .hybrid_text.*)
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } > FLASH

    .hybrid_data : ALIGN(4)
    {
        __hybrid_data_start = .;
        *(.data .data.*)
        . = ALIGN(4);
        __hybrid_data_end = .;
    } > RAM AT > FLASH
    __hybrid_data_load = LOADADDR(.hybrid_data);

    .hybrid_bss (NOLOAD) : ALIGN(4)
    {
        __hybrid_bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        __hybrid_bss_end = .;
    } > RAM

    .hybrid_noinit (NOLOAD) : ALIGN(4)
    {
        __hybrid_noinit_start = .;
        *(.noinit .noinit.*)
        . = ALIGN(4);
        __hybrid_noinit_end = .;
    } > RAM

    .hybrid_pool (NOLOAD) : ALIGN(16)
    {
        __hybrid_pool_start = .;
        *(.pool .pool.*)
        . = ALIGN(16);
        __hybrid_pool_end = .;
    } > POOL

    .hybrid_copy_table : ALIGN(4)
    {
        __hybrid_copy_table_start = .;
        LONG(LOADADDR(.hybrid_data)); LONG(ADDR(.hybrid_data)); LONG(SIZEOF(.hybrid_data));
        __hybrid_copy_table_end = .;
    } > FLASH

    .hybrid_zero_table : ALIGN(4)
    {
        __hybrid_zero_table_start = .;
        LONG(ADDR(.hybrid_bss)); LONG(SIZEOF(.hybrid_bss));
        __hybrid_zero_table_end = .;
    } > FLASH

    ASSERT(SIZEOF(.hybrid_reset) <= 0x100, "HYBRID reset section unexpectedly large")
}
