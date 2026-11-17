/* SPDX-License-Identifier: GPL-3.0-only
 * Native WL82/AC79 ALNK0 audio backend for SMK-37 Pro.
 */
#ifndef HYBRID_AC79_AUDIO_H
#define HYBRID_AC79_AUDIO_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define HYBRID_AC79_AUDIO_RATE 44118u
#define HYBRID_AC79_AUDIO_FRAMES 128u
#define HYBRID_AC79_AUDIO_CHANNELS 2u

typedef struct {
    uint32_t (*clock_hz)(void *context);
    uint32_t (*ticks)(void *context);
    int (*codec_init)(void *context);
    int (*irq_attach)(void *context, unsigned irq, void (*handler)(void), unsigned priority);
    void *context;
} hybrid_ac79_audio_hw_t;

typedef int (*hybrid_ac79_render_fn)(void *context, int32_t *stereo_q15, uint16_t frames);
typedef struct {
    uint32_t blocks;
    uint32_t underruns;
    uint32_t late_commits;
    uint32_t render_errors;
    uint32_t peak_ticks;
    uint32_t last_ticks;
    uint32_t interval_errors;
} hybrid_ac79_audio_stats_t;

int hybrid_ac79_audio_init(const hybrid_ac79_audio_hw_t *hardware,
                           hybrid_ac79_render_fn render, void *render_context);
void hybrid_ac79_audio_irq(void);
void hybrid_ac79_audio_get_stats(hybrid_ac79_audio_stats_t *out);
void hybrid_ac79_audio_mute(uint8_t mute);
#ifdef __cplusplus
}
#endif
#endif
