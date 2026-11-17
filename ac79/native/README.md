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
