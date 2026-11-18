/* SPDX-License-Identifier: GPL-3.0-only
 * Native WL82/AC79 board adapter for ALNK0, DMA and TIMER4.
 * TIMER4 is the 24 MHz crystal counter used by the hardware-tested pi32v2
 * application example. IRQ routing is owned by the HYBRID native vector
 * backend; no Felucca vector array or audio_init() is referenced.
 */
#include <stdint.h>
#include "hybrid_ac79_audio.h"
#include "hybrid_ac79_wl82_map.h"
#include "hybrid_native_irq.h"
#define BIT(n) (1u << (n))
#define TIMER4_CON (*(volatile uint32_t *)0x00010800u)
#define TIMER4_COUNT (*(volatile uint32_t *)0x00010804u)
#define TIMER4_PERIOD (*(volatile uint32_t *)0x00010808u)
#define PORTC ((volatile uint32_t *)HYBRID_WL82_PORTC_BASE)
#define AUDIO_IRQ 11u
#define AUDIO_PRIORITY 4u
#define AUDIO_TIMER_HZ 24000000u

int hybrid_ac79_timer_init(void)
{
    TIMER4_CON=0u;
    TIMER4_PERIOD=0xffffffffu;
    TIMER4_COUNT=0u;
    TIMER4_CON=0x4009u;
    return 0;
}
uint32_t hybrid_ac79_timer_ticks(void) { return TIMER4_COUNT; }

static uint32_t board_clock_hz(void *context)
{
    (void)context;
    return AUDIO_TIMER_HZ;
}
static uint32_t board_ticks(void *context)
{
    (void)context;
    return hybrid_ac79_timer_ticks();
}
static uint32_t board_irq_save(void *context)
{
    uint32_t flags,masked;
    (void)context;
    __asm__ volatile("%0 = icfg" : "=r"(flags) :: "memory");
    masked=flags&~(1u<<8);
    __asm__ volatile("icfg = %0" :: "r"(masked) : "memory");
    return flags;
}
static void board_irq_restore(void *context,uint32_t flags)
{
    (void)context;
    __asm__ volatile("icfg = %0" :: "r"(flags) : "memory");
}
static int board_dac_enable(void *context)
{
    (void)context;
    PORTC[0]|=BIT(6);
    return 0;
}
static int board_irq_attach(void *context,unsigned irq,void (*handler)(void),unsigned priority)
{
    (void)context;
    return hybrid_native_irq_attach(irq,handler,priority);
}
int hybrid_ac79_audio_startup(void *renderer_context)
{
    static const hybrid_ac79_audio_hw_t hardware={
        board_clock_hz,board_ticks,board_irq_save,board_irq_restore,
        board_dac_enable,board_irq_attach,0
    };
    if(!renderer_context)return -1;
    return hybrid_ac79_audio_init(&hardware,hybrid_ac79_audio_render_core,renderer_context);
}
