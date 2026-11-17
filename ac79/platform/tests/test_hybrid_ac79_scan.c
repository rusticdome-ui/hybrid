#include <assert.h>
#include <stdint.h>
#include "hybrid_ac79_scan.h"

static uint32_t sample_value;
static unsigned starts,soft_count,latches,adcs,muxes;
static uint8_t last_adc,last_mux,last_adc_param;
static uint32_t sample(void *c){(void)c;return ++sample_value;}
static void spi(void *c,uint32_t w,uint8_t n){(void)c;(void)w;assert(n==4);++starts;}
static void latch(void *c){(void)c;++latches;}
static void soft_irq(void *c){(void)c;++soft_count;}
static void adc(void *c,uint8_t ch,uint8_t param){(void)c;last_adc=ch;last_adc_param=param;++adcs;}
static void mux(void *c,uint8_t ch){(void)c;assert(ch<8);last_mux=ch;++muxes;}
static void tick(hybrid_ac79_scan_t *s,hybrid_ac79_scan_ops_t *o)
{ hybrid_ac79_scan_irq(s,o,0xffffu,0xffffffffu,0xffffffffu); }

int main(void)
{
    static const uint8_t fast_channels[18]={1,1,1,1,1,1,1,1,8,8,8,8,8,8,8,8,2,2};
    static const uint8_t fast_muxes[18]={0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7,4,5};
    static const uint8_t slow_kinds[8]={HYBRID_ADC_FADER,HYBRID_ADC_FADER,HYBRID_ADC_FADER,HYBRID_ADC_FADER,HYBRID_ADC_PEDAL,HYBRID_ADC_BATTERY,HYBRID_ADC_VBAT,HYBRID_ADC_REFERENCE};
    static const uint8_t slow_channels[8]={2,2,2,2,2,2,15,15};
    static const uint8_t slow_muxes[8]={0,1,2,3,6,7,0,0};
    hybrid_ac79_scan_t s;
    uint32_t out[HYBRID_SCAN_PHASES];
    hybrid_ac79_scan_ops_t o={spi,sample,latch,soft_irq,adc,mux,0};
    unsigned i;

    hybrid_ac79_scan_init(&s);
    for(i=0;i<16;i++) tick(&s,&o);
    assert(soft_count==1&&starts==16&&latches==16&&s.ready);
    assert(hybrid_ac79_scan_take(&s,out)==1&&out[0]==1&&out[15]==16);
    assert(hybrid_ac79_scan_take(&s,out)==0);

    /* Verify both halves are alternated and a missed consumer is counted. */
    for(i=0;i<16;i++) tick(&s,&o);
    assert(hybrid_ac79_scan_take(&s,out)==1&&out[0]==17&&out[15]==32);
    for(i=0;i<32;i++) tick(&s,&o);
    assert(s.scan_overruns==1&&s.ready);
    assert(hybrid_ac79_scan_take(&s,out)==1&&out[0]==49&&out[15]==64);

    /* Boot must select the initial mux and start the first conversion only
       after the three-tick settling interval. */
    hybrid_ac79_scan_init(&s);
    starts=soft_count=latches=adcs=muxes=0;
    tick(&s,&o);
    assert(muxes==1&&last_mux==0&&adcs==0&&s.adc_delay==1);
    tick(&s,&o);
    assert(adcs==0&&s.adc_delay==2);
    tick(&s,&o);
    assert(adcs==1&&last_adc==1&&s.adc_delay==3);

    /* Exercise every fast and slow conversion in order, including mux routing. */
    for(i=0;i<HYBRID_SCAN_FAST_ADC;i++) {
        hybrid_ac79_adc_step_t a=hybrid_ac79_adc_next(&s);
        assert(a.kind==(i<16?HYBRID_ADC_PAD:HYBRID_ADC_AUX_FAST));
        assert(a.index==i&&a.adc_channel==fast_channels[i]&&a.mux_channel==fast_muxes[i]);
        hybrid_ac79_adc_complete_value(&s,&o,(uint16_t)(100u+i));
        if(i+1<HYBRID_SCAN_FAST_ADC) { tick(&s,&o); tick(&s,&o); tick(&s,&o); }
    }
    /* The last fast conversion must also settle the first slow mux. */
    tick(&s,&o); tick(&s,&o); tick(&s,&o);
    for(i=0;i<HYBRID_SCAN_SLOW_ADC;i++) {
        hybrid_ac79_adc_step_t a=hybrid_ac79_adc_next(&s);
        assert(a.kind==slow_kinds[i]&&a.index==i);
        assert(a.adc_channel==slow_channels[i]&&a.mux_channel==slow_muxes[i]);
        assert(a.adc_param==(i==6u?5u:0u));
        hybrid_ac79_adc_complete_value(&s,&o,(uint16_t)(118u+i));
        if(i+1<HYBRID_SCAN_SLOW_ADC) { tick(&s,&o); tick(&s,&o); tick(&s,&o); }
    }
    assert(s.adc_completions==26);
    assert(s.fast_index==0&&s.slow_index==0);
    assert(adcs==26&&muxes==27);
    { uint16_t values[26]; uint32_t valid=0; hybrid_ac79_adc_copy(&s,values,&valid);
      assert(valid==0x03ffffffu); for(i=0;i<26;i++)assert(values[i]==(uint16_t)(100u+i)); }
    return 0;
}
