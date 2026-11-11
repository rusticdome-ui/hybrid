/* SPDX-License-Identifier: GPL-3.0-only
 * SMK-37 VOLCA HYBRID: bounded four-lane, shared-clock sequencer.
 * No heap allocation. Factory phrases are immutable and remain in XIP/rodata.
 */
#ifndef SMK37_HYBRID_SEQUENCER_H
#define SMK37_HYBRID_SEQUENCER_H
#include <stdint.h>
#define HYBRID_SEQ_TRACKS 4u
#define HYBRID_SEQ_STEPS 16u
#define HYBRID_SEQ_CHAIN_MAX 16u
#define HYBRID_SEQ_FAMILIES 5u
typedef enum {
    HYBRID_SEQ_BASS=0, HYBRID_SEQ_KEYS=1, HYBRID_SEQ_DRUM=2,
    HYBRID_SEQ_FM=3, HYBRID_SEQ_SAMPLE=4
} hybrid_seq_family_t;
typedef enum {
    HYBRID_SEQ_NOTE_OFF=0, HYBRID_SEQ_NOTE_ON=1,
    HYBRID_SEQ_CLOCK=2, HYBRID_SEQ_START=3, HYBRID_SEQ_STOP=4
} hybrid_seq_event_t;
/* engine is 0..3 for the four HYBRID slots; 0xff denotes global transport. */
typedef void (*hybrid_seq_event_fn)(void *context, uint8_t engine,
                                    hybrid_seq_event_t event,
                                    uint8_t note, uint8_t velocity);
typedef struct {
    uint32_t sample_rate, step_q16, clock_q16, phase_q16, clock_phase_q16;
    uint16_t bpm;
    uint8_t chain[HYBRID_SEQ_CHAIN_MAX], chain_length, chain_position;
    uint8_t step, running, variation_b, fill;
    uint8_t mute_mask, solo_mask, active_note[HYBRID_SEQ_TRACKS];
    hybrid_seq_event_fn emit;
    void *context;
} hybrid_seq_t;
void hybrid_seq_init(hybrid_seq_t *seq, uint32_t sample_rate,
                     hybrid_seq_event_fn emit, void *context);
int hybrid_seq_tempo(hybrid_seq_t *seq, uint16_t bpm);
int hybrid_seq_chain(hybrid_seq_t *seq, const uint8_t *families, uint8_t length);
void hybrid_seq_variation(hybrid_seq_t *seq, uint8_t variation_b);
void hybrid_seq_fill(hybrid_seq_t *seq, uint8_t enabled);
void hybrid_seq_mute(hybrid_seq_t *seq, uint8_t track, uint8_t muted);
void hybrid_seq_solo(hybrid_seq_t *seq, uint8_t track, uint8_t solo);
void hybrid_seq_start(hybrid_seq_t *seq);
void hybrid_seq_stop(hybrid_seq_t *seq);
void hybrid_seq_process(hybrid_seq_t *seq, uint16_t frames);
#endif
