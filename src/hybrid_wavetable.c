/* SPDX-License-Identifier: GPL-3.0-only
 * Independent compact dual-oscillator wavetable-style engine for AC79.
 * Integer/Q15 implementation; no external DSP or proprietary SHRUB code.
 */
#include <stdint.h>
#include "hybrid_wavetable.h"
#define Q15 32767
#define VOICES 16u
static const uint32_t note_step[12]={25480119u,26995246u,28600467u,30301139u,32102938u,34011878u,36034330u,38177043u,40447168u,42852281u,45400411u,48100060u};
static int32_t clamp16(int32_t x){return x>32767?32767:(x< -32768?-32768:x);}
static uint32_t step_for(uint8_t n){uint32_t s=note_step[n%12u];int octave=(int)n/12-5;if(octave>0){while(octave--&&s<0x80000000u)s<<=1;}else while(octave++<0)s>>=1;return s;}
static int32_t wave(uint32_t phase,uint8_t shape){uint32_t p=phase>>16;int32_t x=(int32_t)p-32768;switch(shape&3u){case 1:return 32767-(int32_t)((p<32768u?p:65535u-p)*2u);case 2:return (int32_t)(p^0xffffu)-32768;case 3:return p<32768u?32767:-32768;default:{int32_t a=(x<0?-x:x);int32_t y=(x*(32767-a/2))/32767;return x<0?-y:y;}}}
void hybrid_wavetable_defaults(hybrid_wavetable_t*s){uint32_t i;if(!s)return;for(i=0;i<sizeof(*s);i++)((uint8_t*)s)[i]=0;s->attack=180;s->decay=900;s->sustain=24576;s->release=700;s->cutoff=30000;s->resonance=0;s->wave_a=0;s->wave_b=2;s->osc2_mix=8192;s->detune=3;s->unison=1;s->lfo_rate=90;s->lfo_depth=0;}
static int wt_init(void*p){hybrid_wavetable_defaults((hybrid_wavetable_t*)p);return 0;}
static void wt_reset(void*p){hybrid_wavetable_t*s=p;uint32_t i;for(i=0;i<VOICES;i++)s->voice[i].active=0;}
static void wt_on(void*p,uint8_t n,uint8_t vel){hybrid_wavetable_t*s=p;uint32_t i,best=0;for(i=0;i<VOICES;i++){if(!s->voice[i].active){best=i;break;}if(s->voice[i].envelope<s->voice[best].envelope)best=i;}hybrid_wavetable_voice_t*v=&s->voice[best];v->phase_a=v->phase_b=0;v->step_a=step_for(n);v->step_b=(uint32_t)(((uint64_t)v->step_a*(1000u+s->detune))/1000u);v->note=n;v->velocity=vel;v->envelope=0;v->filter_state=0;v->stage=1;v->active=1;}
static void wt_off(void*p,uint8_t n){hybrid_wavetable_t*s=p;uint32_t i;for(i=0;i<VOICES;i++)if(s->voice[i].active&&s->voice[i].note==n)s->voice[i].stage=4;}
static void wt_control(void*p,uint16_t id,int32_t value){hybrid_wavetable_t*s=p;uint16_t v=(uint16_t)(value<0?0:(value>32767?32767:value));switch(id){case 0:s->wave_a=(uint8_t)(v>>13);break;case 1:s->wave_b=(uint8_t)(v>>13);break;case 2:s->osc2_mix=v;break;case 3:s->attack=v;break;case 4:s->decay=v;break;case 5:s->sustain=v;break;case 6:s->release=v;break;case 7:s->cutoff=v;break;case 8:s->resonance=v;break;case 9:s->lfo_rate=v;break;case 10:s->lfo_depth=v;break;case 11:s->detune=(uint8_t)(v>>8);break;case 12:s->unison=(uint8_t)(1u+(v>>14));break;default:break;}}
static void wt_render(void*p,int32_t*out,uint16_t frames){hybrid_wavetable_t*s=p;uint16_t f;uint32_t i;for(f=0;f<frames;f++){int32_t mix=0;for(i=0;i<VOICES;i++){hybrid_wavetable_voice_t*v=&s->voice[i];int32_t a,b,x,amp;if(!v->active)continue;if(v->stage==1){v->envelope+=(int32_t)(s->attack?s->attack:1u);if(v->envelope>=Q15){v->envelope=Q15;v->stage=2;}}else if(v->stage==2){if(v->envelope>(int32_t)s->sustain){v->envelope-=(int32_t)(s->decay?s->decay:1u);if(v->envelope<=(int32_t)s->sustain){v->envelope=s->sustain;v->stage=3;}}}else if(v->stage==4){v->envelope-=(int32_t)(s->release?s->release:1u);if(v->envelope<=0){v->envelope=0;v->active=0;continue;}}v->phase_a+=v->step_a;v->phase_b+=v->step_b;a=wave(v->phase_a,s->wave_a);b=wave(v->phase_b,s->wave_b);x=(a*(32767-(int32_t)s->osc2_mix)+b*(int32_t)s->osc2_mix)/Q15;amp=(v->envelope*(int32_t)v->velocity)/127;x=(x*amp)/Q15;v->filter_state+=(int32_t)(((int64_t)(x-v->filter_state)*s->cutoff)>>15);mix+=v->filter_state;}mix=clamp16(mix/4);out[2u*f]=mix;out[2u*f+1u]=mix;s->lfo_phase+=s->lfo_rate;}}
static void wt_panic(void*p){wt_reset(p);}
const hybrid_engine_ops_t hybrid_wavetable_ops={"HYBRID WAVETABLE",wt_init,wt_reset,wt_on,wt_off,wt_control,wt_render,wt_panic,0,0};
int hybrid_register_wavetable_engine(hybrid_core_t*c,hybrid_wavetable_t*s){if(!c||!s)return -1;hybrid_wavetable_defaults(s);return hybrid_register_engine(c,HYBRID_WAVETABLE,&hybrid_wavetable_ops,s);}
