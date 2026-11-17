/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception */
#include "hybrid_ac79_scan.h"
static const uint8_t fast_mux[HYBRID_SCAN_FAST_ADC]={0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7,4,5};
static const uint8_t fast_adc[HYBRID_SCAN_FAST_ADC]={1,1,1,1,1,1,1,1,8,8,8,8,8,8,8,8,2,2};
void hybrid_ac79_scan_init(hybrid_ac79_scan_t *s)
{
    unsigned b,p;
    if(!s)return;
    for(b=0;b<HYBRID_SCAN_BUFFERS;b++)for(p=0;p<HYBRID_SCAN_PHASES;p++)s->samples[b][p]=0xffffffffu;
    s->phase=0;s->write_buffer=0;s->ready_buffer=1;s->ready=0;
    s->adc_delay=0;s->fast_index=0;s->slow_index=0;
    s->scan_overruns=0;s->adc_completions=0;
}
void hybrid_ac79_scan_irq(hybrid_ac79_scan_t *s,const hybrid_ac79_scan_ops_t *o,
                          uint32_t phase_mask,uint32_t led_mask,uint32_t mux_mask)
{
    uint8_t next;
    uint32_t sample,shift_word;
    if(!s||!o||!o->gpio_sample||!o->spi2_start||!o->latch_pulse)return;
    sample=o->gpio_sample(o->ctx);
    s->samples[s->write_buffer][s->phase]=sample;
    if(s->adc_delay && s->adc_delay<3u) {
        ++s->adc_delay;
        if(s->adc_delay==3u && o->adc_start) {
            hybrid_ac79_adc_step_t step=hybrid_ac79_adc_next(s);
            o->adc_start(o->ctx,step.adc_channel);
        }
    }
    next=(uint8_t)((s->phase+1u)&15u);
    o->latch_pulse(o->ctx);
    shift_word=~(1u<<(next<8u?(next+24u):(next+8u)));
    o->spi2_start(o->ctx,shift_word&phase_mask&led_mask&mux_mask,4u);
    s->phase=next;
    if(next==0u) {
        if(s->ready)s->scan_overruns++;
        s->ready_buffer=s->write_buffer;s->ready=1u;
        s->write_buffer^=1u;
        if(o->software_irq)o->software_irq(o->ctx);
    }
}
int hybrid_ac79_scan_take(hybrid_ac79_scan_t *s,uint32_t out[HYBRID_SCAN_PHASES])
{
    unsigned i;
    if(!s||!out||!s->ready)return 0;
    for(i=0;i<HYBRID_SCAN_PHASES;i++)out[i]=s->samples[s->ready_buffer][i];
    s->ready=0;
    return 1;
}
hybrid_ac79_adc_step_t hybrid_ac79_adc_next(const hybrid_ac79_scan_t *s)
{
    hybrid_ac79_adc_step_t r={HYBRID_ADC_PAD,0,1,0};
    if(!s)return r;
    if(s->fast_index<HYBRID_SCAN_FAST_ADC) {
        r.kind=s->fast_index<16u?HYBRID_ADC_PAD:HYBRID_ADC_AUX_FAST;r.index=s->fast_index;
        r.adc_channel=fast_adc[s->fast_index];r.mux_channel=fast_mux[s->fast_index];
    } else {
        uint8_t n=s->slow_index;
        r.index=n;
        if(n<4u){r.kind=HYBRID_ADC_FADER;r.adc_channel=2;r.mux_channel=(uint8_t)(n+4u);}
        else if(n==4u){r.kind=HYBRID_ADC_PEDAL;r.adc_channel=2;r.mux_channel=6;}
        else if(n==5u){r.kind=HYBRID_ADC_BATTERY;r.adc_channel=2;r.mux_channel=7;}
        else if(n==6u){r.kind=HYBRID_ADC_VBAT;r.adc_channel=15;r.mux_channel=0;}
        else {r.kind=HYBRID_ADC_REFERENCE;r.adc_channel=15;r.mux_channel=0;}
    }
    return r;
}
void hybrid_ac79_adc_complete(hybrid_ac79_scan_t *s,const hybrid_ac79_scan_ops_t *o)
{
    hybrid_ac79_adc_step_t step;
    if(!s)return;
    ++s->adc_completions;
    if(s->fast_index<HYBRID_SCAN_FAST_ADC) {
        ++s->fast_index;
        if(s->fast_index==HYBRID_SCAN_FAST_ADC)s->slow_index=0;
    } else {
        s->slow_index=(uint8_t)((s->slow_index+1u)%HYBRID_SCAN_SLOW_ADC);
        if(s->slow_index==0u)s->fast_index=0;
    }
    step=hybrid_ac79_adc_next(s);
    if(o&&o->adc_mux)o->adc_mux(o->ctx,step.mux_channel);
    s->adc_delay=1u;
}
