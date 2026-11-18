/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef SMK37_HYBRID_NATIVE_IRQ_H
#define SMK37_HYBRID_NATIVE_IRQ_H
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*hybrid_native_irq_handler_t)(void);
int hybrid_native_irq_init(void);
int hybrid_native_irq_attach(unsigned irq, hybrid_native_irq_handler_t handler, unsigned priority);
void hybrid_native_default_irq(void);
#ifdef __cplusplus
}
#endif
#endif
