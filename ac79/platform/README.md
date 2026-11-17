# Independent AC79 platform layer — SMK-37 Pro

This directory separates the HYBRID instrument engine from the physical
SMK-37 Pro board implementation.

## Verified board surface

The Pro board reference implementation reports 37 velocity-sensed keys,
18 panel buttons, 8 rotary encoders and 16 pressure pads. The HYBRID platform
contract now uses those counts (not the earlier 14-button/7-encoder estimate).
Four fader channels and two wheel channels remain exposed by the abstraction;
their ADC channel-to-control calibration still needs confirmation against the
specific Pro hardware revision.

The publicly available `m-vave/smk37_open` board sources identify:
- Matrix scan: SPI2/shift-register scan, with PA1 latch, PA3 clock, PA4 data.
- Matrix input sample pins: PH6, PH8, PH9, PA2, PA5, PA6, PA7 and PA9.
- LCD transport: SPI1; PC8 chip-select, PB5 data/command, PB6 clock,
  PB7 shared data route; PC10 RGB route.
- Audio output: ALNK0/IIS with two DMA directions/buffers and an external
  codec control sequence on PC0/PC1/PC2/PC6.

These are source-derived reference mappings, not a claim that the new HYBRID
driver has already initialized or exercised the hardware. See
`include/hybrid_ac79_wl82_map.h` for the register/pin constants.

## Driver boundary

`hybrid_ac79_board_ops_t` is the interface between the event/debounce layer
and the native WL82/AC79 board driver. The production driver still needs to
implement matrix scan, ADC/pad acquisition, encoder decoding, LCD SPI1 and
LED service directly against the pinned SDK/register definitions.

The independent audio driver must own ALNK0/IIS, aligned double buffers,
DMA-half ownership, interrupt acknowledgement, codec bring-up and underrun
statistics. It must call the HYBRID render callback only for the free half.
No UI, LCD, allocation or blocking operation may run in the audio ISR.

## Status and gate

This remains a hardware integration milestone, not a completed BSP. The
reference register map and corrected control counts are committed first so
that the drivers are built against the right Pro surface. The board and audio
driver implementations, native SDK compilation, startup/linker migration and
physical SMK-37 Pro verification remain separate gates. Felucca is retained
until all gates pass.

## Host test

```sh
cc -std=c99 -O2 -Wall -Wextra -Werror \
  -Iac79/platform/include \
  ac79/platform/src/hybrid_ac79_platform.c \
  ac79/platform/tests/test_hybrid_ac79_platform.c \
  -o /tmp/test_hybrid_ac79_platform
/tmp/test_hybrid_ac79_platform
```
