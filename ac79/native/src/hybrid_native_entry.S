/* SPDX-License-Identifier: MIT-0
 * SMK-37 Pro application entry for the m-bootloader hand-off ABI.
 * The bootloader calls 0x02000120 with interrupts disabled, its own stacks
 * active, and the application boot-info pointer in r0. This is not the mask
 * ROM reset vector; the bootloader owns the ROM-to-application hand-off.
 */
    .section .entry,"ax"
    .globl _start
_start:
    goto 1f
    rts
1:
    cli
    ssp = __hybrid_sstack_top
    sp = __hybrid_stack_top
    call hybrid_native_reset_c
2:
    goto 2b
