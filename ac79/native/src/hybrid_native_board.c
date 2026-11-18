/* SPDX-License-Identifier: GPL-3.0-only
 * Minimal native board entry. Clock/PLL/XIP are supplied by the documented
 * m-bootloader hand-off; no Felucca setup or audio_init() is called here.
 */
#include <stdint.h>
#include "hybrid_native_irq.h"
#include "hybrid_ac79_audio.h"
extern int hybrid_audio_init(void);
extern void *hybrid_audio_render_context(void);
/* Native USB-MIDI transport is not implemented yet. */
__attribute__((weak))
void hybrid_midi_out_event(uint32_t packet) { (void)packet; }
int hybrid_native_early_board_init(void)
{
    if (hybrid_ac79_timer_init() != 0) return -1;
    return hybrid_native_irq_init();
}
int hybrid_native_main(void)
{
    void *renderer;
    if (hybrid_audio_init() != 0) return -1;
    renderer = hybrid_audio_render_context();
    if (!renderer) return -2;
    if (hybrid_ac79_audio_startup(renderer) != 0) return -3;
    for (;;) __asm__ volatile("idle");
}
