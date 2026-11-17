/* SPDX-License-Identifier: GPL-3.0-only
 * Independent SMK-37 Pro board/platform contract for HYBRID.
 * The AC79 SDK-specific GPIO/I2S/ADC/LCD backend is supplied separately.
 */
#ifndef HYBRID_AC79_PLATFORM_H
#define HYBRID_AC79_PLATFORM_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define HYBRID_AC79_KEYS 37u
#define HYBRID_AC79_BUTTONS 18u
#define HYBRID_AC79_ENCODERS 8u
#define HYBRID_AC79_FADERS 4u
#define HYBRID_AC79_PADS 16u
#define HYBRID_AC79_WHEELS 2u
#define HYBRID_AC79_DEBOUNCE_PRESS 2u
#define HYBRID_AC79_DEBOUNCE_RELEASE 8u

typedef enum {
 HYBRID_HW_KEY_DOWN, HYBRID_HW_KEY_UP,
 HYBRID_HW_BUTTON_DOWN, HYBRID_HW_BUTTON_UP,
 HYBRID_HW_ENCODER, HYBRID_HW_FADER, HYBRID_HW_WHEEL,
 HYBRID_HW_PAD_DOWN, HYBRID_HW_PAD_UP
} hybrid_ac79_event_type_t;

typedef struct {
 hybrid_ac79_event_type_t type;
 uint8_t index;
 int16_t value;
} hybrid_ac79_event_t;

/* Raw, board-specific readings. Bit 1 means physically active/pressed. */
typedef struct {
 uint32_t keys;       /* bits 0..36 */
 uint16_t buttons;    /* bits 0..17 */
 uint16_t pads;       /* bits 0..15 */
 int16_t encoder_delta[HYBRID_AC79_ENCODERS];
 uint16_t fader[HYBRID_AC79_FADERS]; /* normalized 0..4095 */
 uint16_t wheel[HYBRID_AC79_WHEELS]; /* normalized 0..4095 */
} hybrid_ac79_input_snapshot_t;

/* Low-level operations implemented against the pinned JieLi AC79 SDK.
 * scan_inputs must be bounded and non-blocking; LCD writes must not run in
 * the audio/DMA interrupt. All callbacks return 0 on success. */
typedef struct {
 int (*init)(void *context);
 int (*scan_inputs)(void *context, hybrid_ac79_input_snapshot_t *snapshot);
 int (*lcd_write)(void *context, uint8_t is_data, const uint8_t *bytes, uint16_t length);
 int (*led_write)(void *context, uint8_t column, uint8_t rows);
 void *context;
} hybrid_ac79_board_ops_t;

typedef void (*hybrid_ac79_event_fn)(void *context, const hybrid_ac79_event_t *event);
typedef struct {
 const hybrid_ac79_board_ops_t *board;
 hybrid_ac79_event_fn emit;
 void *emit_context;
 hybrid_ac79_input_snapshot_t previous;
 uint8_t key_count[HYBRID_AC79_KEYS];
 uint8_t button_count[HYBRID_AC79_BUTTONS];
 uint8_t pad_count[HYBRID_AC79_PADS];
 uint32_t stable_keys;
 uint16_t stable_buttons, stable_pads;
 uint16_t last_fader[HYBRID_AC79_FADERS];
 uint16_t last_wheel[HYBRID_AC79_WHEELS];
 uint8_t initialized;
} hybrid_ac79_platform_t;

int hybrid_ac79_platform_init(hybrid_ac79_platform_t *platform,
 const hybrid_ac79_board_ops_t *board, hybrid_ac79_event_fn emit, void *emit_context);
int hybrid_ac79_platform_poll(hybrid_ac79_platform_t *platform);
int hybrid_ac79_platform_lcd(hybrid_ac79_platform_t *platform, uint8_t is_data,
 const uint8_t *bytes, uint16_t length);
int hybrid_ac79_platform_led(hybrid_ac79_platform_t *platform, uint8_t column, uint8_t rows);
#ifdef __cplusplus
}
#endif
#endif
