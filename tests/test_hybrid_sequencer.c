/* SPDX-License-Identifier: GPL-3.0-only */
#include "hybrid_sequencer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef struct {uint32_t on,off,clock,start,stop,on_by_engine[4];uint8_t last_engine,last_note;} trace_t;
static void event(void*p,uint8_t engine,hybrid_seq_event_t e,uint8_t note,uint8_t velocity)
{
 trace_t*t=(trace_t*)p;(void)velocity;
 if(e==HYBRID_SEQ_NOTE_ON){t->on++;if(engine<4u)t->on_by_engine[engine]++;t->last_engine=engine;t->last_note=note;}
 else if(e==HYBRID_SEQ_NOTE_OFF)t->off++;
 else if(e==HYBRID_SEQ_CLOCK)t->clock++;
 else if(e==HYBRID_SEQ_START)t->start++;
 else if(e==HYBRID_SEQ_STOP)t->stop++;
}
int main(void)
{
 hybrid_seq_t s;trace_t t={0};uint8_t chain[3]={HYBRID_SEQ_BASS,HYBRID_SEQ_DRUM,HYBRID_SEQ_FM};
 hybrid_seq_init(&s,44100u,event,&t);
 assert(hybrid_seq_tempo(&s,120u)==0);assert(hybrid_seq_tempo(&s,240u)==0);assert(hybrid_seq_tempo(&s,241u)==-1);assert(hybrid_seq_tempo(&s,120u)==0);assert(hybrid_seq_tempo(&s,39u)==-1);
 assert(hybrid_seq_chain(&s,chain,3u)==0);assert(hybrid_seq_chain(&s,chain,0u)==-1);
 hybrid_seq_mute(&s,1u,1u);hybrid_seq_solo(&s,2u,1u);hybrid_seq_variation(&s,1u);hybrid_seq_fill(&s,1u);
 hybrid_seq_start(&s);assert(t.start==1u);
 /* Six MIDI clocks per 16th. One 16th at 120 BPM is 5512.5 samples. */
 for(unsigned i=0;i<700;i++)hybrid_seq_process(&s,128u);
 assert(t.clock>=90u);assert(t.on>0u);assert(t.on_by_engine[0]==0u&&t.on_by_engine[1]==0u&&t.on_by_engine[2]>0u&&t.on_by_engine[3]==0u);
 assert(s.chain_position==1u);
 hybrid_seq_stop(&s);assert(t.stop==1u);assert(!s.running);
 assert(t.off>0u);
 {
  hybrid_seq_t ext;trace_t e={0};uint8_t one[1]={HYBRID_SEQ_KEYS};
  hybrid_seq_init(&ext,44100u,event,&e);assert(hybrid_seq_chain(&ext,one,1u)==0);
  hybrid_seq_external_clock(&ext,1u);hybrid_seq_start(&ext);
  for(unsigned i=0;i<700;i++)hybrid_seq_process(&ext,128u);
  assert(e.clock==0u&&e.on==0u);
  for(unsigned i=0;i<6;i++)hybrid_seq_clock_pulse(&ext);
  assert(e.clock==6u&&e.on>0u);
  hybrid_seq_stop(&ext);
 }
 {
  hybrid_seq_t all;trace_t a={0};uint8_t one[1]={HYBRID_SEQ_BASS};
  hybrid_seq_init(&all,44100u,event,&a);assert(hybrid_seq_chain(&all,one,1u)==0);
  hybrid_seq_start(&all);
  for(unsigned i=0;i<700;i++)hybrid_seq_process(&all,128u);
  assert(a.on_by_engine[0]>0u&&a.on_by_engine[1]>0u&&a.on_by_engine[2]>0u&&a.on_by_engine[3]>0u);
  hybrid_seq_stop(&all);
 }
 puts("SMK-37 VOLCA shared-clock sequencer tests: PASS");
 return 0;
}
