/* SPDX-License-Identifier: GPL-3.0-only
 * Concrete WL82/AC79 SPI2 + ADC interrupt adapter for SMK-37 Pro.
 */
#ifndef HYBRID_AC79_SCAN_HW_H
#define HYBRID_AC79_SCAN_HW_H
#include <stdint.h>
typedef int (*hybrid_ac79_scan_irq_attach_fn)(void *ctx, unsigned irq,
                                               void (*handler)(void),
                                               unsigned priority);
typedef struct {
    hybrid_ac79_scan_irq_attach_fn irq_attach;
    void (*soft_scan)(void *ctx);
    void *ctx;
} hybrid_ac79_scan_hw_config_t;
int hybrid_ac79_scan_hw_init(const hybrid_ac79_scan_hw_config_t *config);
void hybrid_ac79_scan_hw_set_led_mask(uint32_t mask);
void hybrid_ac79_scan_hw_get_samples(uint32_t out[16]);
void hybrid_ac79_scan_hw_get_adc(uint16_t out[26],uint32_t *valid_mask);
#endif
