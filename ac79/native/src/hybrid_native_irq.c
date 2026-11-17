/* SPDX-License-Identifier: GPL-3.0-only
 * HYBRID-owned IRQ dispatch table. This is the software handler registry,
 * not a claim that the AC79 CPU exception/reset vector ABI is solved.
 */
#include <stddef.h>
#include <stdint.h>

#define HYBRID_NATIVE_IRQ_COUNT 128u
typedef void (*hybrid_native_irq_handler_t)(void);
static hybrid_native_irq_handler_t handlers[HYBRID_NATIVE_IRQ_COUNT];

int hybrid_native_irq_register(unsigned irq, hybrid_native_irq_handler_t handler)
{
    if (irq >= HYBRID_NATIVE_IRQ_COUNT || !handler) return -1;
    handlers[irq] = handler;
    return 0;
}

void hybrid_native_irq_unregister(unsigned irq)
{
    if (irq < HYBRID_NATIVE_IRQ_COUNT) handlers[irq] = 0;
}

void hybrid_native_irq_dispatch(unsigned irq)
{
    if (irq >= HYBRID_NATIVE_IRQ_COUNT) return;
    if (handlers[irq]) handlers[irq]();
}
