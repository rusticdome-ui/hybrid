/* SPDX-License-Identifier: GPL-3.0-only
 * WL82 peripheral IRQ vector table. The SDK hwi.h defines ISR_ENTRY and IRQ
 * indices; pointer-table/ICFG programming follows the hardware-tested
 * pi32v2-isa timer ISR example. This is not the mask-ROM reset/exception ABI.
 */
#include <stdint.h>
#include "hybrid_native_irq.h"
#define IRQ_COUNT 128u
#define VECTOR_BASE 0x01c7fe00u
#define ICFG_BASE 0x01eef100u
#define ILAT_CLR (*(volatile uint32_t *)0x01eef1a4u)
static volatile uint32_t *const vectors=(volatile uint32_t *)(uintptr_t)VECTOR_BASE;
static volatile uint32_t *const icfg=(volatile uint32_t *)(uintptr_t)ICFG_BASE;

__attribute__((interrupt))
void hybrid_native_default_irq(void)
{
    __asm__ volatile("cli" ::: "memory");
    for (;;) __asm__ volatile("idle");
}

int hybrid_native_irq_init(void)
{
    unsigned i;
    __asm__ volatile("cli" ::: "memory");
    for(i=0;i<IRQ_COUNT;i++) vectors[i]=(uint32_t)(uintptr_t)hybrid_native_default_irq;
    /* Eight four-bit source configurations per word; clear the 32-word window. */
    for(i=0;i<32u;i++) icfg[i]=0u;
    ILAT_CLR=0xffu;
    __asm__ volatile("csync" ::: "memory");
    return 0;
}

int hybrid_native_irq_attach(unsigned irq, hybrid_native_irq_handler_t handler, unsigned priority)
{
    uint32_t shift,word,config,gates=0x300u;
    if(irq>=IRQ_COUNT||!handler||priority>7u)return -1;
    shift=(irq&7u)*4u;
    word=irq>>3;
    config=((priority<<1)|1u)<<shift;
    vectors[irq]=(uint32_t)(uintptr_t)handler;
    icfg[word]=(icfg[word]&~(0x0fu<<shift))|config;
    __asm__ volatile("csync\n\ticfg = %0\n\tcsync"::"r"(gates):"memory");
    return 0;
}
