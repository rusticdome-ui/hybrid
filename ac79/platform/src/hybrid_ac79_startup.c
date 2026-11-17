/* SPDX-License-Identifier: GPL-3.0-only
 * Native Felucca/WL82 startup bridge for the independent SMK-37 scanner.
 * This starts raw acquisition only; it does not replace the existing musical
 * key/pad decoder until calibrated event parity has been demonstrated.
 */
#include <stdint.h>
#include "hybrid_ac79_scan_hw.h"
#include "fm1_gpio.h"
#include "fm1_irq.h"

#define BIT(n) (1u << (n))
extern void smk37_adc_pmu_detect_en(unsigned mode) __attribute__((weak));
static volatile uint8_t pmu_detect_available;
static volatile uint8_t scan_irq_seen;

static int irq_attach(void *ctx,unsigned irq,void (*handler)(void),unsigned priority)
{
    (void)ctx;
    fm1_irq_attach(irq,handler,priority);
    return 0;
}
static int gpio_input(void *ctx,unsigned pin,unsigned pull_up)
{
    unsigned port=pin>>4,bit=pin&15u;
    (void)ctx;
    if(port>7u)return -1;
    FM1_PR(port,FM1_DIE)|=BIT(bit);
    FM1_PR(port,FM1_DIR)|=BIT(bit);
    if(pull_up)FM1_PR(port,FM1_PU)|=BIT(bit);else FM1_PR(port,FM1_PU)&=~BIT(bit);
    FM1_PR(port,FM1_PD)&=~BIT(bit);
    return 0;
}
static int gpio_output(void *ctx,unsigned pin,unsigned value)
{
    unsigned port=pin>>4,bit=pin&15u;
    (void)ctx;
    if(port>7u)return -1;
    FM1_PR(port,FM1_DIE)|=BIT(bit);
    if(value)FM1_PR(port,FM1_OUT)|=BIT(bit);else FM1_PR(port,FM1_OUT)&=~BIT(bit);
    FM1_PR(port,FM1_DIR)&=~BIT(bit);
    return 0;
}
static int adc_input(void *ctx,unsigned pin)
{
    unsigned port=pin>>4,bit=pin&15u;
    (void)ctx;
    if(port>7u)return -1;
    FM1_PR(port,FM1_DIE)&=~BIT(bit);
    FM1_PR(port,FM1_DIR)|=BIT(bit);
    FM1_PR(port,FM1_PU)&=~BIT(bit);
    FM1_PR(port,FM1_PD)&=~BIT(bit);
    return 0;
}
static void pmu_detect(void *ctx,unsigned mode){(void)ctx;if(smk37_adc_pmu_detect_en)smk37_adc_pmu_detect_en(mode);else pmu_detect_available=0u;}
static void soft_scan(void *ctx){(void)ctx;scan_irq_seen=1u;}

int hybrid_ac79_scan_startup(void)
{
    const hybrid_ac79_scan_hw_config_t config={
        irq_attach,soft_scan,gpio_input,gpio_output,adc_input,pmu_detect,0
    };
    scan_irq_seen=0u;pmu_detect_available=(uint8_t)(smk37_adc_pmu_detect_en!=0);
    return hybrid_ac79_scan_hw_init(&config);
}
int hybrid_ac79_scan_pending(void){return scan_irq_seen!=0u;}

int hybrid_ac79_scan_pmu_detect_available(void){return pmu_detect_available!=0u;}
