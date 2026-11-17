# Independent AC79 platform layer — SMK-37 Pro

This directory separates the HYBRID instrument engine from the physical
SMK-37 Pro board implementation. The migration is incremental; Felucca remains
in the existing firmware build until every replacement gate passes.

## Verified board surface

The public SMK-37 Pro board source in `m-vave/smk37_open` reports:
- 37 velocity-sensed keys, 18 panel buttons, 8 rotary encoders and 16 pads.
- Matrix scan via SPI2/shift-register hardware and sampled GPIO inputs.
- LCD transport via SPI1.
- ALNK0/IIS audio at 44.118 kHz, 128 stereo frames per DMA block.
- ALNK0 audio clock/control pins PC0/PC1/PC2/PC3/PC6.

The register and pin constants are recorded in
`include/hybrid_ac79_wl82_map.h`. They are source-derived references, not
evidence that the new HYBRID input backend has already exercised the board.

## Native ALNK0 audio driver

`include/hybrid_ac79_audio.h` and `src/hybrid_ac79_audio.c` provide the first
native audio-driver implementation:
- ALNK0 register setup and RX/TX DMA buffer configuration.
- Two 128-frame stereo DMA halves, 32-byte aligned.
- Renderer callback consumes signed Q15 stereo; driver promotes samples to
  signed Q23 32-bit DMA slots.
- DMA IRQ entry point, half-ownership checks and late-commit/underrun counters.
- Peak and last render duration counters.
- No allocation, blocking calls or UI work inside the IRQ.

The board integration must provide the clock, interrupt-vector attachment and
codec bring-up callbacks. The public board code establishes the ALNK0 pins and
register setup, but the complete external codec programming sequence has not
yet been identified in the available source. The driver therefore refuses to
start if codec initialization is not supplied; it does not pretend that merely
setting the enable pin initializes the codec.

## Physical input backend — still pending

The event/debounce layer in `hybrid_ac79_platform.c` is not a physical scanner.
The production backend still needs native SPI2 matrix scan, key-contact velocity,
ADC/multiplexer sequencing for pad pressure and faders/wheels, quadrature
encoder decoding, SPI1 LCD transactions, and LED multiplexing. The public
reference scanner uses additional timer/ADC interrupt routing and calibration
curves; these must be ported with their real board services rather than replaced
by guessed polling delays or fabricated pin assignments.

## Build and removal gates

1. Compile the ALNK0 driver with JieLi `pi32v2` (CI step added).
2. Implement and compile the physical SPI2/ADC/LCD/LED backend with the pinned
   AC79 SDK.
3. Integrate the renderer callback and codec initialization in native firmware.
4. Validate on SMK-37 Pro: audio continuity, controls, DMA ownership, underruns,
   CPU and RAM.
5. Only after all previous gates pass, migrate startup/linker/package identity
   and remove Felucca from CI and firmware.

Host compilation is only a syntax/build check. It does not exercise memory-mapped
registers. A pi32v2 object compile does not prove board operation. Hardware
validation must be performed on the physical SMK-37 Pro.


## Native audio integration update

The ALNK0 backend now requires explicit SDK-provided `irq_save`/`irq_restore`
hooks and performs the DMA half ownership check, copy, memory fence and
post-copy ownership check inside that critical section. This avoids treating
a pre-copy free-half observation as proof that the half remained writable.

`hybrid_ac79_audio_render_core()` adapts the native `hybrid_core_t` /
`hybrid_process()` renderer to the audio callback contract. The board startup
must register the Wavetable engine before passing the core to this adapter.

This remains an integration component, not a complete board port. The SDK
adapter must still supply verified clock, codec, interrupt attach and critical
section implementations. Physical DMA timing and codec output require tests
on the SMK-37 Pro.

## Control backend status

The exact scan/ADC behavior is documented from the public SMK-37 Pro
reference source, but the independent AC79 implementation of SPI2 scan ISR,
software scan ISR, ADC/CD4051 scheduling and calibration curves, encoder
quadrature, button debounce, LCD SPI1 and multiplexed LEDs is not yet present.
The existing callback contract is not evidence that these peripherals operate
without Felucca.
