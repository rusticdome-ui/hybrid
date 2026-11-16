/* SPDX-License-Identifier: GPL-3.0-only
 * Independent compact dual-oscillator wavetable-style engine for AC79.
 * Integer/Q15 implementation; no external DSP or proprietary SHRUB code.
 */
#include <stdint.h>
#include "hybrid_wavetable.h"
#ifdef HYBRID_ENABLE_PROFILE
#include "hybrid_profile.h"
#endif
#define Q15 32767
#define VOICES 16u
static const uint32_t note_step[12]={25480119u,26995246u,28600467u,30301139u,32102938u,34011878u,36034330u,38177043u,40447168u,42852281u,45400411u,48100060u};
static int32_t clamp16(int32_t x){return x>32767?32767:(x< -32768?-32768:x);}
static uint32_t step_for(uint8_t n){uint32_t s=note_step[n%12u];int octave=(int)n/12-5;if(octave>0){while(octave--&&s<0x80000000u)s<<=1;}else while(octave++<0)s>>=1;return s;}
static const int16_t hybrid_wavetable_data[4][64]={
 {0,3212,6393,9512,12539,15446,18204,20787,23170,25329,27245,28898,30273,31356,32137,32609,32767,32609,32137,31356,30273,28898,27245,25329,23170,20787,18204,15446,12539,9512,6393,3212,0,-3212,-6393,-9512,-12539,-15446,-18204,-20787,-23170,-25329,-27245,-28898,-30273,-31356,-32137,-32609,-32767,-32609,-32137,-31356,-30273,-28898,-27245,-25329,-23170,-20787,-18204,-15446,-12539,-9512,-6393,-3212},
 {-32767,-30719,-28671,-26623,-24575,-22527,-20479,-18431,-16383,-14336,-12288,-10240,-8192,-6144,-4096,-2048,0,2048,4096,6144,8192,10240,12288,14336,16384,18431,20479,22527,24575,26623,28671,30719,32767,30719,28671,26623,24575,22527,20479,18431,16384,14336,12288,10240,8192,6144,4096,2048,0,-2048,-4096,-6144,-8192,-10240,-12288,-14336,-16383,-18431,-20479,-22527,-24575,-26623,-28671,-30719},
 {-32767,-31743,-30719,-29695,-28671,-27647,-26623,-25599,-24575,-23551,-22527,-21503,-20479,-19455,-18431,-17407,-16383,-15360,-14336,-13312,-12288,-11264,-10240,-9216,-8192,-7168,-6144,-5120,-4096,-3072,-2048,-1024,0,1024,2048,3072,4096,5120,6144,7168,8192,9216,10240,11264,12288,13312,14336,15360,16384,17407,18431,19455,20479,21503,22527,23551,24575,25599,26623,27647,28671,29695,30719,31743},
 {32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767}
};
static int32_t wave(uint32_t phase,uint8_t shape){uint32_t index=phase>>26,frac=(phase>>10)&0xffffu;int32_t a=hybrid_wavetable_data[shape&3u][index],b=hybrid_wavetable_data[shape&3u][(index+1u)&63u];return a+(int32_t)(((int64_t)(b-a)*frac)>>16);}

