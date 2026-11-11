/* SPDX-License-Identifier: GPL-3.0-only */
#include "hybrid_sequencer.h"
#define NO_NOTE 0u
/* Compact immutable 16-step phrases. Zero is a rest; no sample data or DSP is embedded. */
static const uint8_t notes[HYBRID_SEQ_FAMILIES][HYBRID_SEQ_STEPS] = {
 {36,36,43,36,0,36,46,36,36,0,48,43,36,36,0,34},       /* Bass */
 {60,0,64,67,0,64,67,71,60,0,64,67,0,65,69,72},       /* Keys */
 {36,42,42,42,38,42,36,42,36,42,42,42,38,42,46,42},  /* Drum */
 {48,55,60,64,67,64,60,55,50,57,62,65,69,65,62,57},  /* FM */
 {36,0,38,0,42,42,0,46,36,0,40,0,42,0,46,0}           /* Sample triggers */
};
static const uint8_t velocity[HYBRID_SEQ_FAMILIES][HYBRID_SEQ_STEPS] = {
 {112,72,96,72,0,96,80,72,104,0,92,88,104,72,0,100},
 {96,0,78,92,0,78,92,100,96,0,78,92,0,78,92,104},
 {118,0,76,0,110,0,84,0,118,0,76,0,110,0,96,0},
 {88,72,78,84,96,84,78,72,80,72,78,84,96,84,78,72},
 {108,0,76,0,92,88,0,100,108,0,76,0,92,0,100,0}
};
static uint32_t period_q16(uint32_t sample_rate, uint16_t bpm)
{
    uint32_t n = sample_rate * 15u;
    uint32_t d = (uint32_t)bpm;
    uint32_t whole = n / d, rem = n % d;
    return (whole << 16) + ((rem << 16) / d);
}
static void emit(hybrid_seq_t *s, uint8_t engine, hybrid_seq_event_t e,
                 uint8_t note, uint8_t velocity_value)
{
    if (s->emit) s->emit(s->context, engine, e, note, velocity_value);
}
void hybrid_seq_init(hybrid_seq_t *s, uint32_t rate, hybrid_seq_event_fn fn, void *ctx)
{
    uint8_t i;
    if (!s) return;
    s->sample_rate = rate ? rate : 44100u;
    s->emit = fn; s->context = ctx; s->bpm = 120u;
    s->chain_length = 1u; s->chain[0] = HYBRID_SEQ_BASS;
    s->step = 0u; s->clock_divider=0u; s->running = 0u; s->variation_b = 0u; s->fill = 0u; s->external_clock=0u;
    s->mute_mask = 0u; s->solo_mask = 0u; s->clock_phase_q16 = 0u;
    s->chain_position = 0u;
    for (i=0;i<HYBRID_SEQ_TRACKS;i++) s->active_note[i]=NO_NOTE;
    (void)hybrid_seq_tempo(s,120u);
}
int hybrid_seq_tempo(hybrid_seq_t *s, uint16_t bpm)
{
    if (!s || bpm < 40u || bpm > 240u) return -1;
    s->bpm=bpm;
    /* One 16th note = 1/4 beat; one MIDI clock = 1/24 beat. */
    s->step_q16=period_q16(s->sample_rate,bpm);
    s->clock_q16=s->step_q16/6u;
    if (!s->clock_q16) s->clock_q16=1u;
    return 0;
}
int hybrid_seq_chain(hybrid_seq_t *s,const uint8_t *f,uint8_t n)
{
    uint8_t i;
    if(!s||!f||!n||n>HYBRID_SEQ_CHAIN_MAX)return -1;
    for(i=0;i<n;i++)if(f[i]>=HYBRID_SEQ_FAMILIES)return -2;
    for(i=0;i<n;i++)s->chain[i]=f[i];
    s->chain_length=n;s->chain_position=0u;
    return 0;
}
void hybrid_seq_variation(hybrid_seq_t*s,uint8_t v){if(s)s->variation_b=(uint8_t)!!v;}
static void step_event(hybrid_seq_t *s);
void hybrid_seq_fill(hybrid_seq_t*s,uint8_t v){if(s)s->fill=(uint8_t)!!v;}
void hybrid_seq_external_clock(hybrid_seq_t*s,uint8_t v){if(s)s->external_clock=(uint8_t)!!v;}
void hybrid_seq_clock_pulse(hybrid_seq_t*s)
{
    if(!s||!s->running||!s->external_clock)return;
    emit(s,0xffu,HYBRID_SEQ_CLOCK,0u,0u);
    if(++s->clock_divider>=6u){s->clock_divider=0u;step_event(s);}
}
void hybrid_seq_mute(hybrid_seq_t*s,uint8_t t,uint8_t v){if(s&&t<HYBRID_SEQ_TRACKS){if(v)s->mute_mask|=(uint8_t)(1u<<t);else s->mute_mask&=(uint8_t)~(1u<<t);}}
void hybrid_seq_solo(hybrid_seq_t*s,uint8_t t,uint8_t v){if(s&&t<HYBRID_SEQ_TRACKS){if(v)s->solo_mask|=(uint8_t)(1u<<t);else s->solo_mask&=(uint8_t)~(1u<<t);}}
static void step_event(hybrid_seq_t*s)
{
    uint8_t t,f=s->chain[s->chain_position],solo=s->solo_mask,step=s->step;
    for(t=0;t<HYBRID_SEQ_TRACKS;t++){
        uint8_t bit=(uint8_t)(1u<<t),n=notes[f][step],v=velocity[f][step];
        if(s->active_note[t]){emit(s,t,HYBRID_SEQ_NOTE_OFF,s->active_note[t],0u);s->active_note[t]=0u;}
        if((s->mute_mask&bit)||(solo&&!(solo&bit)))continue;
        /* Variation B adds syncopation and octave movement without allocating a second pattern bank. */
        if(s->variation_b){if((step%4u)==2u&&n)n=0u;else if(n&&f!=HYBRID_SEQ_DRUM&&f!=HYBRID_SEQ_SAMPLE)n=(uint8_t)(n+12u);}
        /* A fill decorates the final four steps; it never changes the stored phrase. */
        if(s->fill&&step>=12u&&((step+t)&1u)==0u){n=(uint8_t)(f==HYBRID_SEQ_DRUM?38u:(n?n:48u));v=120u;}
        if(n){uint8_t transpose=(f==HYBRID_SEQ_DRUM||f==HYBRID_SEQ_SAMPLE)?0u:(uint8_t)(t*12u);uint16_t nn=(uint16_t)n+transpose;if(nn>127u)nn=127u;emit(s,t,HYBRID_SEQ_NOTE_ON,(uint8_t)nn,v);s->active_note[t]=(uint8_t)nn;}
    }
    s->step=(uint8_t)((s->step+1u)&15u);
    if(s->step==0u)s->chain_position=(uint8_t)((s->chain_position+1u)%s->chain_length);
}
void hybrid_seq_start(hybrid_seq_t*s)
{
    uint8_t t;if(!s)return;
    for(t=0;t<HYBRID_SEQ_TRACKS;t++)if(s->active_note[t]){emit(s,t,HYBRID_SEQ_NOTE_OFF,s->active_note[t],0u);s->active_note[t]=0u;}
    s->clock_phase_q16=0u;s->step=0u;s->clock_divider=0u;s->chain_position=0u;s->running=1u;
    emit(s,0xffu,HYBRID_SEQ_START,0u,0u);
}
void hybrid_seq_stop(hybrid_seq_t*s)
{
    uint8_t t;if(!s)return;
    for(t=0;t<HYBRID_SEQ_TRACKS;t++)if(s->active_note[t]){emit(s,t,HYBRID_SEQ_NOTE_OFF,s->active_note[t],0u);s->active_note[t]=0u;}
    s->running=0u;s->phase_q16=0u;s->clock_phase_q16=0u;s->clock_divider=0u;
    emit(s,0xffu,HYBRID_SEQ_STOP,0u,0u);
}
void hybrid_seq_process(hybrid_seq_t*s,uint16_t frames)
{
    uint32_t add;
    if(!s||!s->running||s->external_clock||!frames)return;
    add=(uint32_t)frames<<16;
    s->clock_phase_q16+=add;
    while(s->clock_phase_q16>=s->clock_q16){
        s->clock_phase_q16-=s->clock_q16;
        emit(s,0xffu,HYBRID_SEQ_CLOCK,0u,0u);
        /* Every six MIDI clocks is one 16th-note step. */
        if(++s->clock_divider>=6u){s->clock_divider=0u;step_event(s);}
    }
}
