/* SPDX-License-Identifier: GPL-3.0-only
 * HYBRID native audio bridge. The only native instrument engine is the
 * independent dual-oscillator wavetable synthesizer; FM6/DX7 remains hosted
 * by its own renderer and enters the common mixer as PCM.
 */
#include <stdint.h>
#include "hybrid_core.h"
#include "hybrid_wavetable.h"
#include "hybrid_wavetable_pro.h"
#include "hybrid_profile.h"
#include "hybrid_sequencer.h"
static hybrid_core_t hybrid_audio_core;
static hybrid_wavetable_t hybrid_wavetable_state;
static int32_t hybrid_audio_mix[HYBRID_MAX_FRAMES * 2u];
static uint8_t hybrid_audio_ready;
static hybrid_seq_t hybrid_audio_sequencer;
static uint8_t hybrid_audio_seq_synced;
static uint16_t hybrid_audio_seq_bpm;
static uint8_t hybrid_audio_seq_external;
static void hybrid_seq_midi_out(uint32_t status,uint32_t data1,uint32_t data2)
{
    extern void hybrid_midi_out_event(uint32_t);
    uint32_t s=status&0xFFu;
    hybrid_midi_out_event(((s>>4)&15u)|(s<<8)|((data1&127u)<<16)|((data2&127u)<<24));
}
static void hybrid_seq_emit(void *context, uint8_t engine, hybrid_seq_event_t event, uint8_t note, uint8_t velocity)
{
    hybrid_core_t *core = (hybrid_core_t *)context;
    if (event == HYBRID_SEQ_CLOCK) { if (!hybrid_audio_seq_external) hybrid_seq_midi_out(0xF8u, 0u, 0u); return; }
    if (event == HYBRID_SEQ_START) { if (!hybrid_audio_seq_external) hybrid_seq_midi_out(0xFAu, 0u, 0u); return; }
    if (event == HYBRID_SEQ_STOP) { if (!hybrid_audio_seq_external) hybrid_seq_midi_out(0xFCu, 0u, 0u); return; }
    if (!core || engine >= HYBRID_CHANNELS) return;
    if (event == HYBRID_SEQ_NOTE_ON && core->engine[engine].ops && core->engine[engine].ops->note_on)
        core->engine[engine].ops->note_on(core->engine[engine].state, note, velocity);
    else if (event == HYBRID_SEQ_NOTE_OFF && core->engine[engine].ops && core->engine[engine].ops->note_off)
        core->engine[engine].ops->note_off(core->engine[engine].state, note);
}

void hybrid_audio_seq_process(uint32_t frames)
{
    if (!hybrid_audio_ready || !frames || frames > HYBRID_MAX_FRAMES) return;
    hybrid_profile_begin(HYBRID_PROF_SEQUENCER);
    hybrid_seq_process(&hybrid_audio_sequencer, (uint16_t)frames);
    hybrid_profile_end(HYBRID_PROF_SEQUENCER, frames);
}
int hybrid_audio_seq_tempo(uint16_t bpm) { return hybrid_seq_tempo(&hybrid_audio_sequencer, bpm); }
int hybrid_audio_seq_chain(const uint8_t *families, uint8_t length) { return hybrid_seq_chain(&hybrid_audio_sequencer, families, length); }
void hybrid_audio_seq_start(void) { hybrid_seq_start(&hybrid_audio_sequencer); }
void hybrid_audio_seq_stop(void) { hybrid_seq_stop(&hybrid_audio_sequencer); }
void hybrid_audio_seq_sync(uint8_t playing, uint16_t bpm, uint8_t external_clock)
{
    if (bpm < 40u) bpm = 40u;
    if (bpm > 240u) bpm = 240u;
    hybrid_audio_seq_external = (uint8_t)!!external_clock;
    hybrid_seq_external_clock(&hybrid_audio_sequencer, hybrid_audio_seq_external);
    if (bpm != hybrid_audio_seq_bpm) {
        (void)hybrid_seq_tempo(&hybrid_audio_sequencer, bpm);
        hybrid_audio_seq_bpm = bpm;
    }
    if (playing && !hybrid_audio_seq_synced) hybrid_seq_start(&hybrid_audio_sequencer);
    else if (!playing && hybrid_audio_seq_synced) hybrid_seq_stop(&hybrid_audio_sequencer);
    hybrid_audio_seq_synced = (uint8_t)!!playing;
}
void hybrid_audio_seq_clock_pulse(void)
{
    if (hybrid_audio_ready) {
        hybrid_profile_begin(HYBRID_PROF_SEQUENCER);
        hybrid_seq_clock_pulse(&hybrid_audio_sequencer);
        hybrid_profile_end(HYBRID_PROF_SEQUENCER, 0u);
    }
}
void hybrid_audio_seq_variation(uint8_t value) { hybrid_seq_variation(&hybrid_audio_sequencer, value); }
void hybrid_audio_seq_fill(uint8_t value) { hybrid_seq_fill(&hybrid_audio_sequencer, value); }
void hybrid_audio_seq_mute(uint8_t track, uint8_t value) { hybrid_seq_mute(&hybrid_audio_sequencer, track, value); }
void hybrid_audio_seq_solo(uint8_t track, uint8_t value) { hybrid_seq_solo(&hybrid_audio_sequencer, track, value); }

