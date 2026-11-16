/* SPDX-License-Identifier: GPL-3.0-only
 * HYBRID wavetable extended features. Independent implementation.
 */
#ifndef SMK37_HYBRID_WAVETABLE_PRO_H
#define SMK37_HYBRID_WAVETABLE_PRO_H
#include <stdint.h>
#include "hybrid_wavetable.h"
#define HYBRID_WT_CUSTOM_SLOTS 4u
#define HYBRID_WT_TABLE_SAMPLES 64u
#define HYBRID_WT_MOD_SLOTS 4u
#define HYBRID_WT_PRESET_VERSION 1u
typedef enum {
 HYBRID_WT_MOD_OFF=0, HYBRID_WT_MOD_LFO=1, HYBRID_WT_MOD_VELOCITY=2,
 HYBRID_WT_MOD_KEY=3, HYBRID_WT_MOD_ENVELOPE=4
} hybrid_wt_mod_source_t;
typedef enum {
 HYBRID_WT_DEST_OFF=0, HYBRID_WT_DEST_PITCH=1, HYBRID_WT_DEST_CUTOFF=2,
 HYBRID_WT_DEST_OSC_MIX=3, HYBRID_WT_DEST_AMP=4
} hybrid_wt_mod_dest_t;
typedef struct {
 uint8_t source,destination;
 int16_t depth_q15;
} hybrid_wt_mod_route_t;
typedef struct {
 uint32_t magic;
 uint16_t version,size;
 uint8_t wave_a,wave_b,unison_voices,sub_level,noise_level,custom_valid_mask;
 uint16_t osc2_mix,attack,decay,sustain,release,cutoff,resonance,unison_detune;
 int16_t custom[HYBRID_WT_CUSTOM_SLOTS][HYBRID_WT_TABLE_SAMPLES];
 hybrid_wt_mod_route_t modulation[HYBRID_WT_MOD_SLOTS];
 uint32_t crc32;
} hybrid_wt_preset_t;
typedef struct {
 int16_t pitch_q15,cutoff_q15,osc_mix_q15,amp_q15;
} hybrid_wt_mod_values_t;
/* Import signed PCM (any length >= 2), linearly resampled to 64 points. */
int hybrid_wavetable_import(hybrid_wavetable_t *state,uint8_t slot,const int16_t *pcm,uint16_t count);
int hybrid_wavetable_clear_custom(hybrid_wavetable_t *state,uint8_t slot);
int32_t hybrid_wavetable_custom_sample(const hybrid_wavetable_t *state,uint8_t slot,uint32_t phase);
/* Utility layers used by the renderer; state is caller-owned and deterministic. */
int32_t hybrid_wavetable_sub_sample(uint32_t phase,uint8_t octave_divider);
int32_t hybrid_wavetable_noise_sample(uint32_t *seed);
uint8_t hybrid_wavetable_unison_count(uint8_t requested);
void hybrid_wavetable_mod_evaluate(const hybrid_wt_mod_route_t routes[HYBRID_WT_MOD_SLOTS],
 int16_t lfo_q15,uint8_t velocity,uint8_t note,int16_t envelope_q15,hybrid_wt_mod_values_t *out);
/* Fixed-size, versioned preset blob with CRC32; no filesystem or heap required. */
int hybrid_wavetable_preset_save(const hybrid_wavetable_t *state,const hybrid_wt_mod_route_t routes[HYBRID_WT_MOD_SLOTS],hybrid_wt_preset_t *out);
int hybrid_wavetable_preset_load(hybrid_wavetable_t *state,hybrid_wt_mod_route_t routes[HYBRID_WT_MOD_SLOTS],const hybrid_wt_preset_t *preset);
#endif
