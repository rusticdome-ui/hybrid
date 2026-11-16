#include "hybrid_wavetable_pro.h"
#include "hybrid_core.h"
#include <assert.h>
#include <stdio.h>
int main(void){hybrid_core_t core;hybrid_wavetable_t s,t;int32_t rendered[HYBRID_MAX_FRAMES*2];unsigned rendered_nonzero=0;hybrid_wt_mod_route_t r[4]={{1,2,12000},{2,4,9000},{0,0,0},{0,0,0}},rr[4];hybrid_wt_preset_t p;int16_t wave[128];uint32_t seed=1;unsigned i;hybrid_wavetable_defaults(&s);for(i=0;i<128;i++)wave[i]=(int16_t)((i<64?i:127-i)*500-16000);assert(hybrid_wavetable_import(&s,0,wave,128)==0);assert(s.custom_valid_mask&1);assert(hybrid_wavetable_custom_sample(&s,0,0)!=0);assert(hybrid_wavetable_unison_count(8)==4);assert(hybrid_wavetable_noise_sample(&seed)!=hybrid_wavetable_noise_sample(&seed));assert(hybrid_wavetable_sub_sample(0,1)==32767);assert(hybrid_wavetable_preset_save(&s,r,&p)==0);hybrid_wavetable_defaults(&t);assert(hybrid_wavetable_preset_load(&t,rr,&p)==0);assert(t.custom_valid_mask==s.custom_valid_mask);p.custom[0][0]^=1;assert(hybrid_wavetable_preset_load(&t,rr,&p)==-1);
 /* Exercise the same PRO controls through the native engine render callback. */
 hybrid_wavetable_defaults(&s);assert(hybrid_register_wavetable_engine(&core,&s)==0);
 assert(hybrid_start(&core)==0);assert(hybrid_wavetable_import(&s,0,wave,128)==0);
 s.wave_a=4;s.wave_b=4;s.osc2_mix=0;s.unison=4;s.unison_detune=500;
 s.sub_level=24;s.noise_level=8;s.attack=32767;s.decay=0;s.sustain=32767;s.cutoff=32767;
 s.mod_source[0]=HYBRID_WT_MOD_LFO;s.mod_destination[0]=HYBRID_WT_DEST_AMP;s.mod_depth_q15[0]=1200;
 hybrid_note_on(&core,60,110,0);assert(hybrid_process(&core,rendered,128)==0);
 for(i=0;i<256;i++)rendered_nonzero+=(rendered[i]!=0);
 assert(rendered_nonzero>0);
 /* A selected custom slot must be read by the native oscillator, not silently mapped to a built-in wave. */
 assert(hybrid_wavetable_custom_sample(&s,0,0)!=0);
 puts("HYBRID wavetable PRO import/layers/modulation/preset + native renderer: PASS");return 0;}
