/* SPDX-License-Identifier: GPL-3.0-only
 * Native WL82/AC79 ALNK0 + DMA backend for SMK-37 Pro.
 * PCM is interleaved signed Q15 stereo, promoted to signed Q23 DMA slots.
 */
#include <stdint.h>
#include "hybrid_ac79_audio.h"
#include "hybrid_ac79_wl82_map.h"
#include "hybrid_core.h"
#define BIT(n) (1u << (n))
#define AUDIO_IRQ 11u
#define AUDIO_PRIORITY 4u
#define AUDIO_WORDS (HYBRID_AC79_AUDIO_FRAMES*HYBRID_AC79_AUDIO_CHANNELS)
#define TIMER_COUNT (*(volatile uint32_t *)0x00010804u)
#define TIMER_PERIOD 0xffffffffu
#define PORTC ((volatile uint32_t *)HYBRID_WL82_PORTC_BASE)
#define IOMAP_AUDIO (*(volatile uint32_t *)0x51030u)
#define AUDIO_CLOCK_ROUTE (*(volatile uint32_t *)0x10014u)
#define ALNK_CON0 (*(volatile uint16_t *)(HYBRID_WL82_ALNK0_BASE+0x00u))
#define ALNK_CON1 (*(volatile uint16_t *)(HYBRID_WL82_ALNK0_BASE+0x04u))
#define ALNK_CON2 (*(volatile uint8_t *)(HYBRID_WL82_ALNK0_BASE+0x08u))
#define ALNK_CON3 (*(volatile uint8_t *)(HYBRID_WL82_ALNK0_BASE+0x0cu))
#define ALNK_RX_ADDR (*(volatile uint32_t *)(HYBRID_WL82_ALNK0_BASE+0x10u))
#define ALNK_TX_ADDR (*(volatile uint32_t *)(HYBRID_WL82_ALNK0_BASE+0x1cu))
#define ALNK_DMA_LENGTH (*(volatile uint16_t *)(HYBRID_WL82_ALNK0_BASE+0x20u))
#if defined(__GNUC__)
#define AUDIO_ALIGN __attribute__((aligned(32)))
#define AUDIO_DMA __attribute__((section(".hybrid_dma"),aligned(32)))
#else
#define AUDIO_ALIGN
#define AUDIO_DMA
#endif
static int32_t rx_dma[2][AUDIO_WORDS] AUDIO_DMA;
static int32_t tx_dma[2][AUDIO_WORDS] AUDIO_DMA;
static int32_t render_q15[AUDIO_WORDS] AUDIO_ALIGN;
static const hybrid_ac79_audio_hw_t *hw;
static hybrid_ac79_render_fn render_block;
static void *render_context;
static hybrid_ac79_audio_stats_t stats;
static uint32_t ticks_per_block,previous_tick;
static uint8_t previous_valid,initialized;
static volatile uint8_t muted;

