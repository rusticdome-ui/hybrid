/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef SMK37_HYBRID_NATIVE_PORTS_H
#define SMK37_HYBRID_NATIVE_PORTS_H
#include <stdint.h>
#include "hybrid_core.h"
#include "hybrid_wavetable.h"
#include "hybrid_wavetable_pro.h"
int hybrid_register_wavetable_engine(hybrid_core_t *core,hybrid_wavetable_t *state);
int hybrid_audio_init(void);
void hybrid_audio_process(int32_t *out_q15,uint32_t frames,int32_t master_q12);
void hybrid_audio_note_on(uint8_t note,uint8_t velocity);
void hybrid_audio_note_off(uint8_t note);
void hybrid_audio_panic(void);
void hybrid_audio_midi_note_on(uint8_t note,uint8_t velocity,uint8_t channel);
void hybrid_audio_midi_note_off(uint8_t note,uint8_t channel);
void hybrid_audio_midi_panic(uint8_t channel);
/* Native Wavetable PRO control/import/preset API; caller owns PCM and preset buffers. */
void hybrid_audio_wavetable_control(uint16_t parameter,int32_t value);
int hybrid_audio_wavetable_import(uint8_t slot,const int16_t *pcm,uint16_t count);
int hybrid_audio_wavetable_clear(uint8_t slot);
int hybrid_audio_wavetable_mod_route(uint8_t slot,uint8_t source,uint8_t destination,int16_t depth_q15);
int hybrid_audio_wavetable_preset_save(hybrid_wt_preset_t *out);
int hybrid_audio_wavetable_preset_load(const hybrid_wt_preset_t *preset);
#endif
