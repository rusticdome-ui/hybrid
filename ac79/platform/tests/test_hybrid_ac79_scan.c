#include <assert.h>
#include <stdint.h>
#include "hybrid_ac79_scan.h"
static uint32_t sample_value;
static unsigned starts,soft,latches,adcs,muxes;
static uint32_t sample(void){return ++sample_value;}
static void spi(void *c,uint32_t w,uint8_t n){(void)c;(void)w;assert(n==4);++starts;}
static void latch(void *c){(void)c;++latches;}
static void soft(void *c){(void)c;++soft;}
static void adc(void *c,uint8_t ch){(void)c;assert(ch==1||ch==8||ch==2||ch==15);++adcs;}
static void mux(void *c,uint8_t ch){(void)c;assert(ch<8);++muxes;}
int main(void){
 hybrid_ac79_scan_t s;uint32_t out[16];hybrid_ac79_scan_ops_t o={spi,sample,latch,soft,adc,mux,0};
 hybrid_ac79_scan_init(&s);
 for(unsigned i=0;i<16;i++)hybrid_ac79_scan_irq(&s,&o,0xffff,0xffffffff,0xffffffff);
 assert(soft==1&&starts==16&&latches==16&&s.ready);
 assert(hybrid_ac79_scan_take(&s,out)==1&&out[0]==1&&out[15]==16);
 assert(hybrid_ac79_scan_take(&s,out)==0);
 for(unsigned i=0;i<18;i++){hybrid_ac79_adc_step_t a=hybrid_ac79_adc_next(&s);assert(a.kind==HYBRID_ADC_PAD);hybrid_ac79_adc_complete(&s,&o);}
 assert(hybrid_ac79_adc_next(&s).kind==HYBRID_ADC_FADER);
 for(unsigned i=0;i<8;i++)hybrid_ac79_adc_complete(&s,&o);
 assert(hybrid_ac79_adc_next(&s).kind==HYBRID_ADC_PAD);
 assert(s.adc_completions==26&&muxes==26);
 /* Three scan IRQ ticks elapse between mux select and conversion start. */
 s.fast_index=0;s.slow_index=0;s.adc_delay=1;
 for(unsigned i=0;i<2;i++)hybrid_ac79_scan_irq(&s,&o,0xffff,0xffffffff,0xffffffff);
 assert(adcs==1);
 return 0;
}
