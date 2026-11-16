/* SPDX-License-Identifier: GPL-3.0-only
 * Independent compact dual-oscillator wavetable-style engine for AC79.
 * Integer/Q15 implementation; no external DSP or proprietary SHRUB code.
 */
#include <stdint.h>
#include "hybrid_wavetable.h"
#include "hybrid_wavetable_pro.h"
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
static int32_t wave(const hybrid_wavetable_t *s,uint32_t phase,uint8_t shape){
 uint32_t index=phase>>26,frac=(phase>>10)&0xffffu;int32_t a,b;
 if(shape>=4u && shape<8u && (s->custom_valid_mask&(1u<<(shape-4u))))
  return hybrid_wavetable_custom_sample(s,(uint8_t)(shape-4u),phase);
 a=hybrid_wavetable_data[shape&3u][index];b=hybrid_wavetable_data[shape&3u][(index+1u)&63u];
 return a+(int32_t)(((int64_t)(b-a)*frac)>>16);
}
static int32_t wt_clamp_q15(int32_t x){return x>32767?32767:(x< -32768?-32768:x);}
static int32_t wt_clamp_u15(int32_t x){return x>32767?32767:(x<0?0:x);}
static void wt_routes(const hybrid_wavetable_t*s,hybrid_wt_mod_route_t r[4]){
 uint32_t i;for(i=0;i<4u;i++){r[i].source=s->mod_source[i];r[i].destination=s->mod_destination[i];r[i].depth_q15=s->mod_depth_q15[i];}
}

void hybrid_wavetable_defaults(hybrid_wavetable_t*s){uint32_t i;if(!s)return;for(i=0;i<sizeof(*s);i++)((uint8_t*)s)[i]=0;s->attack=180;s->decay=900;s->sustain=24576;s->release=700;s->cutoff=30000;s->resonance=0;s->wave_a=0;s->wave_b=2;s->osc2_mix=8192;s->detune=3;s->unison=1;s->lfo_rate=90;s->lfo_depth=0;s->unison_detune=250;s->noise_state=0x6d2b79f5u;}
static int wt_init(void*p){hybrid_wavetable_defaults((hybrid_wavetable_t*)p);return 0;}
static void wt_reset(void*p){hybrid_wavetable_t*s=p;uint32_t i;for(i=0;i<VOICES;i++)s->voice[i].active=0;}
static void wt_on(void*p,uint8_t n,uint8_t vel){hybrid_wavetable_t*s=p;uint32_t i,best=0;for(i=0;i<VOICES;i++){if(!s->voice[i].active){best=i;break;}if(s->voice[i].envelope<s->voice[best].envelope)best=i;}hybrid_wavetable_voice_t*v=&s->voice[best];v->phase_a=v->phase_b=0;v->step_a=step_for(n);v->step_b=(uint32_t)(((uint64_t)v->step_a*(1000u+s->detune))/1000u);v->note=n;v->velocity=vel;v->envelope=0;v->filter_state=0;v->stage=1;v->active=1;}
static void wt_off(void*p,uint8_t n){hybrid_wavetable_t*s=p;uint32_t i;for(i=0;i<VOICES;i++)if(s->voice[i].active&&s->voice[i].note==n)s->voice[i].stage=4;}
static void wt_control(void*p,uint16_t id,int32_t value){hybrid_wavetable_t*s=p;uint16_t v=(uint16_t)(value<0?0:(value>32767?32767:value));switch(id){case 0:s->wave_a=(uint8_t)(v>>12);break;case 1:s->wave_b=(uint8_t)(v>>12);break;case 2:s->osc2_mix=v;break;case 3:s->attack=v;break;case 4:s->decay=v;break;case 5:s->sustain=v;break;case 6:s->release=v;break;case 7:s->cutoff=v;break;case 8:s->resonance=v;break;case 9:s->lfo_rate=v;break;case 10:s->lfo_depth=v;break;case 11:s->detune=(uint8_t)(v>>8);break;case 12:s->unison=hybrid_wavetable_unison_count((uint8_t)(1u+(v>>13)));break;
 case 13:s->unison_detune=(uint16_t)(v>5000u?5000u:v);break;case 14:s->sub_level=(uint8_t)(v>>8);break;case 15:s->noise_level=(uint8_t)(v>>8);break;
 default:if(id>=32u&&id<44u){uint8_t slot=(uint8_t)((id-32u)/3u),part=(uint8_t)((id-32u)%3u);if(part==0)s->mod_source[slot]=(uint8_t)(v>>12);else if(part==1)s->mod_destination[slot]=(uint8_t)(v>>12);else s->mod_depth_q15[slot]=(int16_t)(value>32767?32767:(value< -32768?-32768:value));}break;}}
