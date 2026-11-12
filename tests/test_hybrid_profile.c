/* SPDX-License-Identifier: GPL-3.0-only */
#include "hybrid_profile.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
uint32_t test_fm1_ticks;
char _ustack_lo[4], _ustack_top[4], _sstack_lo[4], _sstack_top[4];
int main(void) {
 hybrid_profile_snapshot_t s;
 hybrid_profile_init();
 test_fm1_ticks=100u; hybrid_profile_begin(HYBRID_PROF_AMB);
 test_fm1_ticks=2500u; hybrid_profile_end(HYBRID_PROF_AMB,64u);
 test_fm1_ticks=2600u; hybrid_profile_begin(HYBRID_PROF_SAMPLE);
 test_fm1_ticks=3800u; hybrid_profile_end(HYBRID_PROF_SAMPLE,128u);
 hybrid_profile_buffers(512u,2048u); hybrid_profile_underrun();
 hybrid_profile_begin(HYBRID_PROF_SEQUENCER); test_fm1_ticks+=120u; hybrid_profile_end(HYBRID_PROF_SEQUENCER,128u);
 hybrid_profile_audio_block_begin(); test_fm1_ticks+=240u; hybrid_profile_audio_block_end();
 hybrid_profile_snapshot(&s);
 assert(s.engine[HYBRID_PROF_AMB].calls==1u);
 assert(s.engine[HYBRID_PROF_AMB].frames==64u);
 assert(s.engine[HYBRID_PROF_AMB].last_ticks==2400u);
 assert(s.engine[HYBRID_PROF_SAMPLE].max_ticks==1200u);
 assert(s.underruns==1u);
 assert(s.buffer_used_bytes==512u && s.buffer_capacity_bytes==2048u);
 assert(s.engine[HYBRID_PROF_SEQUENCER].calls==1u && s.engine[HYBRID_PROF_SEQUENCER].frames==128u);
 assert(s.engine[HYBRID_PROF_SEQUENCER].last_ticks==120u);
 assert(s.audio_blocks==1u && s.audio_max_ticks==240u);
 puts("SMK-37 HYBRID profiler host tests: PASS");
 return 0;
}
