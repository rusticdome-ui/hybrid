/* SPDX-License-Identifier: GPL-3.0-only
 * Concrete WL82/AC79 SPI2 matrix scan and ADC adapter.
 * Register layout, IRQ numbers and control bits follow the public SMK-37 Pro
 * board implementation; this adapter remains subject to physical validation.
 */
#include <stdint.h>
#include "hybrid_ac79_scan.h"
#include "hybrid_ac79_scan_hw.h"
#include "hybrid_ac79_wl82_map.h"

#define BIT(n) (1u << (n))
typedef struct { volatile uint32_t OUT,IN,DIR,DIE,PU,PD,HD0,HD,DIEH; } port_t;
typedef struct { volatile uint32_t CON,BAUD,BUF,ADR,CNT; } spi_t;
typedef struct { volatile uint32_t CON,RES; } adc_t;
typedef struct { volatile uint32_t CON0,CON1; } iomap_t;
#define PORTA ((port_t *)HYBRID_WL82_PORTA_BASE)
#define PORTC ((port_t *)HYBRID_WL82_PORTC_BASE)
#define PORTH ((port_t *)HYBRID_WL82_PORTH_BASE)
#define IOMAP ((iomap_t *)HYBRID_WL82_IOMAP_BASE)
#define SPI2 ((spi_t *)HYBRID_WL82_SPI2_BASE)
#define ADC ((adc_t *)HYBRID_WL82_ADC_BASE)
#define SOFT_SET (*(volatile uint32_t *)0x1eef1a0u)
#define SOFT_CLEAR (*(volatile uint32_t *)0x1eef1a4u)

static hybrid_ac79_scan_t scanner;
static hybrid_ac79_scan_hw_config_t hw;
static hybrid_ac79_scan_ops_t ops;
static volatile uint32_t shift_data;
static volatile uint32_t led_mask=0xffffffffu;
static volatile uint32_t mux_mask=0xffffffc7u;
static uint32_t scan_out[HYBRID_SCAN_PHASES];

static uint32_t gpio_sample(void *ctx)
{
    uint32_t h=PORTH->IN,a=PORTA->IN;
    (void)ctx;
    return ((h&0x300u)>>2)|((h&0x40u)>>1)|((a&0x200u)>>5)|
           ((a&0xe0u)>>4)|((a&0x4u)>>2);
}
static void latch_pulse(void *ctx)
{
    (void)ctx;
    PORTA->OUT|=BIT(1);
    PORTA->OUT&=~BIT(1);
}
static void spi2_start(void *ctx,uint32_t word,uint8_t bits)
{
    (void)ctx;
    if(bits!=4u)return;
    shift_data=word;
    SPI2->ADR=(uint32_t)(uintptr_t)&shift_data;
    SPI2->CNT=4u;
}
static void adc_mux(void *ctx,uint8_t channel)
{
    (void)ctx;
    mux_mask=0xffffffc7u|(((uint32_t)channel&7u)<<3);
}
static void adc_start(void *ctx,uint8_t channel)
{
    (void)ctx;
    ADC->CON=5u|(4u<<12)|BIT(3)|BIT(5)|BIT(6)|(((uint32_t)channel&15u)<<8);
    ADC->CON|=BIT(4);
    ADC->CON|=BIT(6);
}
static void software_irq(void *ctx)
{
    (void)ctx;
    SOFT_SET|=BIT(2);
}
static void scan_irq_handler(void)
{
    SPI2->CON|=BIT(14);
    hybrid_ac79_scan_irq(&scanner,&ops,0xffffffffu,led_mask,mux_mask);
}
static void software_irq_handler(void)
{
    SOFT_CLEAR|=BIT(2);
    if(hw.soft_scan)hw.soft_scan(hw.ctx);
}
static void adc_irq_handler(void)
{
    uint32_t con=ADC->CON;
    uint16_t result;
    if(!(con&BIT(7)))return;
    result=(uint16_t)ADC->RES;
    ADC->CON=BIT(6);
    ADC->CON=0u;
    /* Preserve the raw ADC sample before advancing the channel schedule. */
    hybrid_ac79_adc_complete_value(&scanner,&ops,result);
}
static void gpio_input(port_t *p,unsigned bit)
{
    p->DIR|=BIT(bit);p->DIE|=BIT(bit);p->PU|=BIT(bit);p->PD&=~BIT(bit);
}
static void gpio_output(port_t *p,unsigned bit)
{
    p->DIR&=~BIT(bit);p->DIE|=BIT(bit);p->PU&=~BIT(bit);p->PD&=~BIT(bit);
}
static void adc_input(port_t *p,unsigned bit)
{
    p->DIR|=BIT(bit);p->DIE&=~BIT(bit);p->PU&=~BIT(bit);p->PD&=~BIT(bit);
}
int hybrid_ac79_scan_hw_init(const hybrid_ac79_scan_hw_config_t *config)
{
    if(!config||!config->irq_attach)return -1;
    hw=*config;
    /* Match Pro reference pin mux and matrix wiring. */
    IOMAP->CON1&=~BIT(16);IOMAP->CON1|=BIT(17);
    gpio_input(PORTH,6);gpio_input(PORTH,8);gpio_input(PORTH,9);
    gpio_input(PORTA,2);gpio_input(PORTA,5);gpio_input(PORTA,6);
    gpio_input(PORTA,7);gpio_input(PORTA,9);
    gpio_output(PORTA,0);gpio_output(PORTA,1);
    gpio_output(PORTA,3);gpio_output(PORTA,4);
    adc_input(PORTA,8);adc_input(PORTC,9);adc_input(PORTA,10);

    hybrid_ac79_scan_init(&scanner);
    ops.spi2_start=spi2_start;ops.gpio_sample=gpio_sample;
    ops.latch_pulse=latch_pulse;ops.software_irq=software_irq;
    ops.adc_start=adc_start;ops.adc_mux=adc_mux;ops.ctx=0;

    if(hw.irq_attach(hw.ctx,37u,scan_irq_handler,5u)!=0)return -2;
    if(hw.irq_attach(hw.ctx,122u,software_irq_handler,4u)!=0)return -3;
    if(hw.irq_attach(hw.ctx,24u,adc_irq_handler,5u)!=0)return -4;

    ADC->CON=0u;
    SPI2->CON=BIT(5)|BIT(14)|BIT(13);
    SPI2->BAUD=5u;
    shift_data=(~BIT(24))|0xffffu;
    SPI2->ADR=(uint32_t)(uintptr_t)&shift_data;
    SPI2->CNT=4u;
    SPI2->CON|=BIT(0);
    PORTA->OUT&=~BIT(0);
    return 0;
}
void hybrid_ac79_scan_hw_set_led_mask(uint32_t mask){led_mask=mask;}
void hybrid_ac79_scan_hw_get_samples(uint32_t out[16])
{
    if(!out)return;
    if(!hybrid_ac79_scan_take(&scanner,scan_out))return;
    for(unsigned i=0;i<16u;++i)out[i]=scan_out[i];
}

void hybrid_ac79_scan_hw_get_adc(uint16_t out[26],uint32_t *valid_mask)
{ hybrid_ac79_adc_copy(&scanner,out,valid_mask); }
