/* SPDX-License-Identifier: GPL-3.0-only
 * Complete XIP application linker script for SMK-37 Pro / m-bootloader.
 * SMK-37 firmware extent [0x4000,0x9d000), mapped at 0x02000000.
 * Entry 0x02000120 leaves 0x98ee0 bytes for the app image.
 * Conservative app RAM [0x01c40000,0x01c7c000), excluding bootloader,
 * download plug-in and retained boot/update areas.
 */
ENTRY(_start)
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x02000120, LENGTH = 0x98ee0
    RAM   (rwx) : ORIGIN = 0x01c40000, LENGTH = 0x0003c000
}
SECTIONS
{
    .entry ORIGIN(FLASH) : ALIGN(4) { KEEP(*(.entry)) } > FLASH
    .text : ALIGN(4)
    {
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.hybrid_text .hybrid_text.*)
        . = ALIGN(4);
    } > FLASH
    .hybrid_data : ALIGN(4)
    {
        __hybrid_data_start = .;
        *(.data .data.*)
        *(.sdata .sdata.*)
        . = ALIGN(4);
        __hybrid_data_end = .;
    } > RAM AT > FLASH
    __hybrid_data_load = LOADADDR(.hybrid_data);
    .hybrid_dma (NOLOAD) : ALIGN(32)
    {
        __hybrid_dma_start = .;
        *(.hybrid_dma .hybrid_dma.*)
        . = ALIGN(32);
        __hybrid_dma_end = .;
    } > RAM
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
    } > RAM
    .hybrid_copy_table : ALIGN(4)
    {
        __hybrid_copy_table_start = .;
        LONG(LOADADDR(.hybrid_data)); LONG(ADDR(.hybrid_data)); LONG(SIZEOF(.hybrid_data));
        __hybrid_copy_table_end = .;
    } > FLASH
    .hybrid_zero_table : ALIGN(4)
    {
        __hybrid_zero_table_start = .;
        LONG(ADDR(.hybrid_dma)); LONG(SIZEOF(.hybrid_dma));
        LONG(ADDR(.hybrid_bss)); LONG(SIZEOF(.hybrid_bss));
        LONG(ADDR(.hybrid_pool)); LONG(SIZEOF(.hybrid_pool));
        __hybrid_zero_table_end = .;
    } > FLASH
    __hybrid_stack_top = ORIGIN(RAM) + LENGTH(RAM);
    __hybrid_sstack_top = __hybrid_stack_top - 0x2000;
    __hybrid_stack_bottom = __hybrid_stack_top - 0x2000;
    __hybrid_sstack_bottom = __hybrid_stack_top - 0x4000;
    _ustack_lo = __hybrid_stack_bottom;
    _ustack_top = __hybrid_stack_top;
    _sstack_lo = __hybrid_sstack_bottom;
    _sstack_top = __hybrid_sstack_top;
    ASSERT(ADDR(.entry) == 0x02000120, "SMK-37 app entry must be 0x02000120")
    ASSERT(SIZEOF(.entry) > 0, "missing native entry stub")
    ASSERT(__hybrid_pool_end <= __hybrid_sstack_bottom, "HYBRID RAM overlaps system/user stacks")
}
