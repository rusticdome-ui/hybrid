/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
 * Deterministic SMK-37 Pro scan/ADC sequencing core.
 * Hardware register access and IRQ vector binding are supplied by the SDK shim.
 */
#ifndef HYBRID_AC79_SCAN_H
#define HYBRID_AC79_SCAN_H
#include <stdint.h>
#define HYBRID_SCAN_PHASES 16u
#define HYBRID_SCAN_BUFFERS 2u
#define HYBRID_SCAN_FAST_ADC 18u
#define HYBRID_SCAN_SLOW_ADC 8u
typedef struct {
    uint8_t phase, write_buffer, ready_buffer, ready;
    uint8_t adc_delay, fast_index, slow_index;
    uint32_t samples[HYBRID_SCAN_BUFFERS][HYBRID_SCAN_PHASES];
    uint32_t scan_overruns, adc_completions;
    uint32_t adc_sample_valid;
    uint16_t adc_samples[HYBRID_SCAN_FAST_ADC + HYBRID_SCAN_SLOW_ADC];
} hybrid_ac79_scan_t;
typedef struct {
    void (*spi2_start)(void *ctx, uint32_t shift_word, uint8_t bit_count);
    uint32_t (*gpio_sample)(void *ctx);
    void (*latch_pulse)(void *ctx);
    void (*software_irq)(void *ctx);
    void (*adc_start)(void *ctx, uint8_t adc_channel);
    void (*adc_mux)(void *ctx, uint8_t mux_channel);
    void *ctx;
} hybrid_ac79_scan_ops_t;
typedef struct { uint8_t kind, index, adc_channel, mux_channel; } hybrid_ac79_adc_step_t;
enum { HYBRID_ADC_PAD=0, HYBRID_ADC_AUX_FAST=1, HYBRID_ADC_FADER=2, HYBRID_ADC_PEDAL=3,
       HYBRID_ADC_BATTERY=4, HYBRID_ADC_VBAT=5, HYBRID_ADC_REFERENCE=6 };
void hybrid_ac79_scan_init(hybrid_ac79_scan_t *s);
void hybrid_ac79_scan_irq(hybrid_ac79_scan_t *s,const hybrid_ac79_scan_ops_t *ops,
                          uint32_t phase_mask,uint32_t led_mask,uint32_t mux_mask);
int hybrid_ac79_scan_take(hybrid_ac79_scan_t *s,uint32_t out[HYBRID_SCAN_PHASES]);
hybrid_ac79_adc_step_t hybrid_ac79_adc_next(const hybrid_ac79_scan_t *s);
void hybrid_ac79_adc_complete(hybrid_ac79_scan_t *s,const hybrid_ac79_scan_ops_t *ops);
void hybrid_ac79_adc_complete_value(hybrid_ac79_scan_t *s,const hybrid_ac79_scan_ops_t *ops,uint16_t value);
void hybrid_ac79_adc_copy(const hybrid_ac79_scan_t *s,uint16_t out[HYBRID_SCAN_FAST_ADC + HYBRID_SCAN_SLOW_ADC],uint32_t *valid_mask);
#endif
