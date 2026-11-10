/* SPDX-License-Identifier: GPL-3.0-only
 * Browser/WASM adapter: deterministic test voices feeding the real HYBRID C mixer.
 * These are test signal generators, not the four proprietary AC79 DSP engines.
 */
#include <stdint.h>
#include "hybrid_core.h"

typedef struct { int32_t phase, step, level; uint32_t on, off; uint8_t note, velocity, active; } wasm_voice_t;
static hybrid_core_t core;
static wasm_voice_t voices[HYBRID_CHANNELS];
static int32_t output[HYBRID_MAX_FRAMES * 2u];
static uint8_t ready;

static int test_init(void *p) { wasm_voice_t *v=(wasm_voice_t*)p; v->phase=0; return 0; }
static void test_reset(void *p) { wasm_voice_t *v=(wasm_voice_t*)p; v->active=0; v->level=0; }
static void test_on(void *p,uint8_t n,uint8_t vel) {
    wasm_voice_t *v=(wasm_voice_t*)p; v->note=n; v->velocity=vel;
    v->step=16000+(int32_t)n*271; v->level=(int32_t)vel*220;
    v->active=1; v->on++;
}
static void test_off(void *p,uint8_t n) {
    wasm_voice_t *v=(wasm_voice_t*)p; if(v->active && v->note==n) v->active=0; v->off++;
}
static void test_control(void *p,uint16_t id,int32_t value) {
    wasm_voice_t *v=(wasm_voice_t*)p; if(id==0) v->level=value;
}
static void test_render(void *p,int32_t *dst,uint16_t frames) {
    wasm_voice_t *v=(wasm_voice_t*)p; uint16_t i;
    for(i=0;i<frames;i++) {
        int32_t sample=0;
        if(v->active) {
            v->phase += v->step;
            sample=((v->phase & 0x8000) ? 1 : -1) * v->level;
        }
        dst[2u*i]=sample; dst[2u*i+1u]=sample;
    }
}
static void test_panic(void *p) { test_reset(p); }
static const hybrid_engine_ops_t ops[HYBRID_CHANNELS]={
    {"AMB-1 wasm test",test_init,test_reset,test_on,test_off,test_control,test_render,test_panic,0,0},
    {"DUB-1 wasm test",test_init,test_reset,test_on,test_off,test_control,test_render,test_panic,0,0},
    {"ZEN-1 wasm test",test_init,test_reset,test_on,test_off,test_control,test_render,test_panic,0,0},
    {"GHOULBOX wasm test",test_init,test_reset,test_on,test_off,test_control,test_render,test_panic,0,0}
};

__attribute__((visibility("default"))) int hybrid_wasm_init(void) {
    uint32_t i;
    hybrid_init_defaults(&core);
    for(i=0;i<HYBRID_CHANNELS;i++) {
        voices[i].phase=0; voices[i].step=18000+(int32_t)i*3000;
        if(hybrid_register_engine(&core,(hybrid_channel_id_t)i,&ops[i],&voices[i])) return -1;
    }
    ready=(uint8_t)(hybrid_start(&core)==0); return ready?0:-1;
}
__attribute__((visibility("default"))) int hybrid_wasm_set_mix(uint32_t id,uint32_t volume,int32_t pan,uint32_t mute,uint32_t solo) {
    if(!ready||id>=HYBRID_CHANNELS)return -1;
    hybrid_set_volume(&core,(hybrid_channel_id_t)id,(uint16_t)(volume>32767?32767:volume));
    hybrid_set_pan(&core,(hybrid_channel_id_t)id,(int16_t)pan);
    hybrid_set_mute(&core,(hybrid_channel_id_t)id,(uint8_t)mute);
    hybrid_set_solo(&core,(hybrid_channel_id_t)id,(uint8_t)solo); return 0;
}
__attribute__((visibility("default"))) int hybrid_wasm_set_route(uint32_t id,uint32_t mode,uint32_t low,uint32_t high) {
    if(!ready||id>=HYBRID_CHANNELS||mode>HYBRID_ROUTE_ZONE)return -1;
    hybrid_set_route(&core,(hybrid_channel_id_t)id,(hybrid_route_mode_t)mode,(uint8_t)low,(uint8_t)high); return 0;
}
__attribute__((visibility("default"))) int hybrid_wasm_set_midi(uint32_t id,uint32_t channel,uint32_t vlow,uint32_t vhigh,int32_t transpose) {
    hybrid_channel_config_t *c;
    if(!ready||id>=HYBRID_CHANNELS)return -1;
    c=&core.engine[id].config;c->midi_channel=(uint8_t)channel;
    c->velocity_low=(uint8_t)vlow;c->velocity_high=(uint8_t)vhigh;
    c->transpose=(int8_t)transpose;return 0;
}
__attribute__((visibility("default"))) void hybrid_wasm_note_on(uint32_t note,uint32_t velocity,uint32_t channel) {
    if(ready)hybrid_note_on(&core,(uint8_t)note,(uint8_t)velocity,(uint8_t)channel);
}
__attribute__((visibility("default"))) void hybrid_wasm_note_off(uint32_t note,uint32_t channel) {
    if(ready)hybrid_note_off(&core,(uint8_t)note,(uint8_t)channel);
}
__attribute__((visibility("default"))) int hybrid_wasm_process(uint32_t frames) {
    if(!ready||!frames||frames>HYBRID_MAX_FRAMES)return -1;
    if(hybrid_process(&core,output,(uint16_t)frames))return -1;
    return 0;
}
__attribute__((visibility("default"))) uint32_t hybrid_wasm_output_ptr(void) { return (uint32_t)(uintptr_t)output; }
__attribute__((visibility("default"))) uint32_t hybrid_wasm_voice_on(uint32_t id) { return id<4?voices[id].on:0; }
__attribute__((visibility("default"))) uint32_t hybrid_wasm_voice_off(uint32_t id) { return id<4?voices[id].off:0; }
__attribute__((visibility("default"))) int hybrid_wasm_scene_capture(uint32_t n) { return ready?hybrid_scene_capture(&core,(uint8_t)n):-1; }
__attribute__((visibility("default"))) int hybrid_wasm_scene_recall(uint32_t n,uint32_t ms) { return ready?hybrid_scene_recall(&core,(uint8_t)n,(uint16_t)ms):-1; }
__attribute__((visibility("default"))) void hybrid_wasm_panic(void) { if(ready)hybrid_panic(&core); }
__attribute__((visibility("default"))) uint32_t hybrid_wasm_core_size(void) { return (uint32_t)sizeof(core); }
