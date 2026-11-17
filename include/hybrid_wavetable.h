/* SPDX-License-Identifier: GPL-3.0-only
 * HYBRID independent wavetable synthesizer. No SHRUB proprietary code/assets.
 */
#ifndef SMK37_HYBRID_WAVETABLE_H
#define SMK37_HYBRID_WAVETABLE_H
#include "hybrid_core.h"
typedef struct {
 uint32_t phase_a,phase_b,step_a,step_b;
 int32_t envelope,filter_state;
 uint8_t note,velocity,stage,active;
} hybrid_wavetable_voice_t;
typedef struct {
 hybrid_wavetable_voice_t voice[16];
 uint16_t attack,decay,sustain,release,cutoff,resonance,lfo_rate,lfo_depth;
 uint8_t wave_a,wave_b,detune,unison;
 uint16_t osc2_mix;
 uint32_t lfo_phase;
} hybrid_wavetable_t;
void hybrid_wavetable_defaults(hybrid_wavetable_t *state);
extern const hybrid_engine_ops_t hybrid_wavetable_ops;
int hybrid_register_wavetable_engine(hybrid_core_t *core,hybrid_wavetable_t *state);
#endif