static uint32_t elapsed(uint32_t start,uint32_t end)
{
    return end>=start?end-start:end+TIMER_PERIOD+1u-start;
}
static uint32_t audio_ticks(void)
{
    return hw&&hw->ticks?hw->ticks(hw->context):TIMER_COUNT;
}
static unsigned dma_free_half(void) { return (ALNK_CON0&BIT(15))?0u:1u; }
static unsigned dma_half_owned(unsigned half)
{
    unsigned before=dma_free_half();
    uint8_t pending=(uint8_t)((ALNK_CON2>>4)&15u);
    unsigned after=dma_free_half();
    return before==half&&after==half&&!(pending&BIT(3));
}
static void dma_ack(uint8_t pending) { ALNK_CON2=(uint8_t)(pending&15u); }
static int32_t q15_to_q23(int32_t value)
{
    if(value>32767)value=32767;
    if(value< -32768)value=-32768;
    return value*256;
}
static void clear_buffers(void)
{
    unsigned h,i;
    for(h=0;h<2;h++)for(i=0;i<AUDIO_WORDS;i++){rx_dma[h][i]=0;tx_dma[h][i]=0;}
}
int hybrid_ac79_audio_init(const hybrid_ac79_audio_hw_t *hardware,
                           hybrid_ac79_render_fn render,void *context)
{
    uint32_t clock,outputs;
    if(!hardware||!hardware->clock_hz||!hardware->irq_attach||!hardware->codec_init||
       !hardware->irq_save||!hardware->irq_restore||!render)return -1;
    if(initialized)return -2;
    hw=hardware;render_block=render;render_context=context;
    clock=hw->clock_hz(hw->context);
    if(!clock)return -3;
    ticks_per_block=(uint32_t)(((uint64_t)clock*HYBRID_AC79_AUDIO_FRAMES)/HYBRID_AC79_AUDIO_RATE);
    if(!ticks_per_block)return -4;
    clear_buffers();
    IOMAP_AUDIO&=~(3u<<6);
    outputs=BIT(0)|BIT(1)|BIT(2)|BIT(6);
    PORTC[0]|=outputs;
    PORTC[3]|=BIT(3);
    PORTC[2]=(PORTC[2]&~outputs)|BIT(3);
    PORTC[5]&=~BIT(3);
    PORTC[4]|=BIT(3);
    AUDIO_CLOCK_ROUTE&=~(15u<<8);
    if(hw->codec_init(hw->context)!=0)return -5;
    ALNK_CON0=0;ALNK_CON1=0;ALNK_CON2=0;ALNK_CON3=0;
    ALNK_CON3=(uint8_t)(3u|(4u<<5));
    ALNK_CON0=(uint16_t)(BIT(8)|BIT(7));
    ALNK_DMA_LENGTH=(uint16_t)AUDIO_WORDS;
    ALNK_RX_ADDR=(uint32_t)(uintptr_t)rx_dma;
    ALNK_TX_ADDR=(uint32_t)(uintptr_t)tx_dma;
    ALNK_CON1=(uint16_t)(1u|BIT(2)|BIT(3)|BIT(12)|BIT(14));
    __asm__ volatile("" ::: "memory");
    if(hw->irq_attach(hw->context,AUDIO_IRQ,hybrid_ac79_audio_irq,AUDIO_PRIORITY)!=0){
        ALNK_CON0=0;ALNK_CON1=0;return -6;
    }
    ALNK_CON0|=BIT(11);
    initialized=1;
    return 0;
}
__attribute__((interrupt(""))) void hybrid_ac79_audio_irq(void)
{
    uint8_t pending=(uint8_t)((ALNK_CON2>>4)&15u);
    uint32_t start,interval,used;
    unsigned rx,half,committed=0,i;
    dma_ack(pending);
    rx=pending&1u;
    if(!rx||muted||!initialized)return;
    start=audio_ticks();
    if(!previous_valid){previous_tick=start;previous_valid=1;return;}
    interval=elapsed(previous_tick,start);
    previous_tick=start;
    if(interval<ticks_per_block-(ticks_per_block>>3)||
       interval>ticks_per_block+(ticks_per_block>>3)){
        ++stats.interval_errors;++stats.underruns;return;
    }
    half=dma_free_half();
    if(render_block(render_context,render_q15,HYBRID_AC79_AUDIO_FRAMES)!=0){
        ++stats.render_errors;++stats.underruns;return;
    }
    {
        uint32_t irq_state=hw->irq_save(hw->context);
        if(dma_half_owned(half)&&elapsed(start,audio_ticks())<ticks_per_block-(ticks_per_block>>3)){
            for(i=0;i<AUDIO_WORDS;i++)tx_dma[half][i]=q15_to_q23(render_q15[i]);
            __asm__ volatile("csync" ::: "memory");
            committed=dma_half_owned(half);
        }
        hw->irq_restore(hw->context,irq_state);
    }
    if(!committed){++stats.late_commits;++stats.underruns;}
    used=elapsed(start,audio_ticks());
    stats.last_ticks=used;
    if(used>stats.peak_ticks)stats.peak_ticks=used;
    ++stats.blocks;
}
void hybrid_ac79_audio_get_stats(hybrid_ac79_audio_stats_t *out)
{
    unsigned i;
    if(!out)return;
    const uint32_t *src=(const uint32_t *)&stats;
    uint32_t *dst=(uint32_t *)out;
    for(i=0;i<sizeof(stats)/sizeof(uint32_t);i++)dst[i]=src[i];
}
void hybrid_ac79_audio_mute(uint8_t mute)
{
    muted=(uint8_t)!!mute;
    if(muted)clear_buffers();else previous_valid=0;
}
int hybrid_ac79_audio_render_core(void *context,int32_t *stereo_q15,uint16_t frames)
{
    uint32_t i;
    extern void *hybrid_audio_render_context(void);
    extern void hybrid_audio_seq_process(uint32_t);
    extern void hybrid_audio_process(int32_t *,uint32_t,int32_t);
    if(!context||context!=hybrid_audio_render_context()||!stereo_q15||
       frames!=HYBRID_MAX_FRAMES)return -1;
    for(i=0;i<(uint32_t)frames*2u;i++)stereo_q15[i]=0;
    hybrid_audio_seq_process(frames);
    hybrid_audio_process(stereo_q15,frames,4096);
    return 0;
}
