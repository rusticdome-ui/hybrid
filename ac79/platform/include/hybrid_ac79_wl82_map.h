/* SPDX-License-Identifier: GPL-3.0-only
 * SMK-37 Pro WL82/AC79 board reference map.
 *
 * Values below are transcribed from the public m-vave/smk37_open board
 * implementation (app/board/smk37_hw.h, scan.c, fm1_lcd_hw.h,
 * fm1_audio.h). This file only names hardware resources; it does not perform
 * register I/O or claim a tested driver.
 */
#ifndef HYBRID_AC79_WL82_MAP_H
#define HYBRID_AC79_WL82_MAP_H
#include <stdint.h>

/* WL82 peripheral register blocks used by the Pro board implementation. */
#define HYBRID_WL82_PORTA_BASE       0x00050000u
#define HYBRID_WL82_PORTB_BASE       0x00050040u
#define HYBRID_WL82_PORTC_BASE       0x00050080u
#define HYBRID_WL82_PORTH_BASE       0x000501c0u
#define HYBRID_WL82_IOMAP_BASE       0x0005101cu
#define HYBRID_WL82_SPI1_BASE        0x00011d00u
#define HYBRID_WL82_SPI2_BASE        0x00011e00u
#define HYBRID_WL82_ADC_BASE         0x00013100u
#define HYBRID_WL82_ALNK0_BASE       0x00012e00u

/* Matrix scan shift-register connections and sampled GPIOs. */
#define HYBRID_SCAN_LATCH_PA1        1u
#define HYBRID_SCAN_CLOCK_PA3        3u
#define HYBRID_SCAN_DATA_PA4         4u
#define HYBRID_SCAN_INPUT_PH6        118u
#define HYBRID_SCAN_INPUT_PH8        120u
#define HYBRID_SCAN_INPUT_PH9        121u
#define HYBRID_SCAN_INPUT_PA2        2u
#define HYBRID_SCAN_INPUT_PA5        5u
#define HYBRID_SCAN_INPUT_PA6        6u
#define HYBRID_SCAN_INPUT_PA7        7u
#define HYBRID_SCAN_INPUT_PA9        9u

/* LCD SPI1 and shared RGB route. */
#define HYBRID_LCD_CS_PC8            40u
#define HYBRID_LCD_DC_PB5            21u
#define HYBRID_LCD_SCLK_PB6          22u
#define HYBRID_LCD_DATA_PB7          23u
#define HYBRID_RGB_ROUTE_PC10        42u

/* ALNK0/IIS registers and codec control pins. */
#define HYBRID_ALNK0_CON0            (HYBRID_WL82_ALNK0_BASE + 0x00u)
#define HYBRID_ALNK0_CON1            (HYBRID_WL82_ALNK0_BASE + 0x04u)
#define HYBRID_ALNK0_CON2            (HYBRID_WL82_ALNK0_BASE + 0x08u)
#define HYBRID_ALNK0_CON3            (HYBRID_WL82_ALNK0_BASE + 0x0cu)
#define HYBRID_ALNK0_RX_ADDR         (HYBRID_WL82_ALNK0_BASE + 0x10u)
#define HYBRID_ALNK0_TX_ADDR         (HYBRID_WL82_ALNK0_BASE + 0x1cu)
#define HYBRID_ALNK0_DMA_LENGTH      (HYBRID_WL82_ALNK0_BASE + 0x20u)
#define HYBRID_CODEC_MCLK_PC0        0u
#define HYBRID_CODEC_SCLK_PC1        1u
#define HYBRID_CODEC_LRCLK_PC2       2u
#define HYBRID_CODEC_ENABLE_PC6      6u
#define HYBRID_CODEC_RX_CLOCK_PC3    3u

#define HYBRID_AC79_AUDIO_RATE_HZ    44118u
#define HYBRID_AC79_AUDIO_BLOCK_FRAMES 128u
#define HYBRID_AC79_AUDIO_CHANNELS   2u

#endif
