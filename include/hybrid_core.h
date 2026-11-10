/* SPDX-License-Identifier: GPL-3.0-only
 * SMK-37 HYBRID common engine interface.
 * Audio samples are signed Q15 values in int32_t containers.
 * Render buffers are interleaved stereo: L, R, L, R...
 */
#ifndef SMK37_HYBRID_CORE_H
#define SMK37_HYBRID_CORE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define HYBRID_CHANNELS 4u
#define HYBRID_SCENES 16u
#define HYBRID_MAX_FRAMES 128u
#define HYBRID_ENGINE_STATE 64u
#define HYBRID_SAMPLE_RATE 44100u
typedef enum { HYBRID_AMB=0, HYBRID_DUB=1, HYBRID_ZEN=2, HYBRID_GHOULBOX=3 } hybrid_channel_id_t;
typedef enum { HYBRID_ROUTE_OFF=0, HYBRID_ROUTE_GLOBAL, HYBRID_ROUTE_ZONE } hybrid_route_mode_t;
typedef struct {
 uint8_t enabled, route_mode, midi_channel;
 int8_t transpose;
 uint8_t key_low, key_high, velocity_low, velocity_high, mute, solo;
 uint16_t volume;
 int16_t pan;
 uint16_t reserved;
} hybrid_channel_config_t;
typedef struct hybrid_engine_ops {
 const char *name;
 int (*init)(void *state);
 void (*reset)(void *state);
 void (*note_on)(void *state,uint8_t note,uint8_t velocity);
 void (*note_off)(void *state,uint8_t note);
 void (*control)(void *state,uint16_t parameter,int32_t value);
 void (*render)(void *state,int32_t *stereo_q15,uint16_t frames);
 void (*panic)(void *state);
 int (*save_scene)(void *state,uint8_t *dst,uint16_t capacity);
 int (*load_scene)(void *state,const uint8_t *src,uint16_t size);
} hybrid_engine_ops_t;
typedef struct {
 hybrid_channel_config_t channel[HYBRID_CHANNELS];
 uint8_t engine_state_size[HYBRID_CHANNELS];
 uint8_t engine_state[HYBRID_CHANNELS][HYBRID_ENGINE_STATE];
 uint32_t checksum;
} hybrid_scene_t;
typedef struct {
 const hybrid_engine_ops_t *ops;
 void *state;
 hybrid_channel_config_t config;
 int32_t scratch[HYBRID_MAX_FRAMES*2u];
 /* One-shot PCM ingress for engines hosted in independent WASM memories.
  * Samples are interleaved stereo Q15 and consumed by the next matching process block. */
 const int32_t *pcm_input;
 uint16_t pcm_frames;
 uint8_t pcm_valid, pcm_reserved;
} hybrid_engine_slot_t;
typedef struct {
 hybrid_engine_slot_t engine[HYBRID_CHANNELS];
 hybrid_scene_t scenes[HYBRID_SCENES];
 uint8_t scene_valid[HYBRID_SCENES], selected_engine, initialized, transition_active, transition_reserved;
 uint32_t transition_total, transition_elapsed;
 hybrid_channel_config_t transition_from[HYBRID_CHANNELS], transition_to[HYBRID_CHANNELS];
 uint8_t master_mute;
} hybrid_core_t;
void hybrid_init_defaults(hybrid_core_t *core);
int hybrid_register_engine(hybrid_core_t *core,hybrid_channel_id_t id,const hybrid_engine_ops_t *ops,void *state);
int hybrid_start(hybrid_core_t *core);
void hybrid_reset(hybrid_core_t *core);
void hybrid_set_route(hybrid_core_t *core,hybrid_channel_id_t id,hybrid_route_mode_t mode,uint8_t low,uint8_t high);
void hybrid_set_volume(hybrid_core_t *core,hybrid_channel_id_t id,uint16_t q15);
void hybrid_set_pan(hybrid_core_t *core,hybrid_channel_id_t id,int16_t pan);
void hybrid_set_mute(hybrid_core_t *core,hybrid_channel_id_t id,uint8_t mute);
void hybrid_set_solo(hybrid_core_t *core,hybrid_channel_id_t id,uint8_t solo);
void hybrid_note_on(hybrid_core_t *core,uint8_t note,uint8_t velocity,uint8_t midi_channel);
void hybrid_note_off(hybrid_core_t *core,uint8_t note,uint8_t midi_channel);
void hybrid_control(hybrid_core_t *core,uint16_t parameter,int32_t value,int8_t engine_id);
/* Submit one stereo Q15 block for an engine. frames must match the next hybrid_process call.
 * A pending PCM block takes precedence over that engine's render callback exactly once. */
int hybrid_submit_pcm(hybrid_core_t *core,hybrid_channel_id_t id,const int32_t *stereo_q15,uint16_t frames);
void hybrid_discard_pcm(hybrid_core_t *core,hybrid_channel_id_t id);
int hybrid_process(hybrid_core_t *core,int32_t *out_stereo_q15,uint16_t frames);
int hybrid_scene_capture(hybrid_core_t *core,uint8_t scene);
int hybrid_scene_recall(hybrid_core_t *core,uint8_t scene,uint16_t fade_ms);
void hybrid_scene_clear(hybrid_core_t *core,uint8_t scene);
void hybrid_panic(hybrid_core_t *core);
#ifdef __cplusplus
}
#endif
#endif
