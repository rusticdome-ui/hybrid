/* SPDX-License-Identifier: GPL-3.0-only
 * WL82/AC79 ALNK0 board adapter.
 *
 * Standalone builds route through the HYBRID-owned IRQ table. The legacy
 * validation build retains the previous direct WL82 vector/ICFG adapter so
 * this file remains buildable while Felucca is still used as a reference.
 * TIMER4 is configured as the 24 MHz crystal counter before ALNK0 starts.
 */
#include <stdint.h>
#include "hybrid_ac79_audio.h"
#include "hybrid_ac79_wl82_map.h"
#if defined(HYBRID_NATIVE_STANDALONE)
#include "hybrid_native_irq.h"
#else
#define FM1_VEC  ((volatile uint32_t *)0x01c7fe00u)
#define FM1_ICFG ((volatile uint32_t *)0x01eef100u)
#endif
#define BIT(n) (1u << (n))
#define TIMER4_CON (*(volatile uint32_t *)0x00010800u)
#define TIMER4_COUNT (*(volatile uint32_t *)0x00010804u)
#define TIMER4_PERIOD (*(volatile uint32_t *)0x00010808u)
#define PORTC ((volatile uint32_t *)HYBRID_WL82_PORTC_BASE)
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
#if defined(HYBRID_NATIVE_STANDALONE)
    return hybrid_native_irq_attach(irq,handler,priority);
#else
    uint32_t shift,config,gates=0x300u;
    if(irq>=128u||!handler||priority>7u)return -1;
    shift=(irq&7u)*4u;
    config=((priority<<1)|1u)<<shift;
    FM1_VEC[irq]=(uint32_t)(uintptr_t)handler;
    FM1_ICFG[irq>>3]=(FM1_ICFG[irq>>3]&~(0x0fu<<shift))|config;
    __asm__ volatile("csync\n\ticfg = %0\n\tcsync"::"r"(gates):"memory");
    return 0;
#endif
}
int hybrid_ac79_audio_startup(void *renderer_context)
{
    static const hybrid_ac79_audio_hw_t hardware={
        board_clock_hz,board_ticks,board_irq_save,board_irq_restore,
        board_dac_enable,board_irq_attach,0
    };
    if(!renderer_context)return -1;
    if(hybrid_ac79_timer_init()!=0)return -2;
    return hybrid_ac79_audio_init(&hardware,hybrid_ac79_audio_render_core,renderer_context);
}
