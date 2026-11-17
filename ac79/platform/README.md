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

## Scanner/ADC sequencing core (new)

`hybrid_ac79_scan.c` now provides a host-testable timing/state core for the
16-phase SPI2 scan, ping-pong input buffers, deferred software-IRQ notification,
and ADC conversion schedule. The fast group is 18 conversions (16 pad mux
samples plus two additional fast inputs); it is followed by the eight-step slow
schedule (four faders, pedal, battery, VBAT and PMU reference). The scan IRQ
models the three-tick mux settling delay before starting an ADC conversion.

This is deliberately the sequencer, not yet the concrete WL82 MMIO/SDK driver:
the production ISR wrappers still need to bind IRQ 37, software IRQ 122 and ADC
IRQ 24, apply the reference GPIO packing and calibrated pad/key curves, and
connect the SDK's ADC start/ack operations. The original scanner's DSP/control
processing and exact lookup-table calibration remain to be ported before
claiming velocity/pressure parity. The tests validate sequencing only.


## Concrete SPI2/ADC MMIO adapter (in progress)

`src/hybrid_ac79_scan_hw.c` now binds the documented WL82 register blocks and
board IRQs: SPI2 scan IRQ 37 (priority 5), software scan IRQ 122 (priority 4),
and ADC IRQ 24 (priority 5). It configures SPI2 mode/baud/DMA address/count,
acknowledges the SPI2/ADC status flags, applies the CD4051 mux selection,
starts ADC conversions with the reference three-SPI-tick settling window, and captures all 26 raw conversion results. VBAT's PMU-detect mode is passed through a required SDK callback; the analog reference-enable bit is cleared as in the Pro board source. GPIO
configuration is delegated to mandatory SDK callbacks so the port DIR
polarity and pad electrical configuration are not guessed. The scan latch
edge is held across the SPI2 DMA submission, matching the Pro reference ISR.

The fast mux map is PA08 channels 0–7, PC09 channels 0–7, then PA10 channels
4–5. The slow mux map is PA10 channels 0–3 for four faders, 6 for pedal, and
7 for battery, followed by VBAT and PMU reference on ADC channel 15. Host tests
assert the 18+8 channel routing and preserve the raw sample bank. The adapter
requires the firmware startup to provide the SDK IRQ and GPIO callbacks.

**Calibration is not yet complete:** the adapter captures raw ADC values but
does not claim calibrated pressure/velocity output. Per-key, per-pad and pedal
curves must be loaded from validated calibration records or measured on the
physical SMK-37 Pro. The reference firmware's factory defaults are not a
substitute for measurements from this unit. Physical IRQ timing, DMA activity,
voltage levels and long-run stability remain hardware test gates.
