#include "hybrid_wavetable.h"
#include <assert.h>
#include <stdio.h>
int main(void){hybrid_core_t c;hybrid_wavetable_t wt;int32_t out[HYBRID_MAX_FRAMES*2];unsigned i,nonzero=0;hybrid_init_defaults(&c);assert(hybrid_register_wavetable_engine(&c,&wt)==0);assert(hybrid_start(&c)==0);hybrid_note_on(&c,60,110,0);assert(hybrid_process(&c,out,128)==0);for(i=0;i<256;i++){assert(out[i]>=-32768&&out[i]<=32767);nonzero+=(out[i]!=0);}assert(nonzero>0);hybrid_note_off(&c,60,0);for(i=0;i<32;i++)assert(hybrid_process(&c,out,128)==0);hybrid_control(&c,0,24576,HYBRID_WAVETABLE);hybrid_panic(&c);puts("HYBRID dual-oscillator wavetable engine: PASS");return 0;}
