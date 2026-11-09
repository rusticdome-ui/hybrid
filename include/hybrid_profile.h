/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef SMK37_HYBRID_PROFILE_H
#define SMK37_HYBRID_PROFILE_H
#include <stdint.h>
typedef enum { HYBRID_PROF_AMB=0, HYBRID_PROF_DUB=1, HYBRID_PROF_ZEN=2, HYBRID_PROF_GHOULBOX=3, HYBRID_PROF_MIXER=4, HYBRID_PROF_COUNT=5 } hybrid_profile_id_t;
typedef struct { uint32_t calls, frames, total_ticks, max_ticks, last_ticks, overruns; } hybrid_profile_engine_stats_t;
typedef struct {
 hybrid_profile_engine_stats_t engine[HYBRID_PROF_COUNT];
 uint32_t underruns, audio_blocks, audio_max_ticks;
 uint32_t stack_used_bytes, stack_capacity_bytes, stack_peak_bytes;
 uint32_t buffer_used_bytes, buffer_capacity_bytes, buffer_peak_bytes, flags;
} hybrid_profile_snapshot_t;
enum { HYBRID_PROFILE_STACK_VALID=1u, HYBRID_PROFILE_BUFFER_VALID=2u };
extern volatile hybrid_profile_snapshot_t hybrid_profile_live;
void hybrid_profile_init(void);
void hybrid_profile_begin(hybrid_profile_id_t id);
void hybrid_profile_end(hybrid_profile_id_t id, uint32_t frames);
void hybrid_profile_audio_block_begin(void);
void hybrid_profile_audio_block_end(void);
void hybrid_profile_underrun(void);
void hybrid_profile_buffers(uint32_t used_bytes, uint32_t capacity_bytes);
void hybrid_profile_snapshot(hybrid_profile_snapshot_t *out);
#endif
