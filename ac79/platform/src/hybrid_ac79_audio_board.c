/* SPDX-License-Identifier: GPL-3.0-only
 * Direct WL82/AC79 board adapter for the native ALNK0 renderer.
 *
 * The SMK-37 Pro reference board routes ALNK0 directly to its onboard DAC:
 * PC0/1/2 carry MCLK/SCLK/LRCLK and PC6 is DAC enable. There is no external
 * codec-control bus in the reference audio bring-up. The 24 MHz timer clock,
 * IRQ 11 and timer registers follow the public SMK-37 Pro board definition.
 */
#include <stdint.h>
#include "hybrid_ac79_audio.h"
#include "hybrid_ac79_wl82_map.h"

#define BIT(n) (1u << (n))
#define AUDIO_TIMER_COUNT (*(volatile uint32_t *)0x1eef0f0u)
#define AUDIO_TIMER_PERIOD (*(volatile uint32_t *)0x1eef0f4u)
#define PORTC ((volatile uint32_t *)HYBRID_WL82_PORTC_BASE)
#define FM1_VEC ((volatile uint32_t *)0x01c7fe00u)
#define FM1_ICFG(n) (*(volatile uint32_t *)(0x1eef100u + 4u * ((n) >> 3)))
#define AUDIO_IRQ 11u
#define AUDIO_PRIORITY 4u
#define AUDIO_TIMER_HZ 24000000u

static uint32_t board_clock_hz(void *context)
{
    (void)context;
    return AUDIO_TIMER_HZ;
}
static uint32_t board_ticks(void *context)
{
    (void)context;
    return AUDIO_TIMER_COUNT;
}
static uint32_t board_irq_save(void *context)
{
    uint32_t flags, masked;
    (void)context;
    __asm__ volatile("%0 = icfg" : "=r"(flags) :: "memory");
    masked = flags & ~(1u << 8);
    __asm__ volatile("icfg = %0" :: "r"(masked) : "memory");
    return flags;
}
static void board_irq_restore(void *context, uint32_t flags)
{
    (void)context;
    __asm__ volatile("icfg = %0" :: "r"(flags) : "memory");
}
static int board_dac_enable(void *context)
{
    (void)context;
    /* Integrated DAC enable on PC6; ALNK0 clocks and pin mux are configured
       by hybrid_ac79_audio_init immediately before this callback. */
    PORTC[0] |= BIT(HYBRID_CODEC_ENABLE_PC6);
    return 0;
}
static int board_irq_attach(void *context, unsigned irq,
                            void (*handler)(void), unsigned priority)
{
    uint32_t shift;
    (void)context;
    if (!handler || irq >= 128u || priority > 7u) return -1;
    shift = (irq & 7u) * 4u;
    FM1_VEC[irq] = (uint32_t)(uintptr_t)handler;
    FM1_ICFG(irq) = (FM1_ICFG(irq) & ~(0x0fu << shift)) |
                    ((((priority & 7u) << 1) | 1u) << shift);
    return 0;
}
int hybrid_ac79_audio_startup(void *renderer_context)
{
    static const hybrid_ac79_audio_hw_t hardware = {
        board_clock_hz, board_ticks, board_irq_save, board_irq_restore,
        board_dac_enable, board_irq_attach, 0
    };
    if (!renderer_context) return -1;
    return hybrid_ac79_audio_init(&hardware, hybrid_ac79_audio_render_core,
                                  renderer_context);
}
