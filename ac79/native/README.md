# smk37-hybrid-native

Standalone firmware target scaffold for SMK-37 Pro / AC79. The target is
deliberately separate from the Felucca build and does not patch upstream
sources.

## Current scope

- HYBRID-owned C runtime memory copy/zero routines.
- Explicit preservation of `.noinit` (excluded from zero table).
- HYBRID-owned software IRQ handler registry (128 slots).
- HYBRID-owned linker section fragment for FLASH, RAM and POOL.
- Host-testable memory initialization.

## Not yet a flashable AC79 image

The repository does not yet contain a verified, standalone SMK-37 Pro reset
ABI, CPU exception-vector format/relocation sequence, or authoritative memory
region origin/length definitions for this target. The existing WL82 register
map is a peripheral map, not a complete boot/linker memory map. Consequently,
this scaffold intentionally does not invent a reset-vector instruction,
flash origin, RAM origin, or memory size. The linker fragment requires the
verified board memory map to define FLASH, RAM and POOL.

The current AC79 validation workflow still builds through the upstream
Felucca project. This target is not wired into that production link yet.

## Integration gates

1. Obtain the exact AC79 ROM/reset ABI and boot header requirements from the
   pinned JieLi SDK/toolchain and board boot package.
2. Verify SMK-37 Pro flash/RAM/POOL regions and reserved boot/OTA areas.
3. Implement the architecture reset/vector adapter and place it in
   `.hybrid_reset`; route all 128 peripheral IRQ entries through the native
   IRQ registry only after validating the hardware dispatch format.
4. Add a standalone link using the target's own linker script and verify ELF
   entry, sections, symbols and map. No Felucca sources or libraries allowed.
5. Port clock/PLL, GPIO, SPI2, ADC, LCD, ALNK0/DMA, flash and MIDI services.
6. Boot on physical SMK-37 Pro and capture UART/LED/USB diagnostics before
   enabling audio.

## Safety

Do not flash this scaffold. It is source-level groundwork only. Keep PR #11
and #12 integration as the known-good validation reference until the native
target passes the independent link and physical boot gates.

## Pinned JieLi SDK findings

The validation workflow pins `AC79NN_SDK_V1.2.1_2023-12-13`. JieLi's WL82 platform documentation identifies these authoritative source inputs:

- `cpu/wl82/setup.c`: `setup_arch()` performs early watchdog, clock, interrupt, debug and reset-reason initialization.
- `cpu/wl82/sdk_ld_sfc.c`: SFC/XIP linker entry.
- `cpu/wl82/sdk_ld_sdram.c`: SDRAM linker entry.
- `include_lib/driver/cpu/wl82/media_text.ld`, `system.ld`, and `system_data.ld`: shared linker fragments.

The documented boot flow is ROM/boot code handing off to the WL82 platform setup. The public documentation does **not** specify the complete reset instruction ABI, physical exception-vector encoding/relocation sequence, or the SMK-37 Pro's exact flash/RAM/POOL origins and reserved ranges. Those details must be confirmed from the pinned source and the matching SMK-37 boot package, not inferred from the Felucca vector table or a different AC79 board.

The CI workflow now expands its sparse checkout to collect the pinned setup/linker/boot sources and emits `native-sdk-evidence.tar.gz` with the validation artifact. The captured source is the evidence gate for implementing the actual reset/vector and full linker script.

## Current migration boundary

The existing ALNK0 board adapter still writes IRQ 11 into the legacy vector window at `0x01c7fe00`. That is not an independent native vector implementation. It also relies on board clock, pin-mux and DAC assumptions that need to be checked against the exact SDK/SMK-37 Pro sources. Until the native reset/vector path is verified, ALNK0 must not be described as booting independently of Felucca.

Likewise, the current native C runtime and section fragment are not a complete firmware link. The next native ELF must be built with `-nostartfiles` and the complete HYBRID-owned linker script, with no Felucca `crt0.o`, vector objects, libraries or startup calls. The current validation ELF is still produced by the legacy Felucca build and is not evidence of a standalone image.
