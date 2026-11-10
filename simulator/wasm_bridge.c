/* SPDX-License-Identifier: GPL-3.0-only
 * Browser/WASM adapter. Test generators remain explicitly labelled until native
 * engine WASM modules are connected; real engines may submit interleaved Q15 PCM.
 */
#include <stdint.h>
#include "hybrid_core.h"
#if defined(__wasm__)
uint64_t __udivdi3(uint64_t numerator,uint64_t denominator){uint64_t q=0,r=0;int bit;if(!denominator)return 0;for(bit=63;bit>=0;--bit){r=(r<<1)|((numerator>>(uint32_t)bit)&1u);if(r>=denominator){r-=denominator;q|=((uint64_t)1u<<(uint32_t)bit);}}return q;}
int64_t __divdi3(int64_t n,int64_t d){uint8_t neg=(uint8_t)((n<0)^(d<0));uint64_t a=n<0?(uint64_t)(-(n+1))+1u:(uint64_t)n,b=d<0?(uint64_t)(-(d+1))+1u:(uint64_t)d,q=__udivdi3(a,b);return neg?-(int64_t)q:(int64_t)q;}
#endif
typedef struct {uint32_t phase,step;int32_t level;uint32_t on,off;uint8_t note,velocity,active;} wasm_voice_t;
static hybrid_core_t core;
static wasm_voice_t voices[HYBRID_CHANNELS];
static int32_t output[HYBRID_MAX_FRAMES*2u];
static int32_t pcm_input[HYBRID_CHANNELS][HYBRID_MAX_FRAMES*2u];
static uint8_t ready;
static int test_init(void*p){wasm_voice_t*v=(wasm_voice_t*)p;v->phase=0;return 0;}
static void test_reset(void*p){wasm_voice_t*v=(wasm_voice_t*)p;v->active=0;v->level=0;}
static void test_on(void*p,uint8_t n,uint8_t vel){wasm_voice_t*v=(wasm_voice_t*)p;v->note=n;v->velocity=vel;v->step=16000u+(uint32_t)n*271u;v->level=(int32_t)vel*220;v->active=1;v->on++;}
static void test_off(void*p,uint8_t n){wasm_voice_t*v=(wasm_voice_t*)p;if(v->active&&v->note==n)v->active=0;v->off++;}
static void test_control(void*p,uint16_t id,int32_t value){wasm_voice_t*v=(wasm_voice_t*)p;if(id==0)v->level=value;}
static void test_render(void*p,int32_t*dst,uint16_t frames){wasm_voice_t*v=(wasm_voice_t*)p;uint16_t i;for(i=0;i<frames;i++){int32_t x=0;if(v->active){v->phase+=v->step;x=((v->phase&0x8000)?1:-1)*v->level;}dst[2u*i]=x;dst[2u*i+1u]=x;}}
static void test_panic(void*p){test_reset(p);}
static const hybrid_engine_ops_t test_ops={"TEST ONLY",test_init,test_reset,test_on,test_off,test_control,test_render,test_panic,0,0};
__attribute__((visibility("default"))) int hybrid_wasm_init(void){uint32_t i;hybrid_init_defaults(&core);for(i=0;i<HYBRID_CHANNELS;i++){voices[i].phase=0;voices[i].step=18000u+i*3000u;if(hybrid_register_engine(&core,(hybrid_channel_id_t)i,&test_ops,&voices[i]))return -1;}ready=(uint8_t)(hybrid_start(&core)==0);return ready?0:-1;}
__attribute__((visibility("default"))) int hybrid_wasm_set_mix(uint32_t id,uint32_t volume,int32_t pan,uint32_t mute,uint32_t solo){if(!ready||id>=4)return -1;hybrid_set_volume(&core,(hybrid_channel_id_t)id,(uint16_t)(volume>32767?32767:volume));hybrid_set_pan(&core,(hybrid_channel_id_t)id,(int16_t)pan);hybrid_set_mute(&core,(hybrid_channel_id_t)id,(uint8_t)mute);hybrid_set_solo(&core,(hybrid_channel_id_t)id,(uint8_t)solo);return 0;}
__attribute__((visibility("default"))) int hybrid_wasm_set_route(uint32_t id,uint32_t mode,uint32_t low,uint32_t high){if(!ready||id>=4||mode>HYBRID_ROUTE_ZONE)return -1;hybrid_set_route(&core,(hybrid_channel_id_t)id,(hybrid_route_mode_t)mode,(uint8_t)low,(uint8_t)high);return 0;}
__attribute__((visibility("default"))) int hybrid_wasm_set_midi(uint32_t id,uint32_t channel,uint32_t vlo,uint32_t vhi,int32_t transpose){hybrid_channel_config_t*c;if(!ready||id>=4)return -1;c=&core.engine[id].config;c->midi_channel=(uint8_t)channel;c->velocity_low=(uint8_t)vlo;c->velocity_high=(uint8_t)vhi;c->transpose=(int8_t)transpose;return 0;}
__attribute__((visibility("default"))) void hybrid_wasm_note_on(uint32_t n,uint32_t v,uint32_t ch){if(ready)hybrid_note_on(&core,(uint8_t)n,(uint8_t)v,(uint8_t)ch);}
__attribute__((visibility("default"))) void hybrid_wasm_note_off(uint32_t n,uint32_t ch){if(ready)hybrid_note_off(&core,(uint8_t)n,(uint8_t)ch);}
__attribute__((visibility("default"))) uint32_t hybrid_wasm_pcm_ptr(uint32_t id){return ready&&id<4?(uint32_t)(uintptr_t)pcm_input[id]:0;}
__attribute__((visibility("default"))) int hybrid_wasm_submit_pcm(uint32_t id,uint32_t frames){if(!ready||id>=4)return -1;return hybrid_submit_pcm(&core,(hybrid_channel_id_t)id,pcm_input[id],(uint16_t)frames);}
__attribute__((visibility("default"))) void hybrid_wasm_discard_pcm(uint32_t id){if(ready&&id<4)hybrid_discard_pcm(&core,(hybrid_channel_id_t)id);}
__attribute__((visibility("default"))) int hybrid_wasm_process(uint32_t frames){if(!ready||!frames||frames>HYBRID_MAX_FRAMES)return -1;return hybrid_process(&core,output,(uint16_t)frames);}
__attribute__((visibility("default"))) uint32_t hybrid_wasm_output_ptr(void){return (uint32_t)(uintptr_t)output;}
__attribute__((visibility("default"))) uint32_t hybrid_wasm_voice_on(uint32_t id){return id<4?voices[id].on:0;}
__attribute__((visibility("default"))) uint32_t hybrid_wasm_voice_off(uint32_t id){return id<4?voices[id].off:0;}
__attribute__((visibility("default"))) int hybrid_wasm_scene_capture(uint32_t n){return ready?hybrid_scene_capture(&core,(uint8_t)n):-1;}
__attribute__((visibility("default"))) int hybrid_wasm_scene_recall(uint32_t n,uint32_t ms){return ready?hybrid_scene_recall(&core,(uint8_t)n,(uint16_t)ms):-1;}
__attribute__((visibility("default"))) void hybrid_wasm_panic(void){if(ready)hybrid_panic(&core);}
__attribute__((visibility("default"))) uint32_t hybrid_wasm_core_size(void){return (uint32_t)sizeof(core);}