static void wt_render(void*p,int32_t*out,uint16_t frames){
 hybrid_wavetable_t*s=p;uint16_t f;uint32_t i;hybrid_wt_mod_route_t routes[HYBRID_WT_MOD_SLOTS];
 wt_routes(s,routes);
#ifdef HYBRID_ENABLE_PROFILE
 hybrid_profile_begin(HYBRID_PROF_WAVETABLE);
#endif
 for(f=0;f<frames;f++){
  int32_t mix=0;
  int16_t lfo=(s->lfo_phase&0x80000000u)?32767:-32767;
  for(i=0;i<VOICES;i++){
   hybrid_wavetable_voice_t*v=&s->voice[i];hybrid_wt_mod_values_t mod;
   int32_t a=0,b=0,x,amp,cutoff,osc_mix,pitch;uint8_t u,count;
   if(!v->active)continue;
   if(v->stage==1){v->envelope+=(int32_t)(s->attack?s->attack:1u);if(v->envelope>=Q15){v->envelope=Q15;v->stage=2;}}
   else if(v->stage==2){if(v->envelope>(int32_t)s->sustain){v->envelope-=(int32_t)(s->decay?s->decay:1u);if(v->envelope<=(int32_t)s->sustain){v->envelope=s->sustain;v->stage=3;}}}
   else if(v->stage==4){v->envelope-=(int32_t)(s->release?s->release:1u);if(v->envelope<=0){v->envelope=0;v->active=0;continue;}}
   hybrid_wavetable_mod_evaluate(routes,lfo,v->velocity,v->note,(int16_t)wt_clamp_q15(v->envelope),&mod);
   pitch=wt_clamp_q15(mod.pitch_q15);
   osc_mix=wt_clamp_u15((int32_t)s->osc2_mix+mod.osc_mix_q15);
   cutoff=wt_clamp_u15((int32_t)s->cutoff+mod.cutoff_q15);
   count=hybrid_wavetable_unison_count(s->unison);
   {
    uint32_t sa=(uint32_t)(((uint64_t)v->step_a*(uint32_t)(32767+pitch))/32767u);
    uint32_t sb=(uint32_t)(((uint64_t)v->step_b*(uint32_t)(32767+pitch))/32767u);
    int32_t da=(int32_t)(((uint64_t)sa*s->unison_detune)/100000u);
    int32_t db=(int32_t)(((uint64_t)sb*s->unison_detune)/100000u);
    uint32_t offset_step=(count==1u)?0u:((count==2u)?0x80000000u:((count==3u)?0x55555555u:0x40000000u));
    for(u=0;u<count;u++){
     int32_t centered=(int32_t)(2u*u)-(int32_t)(count-1u);
     uint32_t offset=(uint32_t)u*offset_step;
     uint32_t ua=(uint32_t)((int64_t)sa+(int64_t)da*centered);
     uint32_t ub=(uint32_t)((int64_t)sb+(int64_t)db*centered);
     a+=wave(s,v->phase_a+offset,s->wave_a);
     b+=wave(s,v->phase_b+offset,s->wave_b);
     if(u==0u){v->phase_a+=ua;v->phase_b+=ub;}
    }
   }
   a/=count;b/=count;
   x=(a*(32767-osc_mix)+b*osc_mix)/Q15;
   if(s->sub_level)x+=(hybrid_wavetable_sub_sample(v->phase_a,1u)*(int32_t)s->sub_level)/255;
   if(s->noise_level)x+=(hybrid_wavetable_noise_sample(&s->noise_state)*(int32_t)s->noise_level)/255;
   amp=(v->envelope*(int32_t)v->velocity)/127;
   amp=(amp*(32767+wt_clamp_q15(mod.amp_q15)))/Q15;
   x=(x*amp)/Q15;
   v->filter_state+=(int32_t)(((int64_t)(x-v->filter_state)*cutoff)>>15);
   mix+=v->filter_state;
  }
  mix=clamp16(mix/4);out[2u*f]=mix;out[2u*f+1u]=mix;s->lfo_phase+=s->lfo_rate;
 }
#ifdef HYBRID_ENABLE_PROFILE
 hybrid_profile_end(HYBRID_PROF_WAVETABLE,frames);
#endif
}
static void wt_panic(void*p){wt_reset(p);}
const hybrid_engine_ops_t hybrid_wavetable_ops={"HYBRID WAVETABLE",wt_init,wt_reset,wt_on,wt_off,wt_control,wt_render,wt_panic,0,0};
int hybrid_register_wavetable_engine(hybrid_core_t*c,hybrid_wavetable_t*s){if(!c||!s)return -1;hybrid_wavetable_defaults(s);return hybrid_register_engine(c,HYBRID_WAVETABLE,&hybrid_wavetable_ops,s);}