int hybrid_audio_init(void)
{
    hybrid_profile_init();
    hybrid_seq_init(&hybrid_audio_sequencer, 44118u, hybrid_seq_emit, &hybrid_audio_core); /* AC79 I2S: 24 MHz / 544 ticks */
    hybrid_audio_seq_synced = 0u; hybrid_audio_seq_bpm = 120u;
    hybrid_profile_buffers(0u, (uint32_t)sizeof(hybrid_audio_mix) + (uint32_t)sizeof(hybrid_wavetable_state));
    hybrid_init_defaults(&hybrid_audio_core);
    if (hybrid_register_wavetable_engine(&hybrid_audio_core, &hybrid_wavetable_state) != 0)
        return -1;
    if (hybrid_start(&hybrid_audio_core) != 0)
        return -1;
    hybrid_audio_ready = 1;
    return 0;
}

void hybrid_audio_process(int32_t *out_q15, uint32_t frames, int32_t master_q12)
{
    uint32_t i;
    if (!hybrid_audio_ready || !out_q15 || !frames ||
        frames > HYBRID_MAX_FRAMES || master_q12 <= 0)
        return;
    hybrid_profile_buffers(frames * 2u * (uint32_t)sizeof(int32_t),
                           (uint32_t)sizeof(hybrid_audio_mix) + (uint32_t)sizeof(hybrid_wavetable_state));
    if (hybrid_process(&hybrid_audio_core, hybrid_audio_mix, (uint16_t)frames) != 0) {
        return;
    }
    for (i = 0; i < frames * 2u; ++i) {
        int64_t v = (int64_t)out_q15[i] +
                    ((int64_t)hybrid_audio_mix[i] * master_q12 >> 12);
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        out_q15[i] = (int32_t)v;
    }
}

void hybrid_audio_note_on(uint8_t note, uint8_t velocity)
{
    if (hybrid_audio_ready)
        hybrid_note_on(&hybrid_audio_core, note, velocity, 0);
}

void hybrid_audio_note_off(uint8_t note)
{
    if (hybrid_audio_ready)
        hybrid_note_off(&hybrid_audio_core, note, 0);
}

void hybrid_audio_panic(void)
{
    if (hybrid_audio_ready)
        hybrid_panic(&hybrid_audio_core);
}

void hybrid_audio_midi_note_on(uint8_t note, uint8_t velocity, uint8_t channel)
{
    if (hybrid_audio_ready)
        hybrid_note_on(&hybrid_audio_core, note, velocity, channel);
}

void hybrid_audio_midi_note_off(uint8_t note, uint8_t channel)
{
    if (hybrid_audio_ready)
        hybrid_note_off(&hybrid_audio_core, note, channel);
}

void hybrid_audio_midi_panic(uint8_t channel)
{
    uint32_t note;
    if (!hybrid_audio_ready)
        return;
    for (note = 0; note < 128u; ++note)
        hybrid_note_off(&hybrid_audio_core, (uint8_t)note, channel);
}

/* PRO operations are control-thread APIs; never call import/preset copies from the audio ISR. */
int hybrid_audio_wavetable_import(uint8_t slot,const int16_t *pcm,uint16_t count)
{
    if (!hybrid_audio_ready) return -1;
    return hybrid_wavetable_import(&hybrid_wavetable_state,slot,pcm,count);
}
int hybrid_audio_wavetable_clear(uint8_t slot)
{
    if (!hybrid_audio_ready) return -1;
    return hybrid_wavetable_clear_custom(&hybrid_wavetable_state,slot);
}
int hybrid_audio_wavetable_mod_route(uint8_t slot,uint8_t source,uint8_t destination,int16_t depth_q15)
{
    if (!hybrid_audio_ready || slot>=HYBRID_WT_MOD_SLOTS ||
        source>HYBRID_WT_MOD_ENVELOPE || destination>HYBRID_WT_DEST_AMP) return -1;
    hybrid_wavetable_state.mod_source[slot]=source;
    hybrid_wavetable_state.mod_destination[slot]=destination;
    hybrid_wavetable_state.mod_depth_q15[slot]=depth_q15;
    return 0;
}
int hybrid_audio_wavetable_preset_save(hybrid_wt_preset_t *out)
{
    if (!hybrid_audio_ready || !out) return -1;
    return hybrid_wavetable_preset_save(&hybrid_wavetable_state,0,out);
}
int hybrid_audio_wavetable_preset_load(const hybrid_wt_preset_t *preset)
{
    if (!hybrid_audio_ready || !preset) return -1;
    return hybrid_wavetable_preset_load(&hybrid_wavetable_state,0,preset);
}
