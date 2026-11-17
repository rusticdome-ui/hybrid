/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef SMK37_HYBRID_NATIVE_PORTS_H
#define SMK37_HYBRID_NATIVE_PORTS_H
#include <stdint.h>
#include "hybrid_core.h"
#include "hybrid_wavetable.h"
int hybrid_register_wavetable_engine(hybrid_core_t *core,hybrid_wavetable_t *state);
int hybrid_audio_init(void);
void hybrid_audio_process(int32_t *out_q15,uint32_t frames,int32_t master_q12);
void hybrid_audio_note_on(uint8_t note,uint8_t velocity);
void hybrid_audio_note_off(uint8_t note);
void hybrid_audio_panic(void);
void hybrid_audio_midi_note_on(uint8_t note,uint8_t velocity,uint8_t channel);
void hybrid_audio_midi_note_off(uint8_t note,uint8_t channel);
void hybrid_audio_midi_panic(uint8_t channel);
#endif
