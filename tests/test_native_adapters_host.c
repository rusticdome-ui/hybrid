/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the real AMB/DUB/ZEN DSP objects through the HYBRID native adapters.
 * No native_port_stubs.c is linked into this test. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "hybrid_core.h"
#include "hybrid_profile.h"
#include "hybrid_native_ports.h"

extern int hybrid_audio_init(void);
extern void hybrid_audio_process(int32_t *out_q15, uint32_t frames, int32_t master_q12);
extern void hybrid_audio_note_on(uint8_t note, uint8_t velocity);
extern void hybrid_audio_note_off(uint8_t note);
extern void hybrid_audio_panic(void);

uint32_t test_fm1_ticks;
char _ustack_lo[4], _ustack_top[4], _sstack_lo[4], _sstack_top[4];
void en_midi_out(uint32_t status, uint32_t d1, uint32_t d2)
{ (void)status; (void)d1; (void)d2; }
void fm_midi_out(uint32_t status, uint32_t d1, uint32_t d2)
{ (void)status; (void)d1; (void)d2; }

#define GUARD_L 0x13579BDF
#define GUARD_R 0x2468ACE0
static uint32_t nonzero_samples;
static void render_checked(uint16_t frames)
{
    int32_t guarded[HYBRID_MAX_FRAMES * 2u + 2u];
    uint32_t i;
    guarded[0] = GUARD_L;
    guarded[HYBRID_MAX_FRAMES * 2u + 1u] = GUARD_R;
    for (i = 0; i < (uint32_t)frames * 2u; ++i) guarded[i + 1u] = 0;
    hybrid_audio_process(&guarded[1], frames, 4096);
    assert(guarded[0] == GUARD_L);
    assert(guarded[HYBRID_MAX_FRAMES * 2u + 1u] == GUARD_R);
    for (i = 0; i < (uint32_t)frames * 2u; ++i) {
        assert(guarded[i + 1u] >= -32768 && guarded[i + 1u] <= 32767);
        nonzero_samples += guarded[i + 1u] != 0;
    }
}

static void invalid_buffer_tests(void)
{
    int32_t b[HYBRID_MAX_FRAMES * 2u + 2u];
    uint32_t i;
    for (i = 0; i < HYBRID_MAX_FRAMES * 2u + 2u; ++i) b[i] = 0x12345678;
    hybrid_audio_process(&b[1], 0, 4096);
    hybrid_audio_process(&b[1], HYBRID_MAX_FRAMES + 1u, 4096);
    hybrid_audio_process(&b[1], HYBRID_MAX_FRAMES, 0);
    for (i = 0; i < HYBRID_MAX_FRAMES * 2u + 2u; ++i) assert(b[i] == 0x12345678);
}

int main(void)
{
    static const uint16_t blocks[] = {32, 64, 96, 128};
    hybrid_profile_snapshot_t stats;
    uint32_t i;
    assert(hybrid_audio_init() == 0);
    invalid_buffer_tests();
    hybrid_audio_note_on(60, 112);
    for (i = 0; i < 12000u; ++i) render_checked(blocks[i & 3u]);
    hybrid_audio_note_off(60);
    hybrid_audio_panic();
    for (i = 0; i < 1024u; ++i) render_checked(blocks[i & 3u]);
    hybrid_profile_snapshot(&stats);
    assert(stats.engine[HYBRID_PROF_AMB].calls > 0u);
    assert(stats.engine[HYBRID_PROF_DUB].calls > 0u);
    assert(stats.engine[HYBRID_PROF_ZEN].calls > 0u);
    assert(stats.engine[HYBRID_PROF_AMB].frames > 100000u);
    assert(stats.engine[HYBRID_PROF_DUB].frames > 100000u);
    assert(stats.engine[HYBRID_PROF_ZEN].frames > 100000u);
    assert(nonzero_samples > 1000u);
    puts("native DSP adapters: PASS (real AMB/DUB/ZEN symbols; render, note, panic, buffer guards, 13k blocks)");
    return 0;
}