/* SPDX-License-Identifier: GPL-3.0-only
 * SMK-37 HYBRID adapters for the upstream AMB-1, DUB-1 and ZEN-1 DSPs.
 *
 * Upstream renderers produce signed 24-bit samples in int32_t containers.
 * hybrid_core consumes signed Q15 in int32_t containers. Convert only after
 * each native renderer has completed its block; the common mixer then applies
 * per-channel gain, pan, mute and solo.
 */
#include <stdint.h>
#include "hybrid_core.h"
#include "hybrid_native_ports.h"

#define HYBRID_RENDER_MAX 128u
#define HYBRID_NATIVE_GAIN_Q12 4096u

static int32_t native_lr[HYBRID_RENDER_MAX * 2u];

static void clear_native(void)
{
    uint32_t i;
    for (i = 0; i < HYBRID_RENDER_MAX * 2u; ++i)
        native_lr[i] = 0;
}

static void native24_to_q15(int32_t *dst, uint16_t frames)
{
    uint32_t i;
    for (i = 0; i < (uint32_t)frames * 2u; ++i)
        dst[i] = native_lr[i] >> 8;
}

/* AMB-1 is a self-running generative instrument. MIDI note events are not
 * translated into arbitrary notes: its native bell/performance interface is
 * not equivalent to a conventional note allocator. MIDI controls and clock
 * need a dedicated event adapter in the next integration stage. */
static int amb_init(void *state) { (void)state; amb_en_init(); amb_en_play(1); return 0; }
static void amb_reset(void *state) { (void)state; amb_en_panic(); amb_en_init(); }
static void amb_note_on(void *state, uint8_t note, uint8_t velocity)
{ (void)state; (void)note; (void)velocity; }
static void amb_note_off(void *state, uint8_t note) { (void)state; (void)note; }
static void amb_control(void *state, uint16_t p, int32_t v)
{ (void)state; amb_en_set((int)p, (int)v); }
static void amb_render(void *state, int32_t *out, uint16_t frames)
{
    (void)state;
    if (!out || !frames || frames > HYBRID_RENDER_MAX) return;
    clear_native();
    amb_en_render(native_lr, frames, HYBRID_NATIVE_GAIN_Q12);
    native24_to_q15(out, frames);
}
static void amb_panic(void *state) { (void)state; amb_en_panic(); }

/* DUB-1 is likewise generative; its own freeze/arrangement controls remain
 * available through the native parameter/control namespace. */
static int dub_init(void *state) { (void)state; dub_en_init(); dub_en_play(1); return 0; }
static void dub_reset(void *state) { (void)state; dub_en_panic(); dub_en_init(); }
static void dub_note_on(void *state, uint8_t note, uint8_t velocity)
{ (void)state; (void)note; (void)velocity; }
static void dub_note_off(void *state, uint8_t note) { (void)state; (void)note; }
static void dub_control(void *state, uint16_t p, int32_t v)
{
    (void)state;
    /* Reserve this adapter command for the DUB freeze toggle; all other
       parameter numbers are passed unchanged to the native engine. */
    if (p == 0xFFFFu) dub_en_freeze(v != 0);
    else dub_en_set((int)p, (int)v);
}
static void dub_render(void *state, int32_t *out, uint16_t frames)
{
    (void)state;
    if (!out || !frames || frames > HYBRID_RENDER_MAX) return;
    clear_native();
    dub_en_render(native_lr, frames, HYBRID_NATIVE_GAIN_Q12);
    native24_to_q15(out, frames);
}
static void dub_panic(void *state) { (void)state; dub_en_panic(); }

/* ZEN-1 accepts MIDI-like note IDs and velocity directly. Its cents argument
 * is relative to A4, so MIDI note 69 maps to 0 cents. */
static int zen_init(void *state) { (void)state; zen_fm_init(); return 0; }
static void zen_reset(void *state) { (void)state; zen_fm_panic(); zen_fm_init(); }
static void zen_note_on(void *state, uint8_t note, uint8_t velocity)
{
    (void)state;
    zen_fm_note_vel(32 + (int)note, 1, ((float)((int)note - 69)) * 100.0f, velocity);
}
static void zen_note_off(void *state, uint8_t note)
{ (void)state; zen_fm_note_vel(32 + (int)note, 0, ((float)((int)note - 69)) * 100.0f, 0); }
static void zen_control(void *state, uint16_t p, int32_t v)
{ (void)state; zen_fm_set((int)p, (int)v); }
static void zen_render(void *state, int32_t *out, uint16_t frames)
{
    (void)state;
    if (!out || !frames || frames > HYBRID_RENDER_MAX) return;
    clear_native();
    zen_fm_render(native_lr, frames, HYBRID_NATIVE_GAIN_Q12);
    native24_to_q15(out, frames);
}
static void zen_panic(void *state) { (void)state; zen_fm_panic(); }

int hybrid_register_native_engines(hybrid_core_t *core)
{
    if (!core) return -1;
    if (hybrid_register_engine(core, HYBRID_AMB, &hybrid_amb_ops, 0) != 0) return -1;
    if (hybrid_register_engine(core, HYBRID_DUB, &hybrid_dub_ops, 0) != 0) return -1;
    if (hybrid_register_engine(core, HYBRID_ZEN, &hybrid_zen_ops, 0) != 0) return -1;
    return 0;
}

const hybrid_engine_ops_t hybrid_amb_ops = {
    "AMB-1", amb_init, amb_reset, amb_note_on, amb_note_off,
    amb_control, amb_render, amb_panic, 0, 0
};
const hybrid_engine_ops_t hybrid_dub_ops = {
    "DUB-1", dub_init, dub_reset, dub_note_on, dub_note_off,
    dub_control, dub_render, dub_panic, 0, 0
};
const hybrid_engine_ops_t hybrid_zen_ops = {
    "ZEN-1", zen_init, zen_reset, zen_note_on, zen_note_off,
    zen_control, zen_render, zen_panic, 0, 0
};
