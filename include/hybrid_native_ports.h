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

#endif
