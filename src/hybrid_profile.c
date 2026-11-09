/* SPDX-License-Identifier: GPL-3.0-only
 * AC79 profiler: TIMER4 24 MHz; no heap or formatted I/O in audio ISR.
 */
#include <stdint.h>
#include "hybrid_profile.h"
#include "fm1_time.h"
extern char _ustack_lo[], _ustack_top[], _sstack_lo[], _sstack_top[];
static volatile hybrid_profile_snapshot_t stats;
static uint32_t started[HYBRID_PROF_COUNT];
static uint8_t active[HYBRID_PROF_COUNT];
static void sample_stack(void) {
 volatile uint8_t marker=0;
 uintptr_t p=(uintptr_t)&marker, lo=(uintptr_t)_ustack_lo, top=(uintptr_t)_ustack_top;
 uint32_t used=0, capacity=0;
 (void)marker;
 if(p>=lo && p<top) { used=(uint32_t)(top-p); capacity=(uint32_t)(top-lo); }
 else { lo=(uintptr_t)_sstack_lo; top=(uintptr_t)_sstack_top;
  if(p>=lo && p<top) { used=(uint32_t)(top-p); capacity=(uint32_t)(top-lo); } }
 if(capacity) { stats.stack_used_bytes=used; stats.stack_capacity_bytes=capacity;
  if(used>stats.stack_peak_bytes) stats.stack_peak_bytes=used;
  stats.flags|=HYBRID_PROFILE_STACK_VALID; }
}
void hybrid_profile_init(void) {
 uint32_t i; volatile uint32_t *p=(volatile uint32_t *)&stats;
 for(i=0;i<(uint32_t)(sizeof(stats)/sizeof(uint32_t));++i) p[i]=0;
 for(i=0;i<HYBRID_PROF_COUNT;++i) { started[i]=0; active[i]=0; }
 sample_stack();
}
void hybrid_profile_begin(hybrid_profile_id_t id) {
 if((uint32_t)id>=HYBRID_PROF_COUNT || active[id]) return;
 started[id]=fm1_ticks(); active[id]=1;
}
void hybrid_profile_end(hybrid_profile_id_t id,uint32_t frames) {
 uint32_t elapsed; volatile hybrid_profile_engine_stats_t *s;
 if((uint32_t)id>=HYBRID_PROF_COUNT || !active[id]) return;
 elapsed=fm1_ticks()-started[id]; active[id]=0; s=&stats.engine[id];
 s->calls++; s->frames+=frames; s->last_ticks=elapsed; s->total_ticks+=elapsed;
 if(elapsed>s->max_ticks) s->max_ticks=elapsed;
 if(id==HYBRID_PROF_MIXER) { stats.audio_blocks++; if(elapsed>stats.audio_max_ticks) stats.audio_max_ticks=elapsed; }
 sample_stack();
}
void hybrid_profile_audio_block_begin(void) { hybrid_profile_begin(HYBRID_PROF_MIXER); }
void hybrid_profile_audio_block_end(void) { hybrid_profile_end(HYBRID_PROF_MIXER,0); }
void hybrid_profile_underrun(void) { stats.underruns++; }
void hybrid_profile_buffers(uint32_t used,uint32_t capacity) {
 stats.buffer_used_bytes=used; stats.buffer_capacity_bytes=capacity;
 if(used>stats.buffer_peak_bytes) stats.buffer_peak_bytes=used;
 if(capacity) stats.flags|=HYBRID_PROFILE_BUFFER_VALID;
}
void hybrid_profile_snapshot(hybrid_profile_snapshot_t *out) {
 uint32_t i; if(!out) return;
 for(i=0;i<(uint32_t)(sizeof(*out)/sizeof(uint32_t));++i)
  ((uint32_t *)out)[i]=((volatile uint32_t *)&stats)[i];
}
