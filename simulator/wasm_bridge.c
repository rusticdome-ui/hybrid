/* SPDX-License-Identifier: GPL-3.0-only
 * Browser/WASM bridge: native HYBRID mixer + independent wavetable engine.
 * FM6/DX7 and sample PCM arrive through separate one-shot stereo PCM slots.
 */
#include <stdint.h>
#include "hybrid_core.h"
#include "hybrid_wavetable.h"
#if defined(__wasm__)
uint64_t __udivdi3(uint64_t n,uint64_t d){uint64_t q=0,r=0;int bit;if(!d)return 0;for(bit=63;bit>=0;--bit){r=(r<<1)|((n>>(uint32_t)bit)&1u);if(r>=d){r-=d;q|=((uint64_t)1u<<(uint32_t)bit);}}return q;}
int64_t __divdi3(int64_t n,int64_t d){uint8_t neg=(uint8_t)((n<0)^(d<0));uint64_t a=n<0?(uint64_t)(-(n+1))+1u:(uint64_t)n,b=d<0?(uint64_t)(-(d+1))+1u:(uint64_t)d,q=__udivdi3(a,b);return neg?-(int64_t)q:(int64_t)q;}
#endif
static hybrid_core_t core;
static hybrid_wavetable_t wavetable[3];
static int32_t output[HYBRID_MAX_FRAMES*2u];
static int32_t pcm_input[HYBRID_CHANNELS][HYBRID_MAX_FRAMES*2u];
static uint8_t ready;
__attribute__((visibility("default"))) int hybrid_wasm_init(void){
 hybrid_init_defaults(&core);
 if(hybrid_register_wavetable_engine(&core,&wavetable[0]))return -1;
 hybrid_wavetable_defaults(&wavetable[1]);
 if(hybrid_register_engine(&core,HYBRID_FM6,&hybrid_wavetable_ops,&wavetable[1]))return -1;
 hybrid_wavetable_defaults(&wavetable[2]);
 if(hybrid_register_engine(&core,HYBRID_SAMPLE,&hybrid_wavetable_ops,&wavetable[2]))return -1;
 ready=(uint8_t)(hybrid_start(&core)==0);return ready?0:-1;
}
__attribute__((visibility("default"))) int hybrid_wasm_set_mix(uint32_t id,uint32_t volume,int32_t pan,uint32_t mute,uint32_t solo){if(!ready||id>=4)return -1;hybrid_set_volume(&core,(hybrid_channel_id_t)id,(uint16_t)(volume>32767?32767:volume));hybrid_set_pan(&core,(hybrid_channel_id_t)id,(int16_t)pan);hybrid_set_mute(&core,(hybrid_channel_id_t)id,(uint8_t)mute);hybrid_set_solo(&core,(hybrid_channel_id_t)id,(uint8_t)solo);return 0;}
__attribute__((visibility("default"))) int hybrid_wasm_set_route(uint32_t id,uint32_t mode,uint32_t low,uint32_t high){if(!ready||id>=4||mode>HYBRID_ROUTE_ZONE)return -1;hybrid_set_route(&core,(hybrid_channel_id_t)id,(hybrid_route_mode_t)mode,(uint8_t)low,(uint8_t)high);return 0;}
__attribute__((visibility("default"))) int hybrid_wasm_set_midi(uint32_t id,uint32_t channel,uint32_t vlo,uint32_t vhi,int32_t transpose){hybrid_channel_config_t*c;if(!ready||id>=4)return -1;c=&core.engine[id].config;c->midi_channel=(uint8_t)channel;c->velocity_low=(uint8_t)vlo;c->velocity_high=(uint8_t)vhi;c->transpose=(int8_t)transpose;return 0;}
__attribute__((visibility("default"))) void hybrid_wasm_note_on(uint32_t n,uint32_t v,uint32_t ch){if(ready)hybrid_note_on(&core,(uint8_t)n,(uint8_t)v,(uint8_t)ch);}
__attribute__((visibility("default"))) void hybrid_wasm_note_off(uint32_t n,uint32_t ch){if(ready)hybrid_note_off(&core,(uint8_t)n,(uint8_t)ch);}
__attribute__((visibility("default"))) uintptr_t hybrid_wasm_pcm_ptr(uint32_t id){return ready&&id<4?(uintptr_t)pcm_input[id]:0;}
__attribute__((visibility("default"))) int hybrid_wasm_submit_pcm(uint32_t id,uint32_t frames){if(!ready||id>=4||!frames||frames>HYBRID_MAX_FRAMES)return -1;return hybrid_submit_pcm(&core,(hybrid_channel_id_t)id,pcm_input[id],(uint16_t)frames);}
__attribute__((visibility("default"))) void hybrid_wasm_discard_pcm(uint32_t id){if(ready&&id<4)hybrid_discard_pcm(&core,(hybrid_channel_id_t)id);}
__attribute__((visibility("default"))) int hybrid_wasm_process(uint32_t frames){if(!ready||!frames||frames>HYBRID_MAX_FRAMES)return -1;return hybrid_process(&core,output,(uint16_t)frames);}
__attribute__((visibility("default"))) uintptr_t hybrid_wasm_output_ptr(void){return (uintptr_t)output;}
__attribute__((visibility("default"))) uint32_t hybrid_wasm_voice_on(uint32_t id){(void)id;return 0;}
__attribute__((visibility("default"))) uint32_t hybrid_wasm_voice_off(uint32_t id){(void)id;return 0;}
__attribute__((visibility("default"))) int hybrid_wasm_scene_capture(uint32_t n){return ready?hybrid_scene_capture(&core,(uint8_t)n):-1;}
__attribute__((visibility("default"))) int hybrid_wasm_scene_recall(uint32_t n,uint32_t ms){return ready?hybrid_scene_recall(&core,(uint8_t)n,(uint16_t)ms):-1;}
__attribute__((visibility("default"))) void hybrid_wasm_panic(void){if(ready)hybrid_panic(&core);}
__attribute__((visibility("default"))) uint32_t hybrid_wasm_core_size(void){return (uint32_t)sizeof(core);}
