# Independent AC79 platform layer — SMK-37 Pro

This directory defines the board-independent HYBRID platform boundary. It is
intentionally separate from the instrument engine in `src/`.

## Preserved physical surface

The event contract covers all 37 keys, 14 mapped panel buttons, 16 performance
pads, seven rotary encoders, four faders, pitch/mod wheels, the LCD transport,
and per-column LED output. Key/button/pad press and release are debounced in
the platform layer (2 scans to press, 8 scans to release). Encoder deltas and
normalized analog controls are emitted as parameter events.

## Backend boundary

`hybrid_ac79_board_ops_t` is the narrow low-level contract that a native
JieLi AC79 SDK backend must implement:
- board/GPIO/ADC/matrix initialization;
- bounded matrix and ADC sampling;
- quadrature encoder accumulation;
- LCD command/data transport;
- LED column output.

The backend must be called from the foreground/task context. It must not block
inside the audio DMA interrupt. The audio renderer continues to use the
existing HYBRID audio bridge until the independent AC79 I2S/DMA backend is
implemented and validated.

## Important implementation status

This is the first independently host-testable platform layer, **not yet a
complete hardware BSP**. The low-level pin/register implementation, LCD bus
timings, ADC channel mapping, board startup, I2S/DMA ISR, linker/startup files,
and production firmware package identity still require direct SDK integration
and verification on an SMK-37 Pro. No Felucca source is copied into this
directory. The exact physical key-to-note and panel-button semantic mapping
must be bound by the UI adapter using the SMK-37 Pro board mapping document.

## Host test

```sh
cc -std=c99 -O2 -Wall -Wextra -Werror \
  -Iac79/platform/include \
  ac79/platform/src/hybrid_ac79_platform.c \
  ac79/platform/tests/test_hybrid_ac79_platform.c \
  -o /tmp/test_hybrid_ac79_platform
/tmp/test_hybrid_ac79_platform
```
