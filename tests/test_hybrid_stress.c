/* SPDX-License-Identifier: GPL-3.0-only
 * Host stress tests for the SMK-37 HYBRID mixer and four engine slots.
 * These tests exercise the common core with deterministic engine doubles;
 * they do NOT emulate or certify the native AMB/DUB/ZEN/GHOULBOX DSPs.
 */
#include "hybrid_core.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t init_count, render_calls, rendered_frames;
    uint32_t note_ons, note_offs, panics, controls;
    uint32_t phase;
    int32_t amplitude;
} stress_engine_t;

static int stress_init(void *p)
{
    stress_engine_t *s = (stress_engine_t *)p;
    s->init_count++;
    return 0;
}
static void stress_reset(void *p) { (void)p; }
static void stress_note_on(void *p, uint8_t note, uint8_t velocity)
{
    stress_engine_t *s = (stress_engine_t *)p;
    (void)note; (void)velocity; s->note_ons++;
}
static void stress_note_off(void *p, uint8_t note)
{
    stress_engine_t *s = (stress_engine_t *)p;
    (void)note; s->note_offs++;
}
static void stress_control(void *p, uint16_t parameter, int32_t value)
{
    stress_engine_t *s = (stress_engine_t *)p;
    (void)parameter; (void)value; s->controls++;
}
static void stress_render(void *p, int32_t *out, uint16_t frames)
{
    stress_engine_t *s = (stress_engine_t *)p;
    uint16_t i;
    s->render_calls++;
    s->rendered_frames += frames;
    for (i = 0; i < frames; ++i) {
        /* Deterministic bounded signal with changing polarity. */
        int32_t v = ((s->phase++ & 1u) ? s->amplitude : -s->amplitude);
        out[2u * i] = v;
        out[2u * i + 1u] = -v;
    }
}
static void stress_panic(void *p)
{
    stress_engine_t *s = (stress_engine_t *)p;
    s->panics++;
}
static const hybrid_engine_ops_t stress_ops = {
    "stress-double", stress_init, stress_reset, stress_note_on,
    stress_note_off, stress_control, stress_render, stress_panic, 0, 0
};

static void assert_q15(const int32_t *p, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; ++i)
        assert(p[i] >= -32768 && p[i] <= 32767);
}

int main(void)
{
    static const uint16_t blocks[] = {32, 64, 96, 128};
    hybrid_core_t core;
    stress_engine_t engines[HYBRID_CHANNELS] = {{0}};
    int32_t output[HYBRID_MAX_FRAMES * 2u];
    uint32_t iteration, total_frames = 0;
    uint8_t e;

    hybrid_init_defaults(&core);
    for (e = 0; e < HYBRID_CHANNELS; ++e) {
        engines[e].amplitude = 9000 + (int32_t)e * 1500;
        assert(hybrid_register_engine(&core, (hybrid_channel_id_t)e,
                                      &stress_ops, &engines[e]) == 0);
    }
    assert(hybrid_start(&core) == 0);
    for (e = 0; e < HYBRID_CHANNELS; ++e)
        assert(engines[e].init_count == 1);

    /* Exercise zone filtering, channel routing and transposition. */
    for (e = 0; e < HYBRID_CHANNELS; ++e) {
        hybrid_set_route(&core, (hybrid_channel_id_t)e,
                         HYBRID_ROUTE_ZONE, 48, 84);
        core.engine[e].config.midi_channel = (uint8_t)(e + 1u);
        core.engine[e].config.transpose = (int8_t)e;
    }
    for (e = 0; e < HYBRID_CHANNELS; ++e) {
        hybrid_note_on(&core, 60, 100, (uint8_t)(e + 1u));
        hybrid_note_off(&core, 60, (uint8_t)(e + 1u));
        assert(engines[e].note_ons == 1 && engines[e].note_offs == 1);
    }

    /* Long deterministic render run: mixed block sizes, all four engines. */
    for (iteration = 0; iteration < 12000u; ++iteration) {
        uint16_t frames = blocks[iteration % 4u];
        if (iteration == 3000u) hybrid_set_mute(&core, HYBRID_DUB, 1);
        if (iteration == 4000u) hybrid_set_mute(&core, HYBRID_DUB, 0);
        if (iteration == 5000u) hybrid_set_solo(&core, HYBRID_ZEN, 1);
        if (iteration == 6000u) hybrid_set_solo(&core, HYBRID_ZEN, 0);
        if (iteration == 7000u) {
            assert(hybrid_scene_capture(&core, 3) == 0);
            hybrid_set_volume(&core, HYBRID_AMB, 12000);
            hybrid_set_pan(&core, HYBRID_DUB, 75);
            assert(hybrid_scene_recall(&core, 3, 1000) == 0);
        }
        if (iteration == 9000u) hybrid_panic(&core);
        assert(hybrid_process(&core, output, frames) == 0);
        assert_q15(output, (uint32_t)frames * 2u);
        total_frames += frames;
    }

    assert(total_frames > 900000u);
    for (e = 0; e < HYBRID_CHANNELS; ++e) {
        assert(engines[e].render_calls > 0u);
        assert(engines[e].rendered_frames > 1000000u / 2u);
        assert(engines[e].panics == 1u);
    }

    /* Repeated lifecycle and invalid block guards. */
    hybrid_reset(&core);
    assert(hybrid_start(&core) == 0);
    assert(hybrid_process(&core, output, 0) == -1);
    assert(hybrid_process(&core, output, HYBRID_MAX_FRAMES + 1u) == -1);
    hybrid_panic(&core);

    puts("SMK-37 HYBRID host stress: PASS");
    printf("Rendered %lu frames across four engine doubles; Q15 bounds PASS\n",
           (unsigned long)total_frames);
    return 0;
}
