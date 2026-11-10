/* SPDX-License-Identifier: GPL-3.0-only
 * SMK-37 HYBRID native DSP port symbols.
 *
 * These are deliberately namespaced exports from the upstream DSP translation
 * units. AMB-1 and DUB-1 both define en_*; the build must compile them as
 * separate objects with symbol-renaming wrappers before linking this bridge.
 */
#ifndef SMK37_HYBRID_NATIVE_PORTS_H
#define SMK37_HYBRID_NATIVE_PORTS_H
#include <stdint.h>
#include "hybrid_core.h"

extern const hybrid_engine_ops_t hybrid_amb_ops;
extern const hybrid_engine_ops_t hybrid_dub_ops;
extern const hybrid_engine_ops_t hybrid_zen_ops;
int hybrid_register_native_engines(hybrid_core_t *core);

void amb_en_init(void);
void amb_en_set(int param, int value);
void amb_en_play(int on);
void amb_en_panic(void);
void amb_en_render(int32_t *out_lr, uint32_t frames, uint32_t gain_q12);

void dub_en_init(void);
void dub_en_set(int param, int value);
void dub_en_play(int on);
void dub_en_freeze(int on);
void dub_en_panic(void);
void dub_en_render(int32_t *out_lr, uint32_t frames, uint32_t gain_q12);

void zen_fm_init(void);
void zen_fm_set(int param, int value);
void zen_fm_note_vel(int id, int on, float cents, int velocity);
void zen_fm_panic(void);
void zen_fm_render(int32_t *out_lr, uint32_t frames, uint32_t gain_q12);

/* Runtime bridge exported by hybrid_native_adapters.c. */
int hybrid_audio_init(void);
void hybrid_audio_process(int32_t *out_q15, uint32_t frames, int32_t master_q12);
void hybrid_audio_note_on(uint8_t note, uint8_t velocity);
void hybrid_audio_note_off(uint8_t note);
void hybrid_audio_panic(void);
void hybrid_audio_midi_note_on(uint8_t note, uint8_t velocity, uint8_t channel);
void hybrid_audio_midi_note_off(uint8_t note, uint8_t channel);
void hybrid_audio_midi_panic(uint8_t channel);

#endif