void hybrid_wavetable_defaults(hybrid_wavetable_t*s){uint32_t i;if(!s)return;for(i=0;i<sizeof(*s);i++)((uint8_t*)s)[i]=0;s->attack=180;s->decay=900;s->sustain=24576;s->release=700;s->cutoff=30000;s->resonance=0;s->wave_a=0;s->wave_b=2;s->osc2_mix=8192;s->detune=3;s->unison=1;s->lfo_rate=90;s->lfo_depth=0;}
static int wt_init(void*p){hybrid_wavetable_defaults((hybrid_wavetable_t*)p);return 0;}
static void wt_reset(void*p){hybrid_wavetable_t*s=p;uint32_t i;for(i=0;i<VOICES;i++)s->voice[i].active=0;}
static void wt_on(void*p,uint8_t n,uint8_t vel){hybrid_wavetable_t*s=p;uint32_t i,best=0;for(i=0;i<VOICES;i++){if(!s->voice[i].active){best=i;break;}if(s->voice[i].envelope<s->voice[best].envelope)best=i;}hybrid_wavetable_voice_t*v=&s->voice[best];v->phase_a=v->phase_b=0;v->step_a=step_for(n);v->step_b=(uint32_t)(((uint64_t)v->step_a*(1000u+s->detune))/1000u);v->note=n;v->velocity=vel;v->envelope=0;v->filter_state=0;v->stage=1;v->active=1;}
static void wt_off(void*p,uint8_t n){hybrid_wavetable_t*s=p;uint32_t i;for(i=0;i<VOICES;i++)if(s->voice[i].active&&s->voice[i].note==n)s->voice[i].stage=4;}
static void wt_control(void*p,uint16_t id,int32_t value){hybrid_wavetable_t*s=p;uint16_t v=(uint16_t)(value<0?0:(value>32767?32767:value));switch(id){case 0:s->wave_a=(uint8_t)(v>>13);break;case 1:s->wave_b=(uint8_t)(v>>13);break;case 2:s->osc2_mix=v;break;case 3:s->attack=v;break;case 4:s->decay=v;break;case 5:s->sustain=v;break;case 6:s->release=v;break;case 7:s->cutoff=v;break;case 8:s->resonance=v;break;case 9:s->lfo_rate=v;break;case 10:s->lfo_depth=v;break;case 11:s->detune=(uint8_t)(v>>8);break;case 12:s->unison=(uint8_t)(1u+(v>>14));break;default:break;}}
static void wt_render(void*p,int32_t*out,uint16_t frames){hybrid_wavetable_t*s=p;uint16_t f;uint32_t i;
#ifdef HYBRID_ENABLE_PROFILE
hybrid_profile_begin(HYBRID_PROF_WAVETABLE);
#endif
for(f=0;f<frames;f++){int32_t mix=0;for(i=0;i<VOICES;i++){hybrid_wavetable_voice_t*v=&s->voice[i];int32_t a,b,x,amp;if(!v->active)continue;if(v->stage==1){v->envelope+=(int32_t)(s->attack?s->attack:1u);if(v->envelope>=Q15){v->envelope=Q15;v->stage=2;}}else if(v->stage==2){if(v->envelope>(int32_t)s->sustain){v->envelope-=(int32_t)(s->decay?s->decay:1u);if(v->envelope<=(int32_t)s->sustain){v->envelope=s->sustain;v->stage=3;}}}else if(v->stage==4){v->envelope-=(int32_t)(s->release?s->release:1u);if(v->envelope<=0){v->envelope=0;v->active=0;continue;}}v->phase_a+=v->step_a;v->phase_b+=v->step_b;a=wave(v->phase_a,s->wave_a);b=wave(v->phase_b,s->wave_b);x=(a*(32767-(int32_t)s->osc2_mix)+b*(int32_t)s->osc2_mix)/Q15;amp=(v->envelope*(int32_t)v->velocity)/127;x=(x*amp)/Q15;v->filter_state+=(int32_t)(((int64_t)(x-v->filter_state)*s->cutoff)>>15);mix+=v->filter_state;}mix=clamp16(mix/4);out[2u*f]=mix;out[2u*f+1u]=mix; s->lfo_phase+=s->lfo_rate;}
#ifdef HYBRID_ENABLE_PROFILE
hybrid_profile_end(HYBRID_PROF_WAVETABLE,frames);
#endif
}
static void wt_panic(void*p){wt_reset(p);}
const hybrid_engine_ops_t hybrid_wavetable_ops={"HYBRID WAVETABLE",wt_init,wt_reset,wt_on,wt_off,wt_control,wt_render,wt_panic,0,0};
int hybrid_register_wavetable_engine(hybrid_core_t*c,hybrid_wavetable_t*s){if(!c||!s)return -1;hybrid_wavetable_defaults(s);return hybrid_register_engine(c,HYBRID_WAVETABLE,&hybrid_wavetable_ops,s);}
