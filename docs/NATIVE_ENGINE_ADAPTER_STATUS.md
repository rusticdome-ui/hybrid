# Native engine adapter status

This branch adds the first executable adapter layer for the HYBRID mixer.

## Implemented here

- AMB-1, DUB-1 and ZEN-1 adapter callback tables matching `hybrid_engine_ops_t`.
- Native 24-bit renderer to HYBRID Q15 conversion, with a 128-frame bound and no allocation in the render callback.
- Registration of the three adapters into HYBRID slots A, B and C. Slot D is deliberately left unregistered for the GHOULBOX host integration.
- Host contract tests using explicit fake native-port functions. These tests validate registration, callback linkage and mixer arithmetic only.

## Not yet integrated

The native DSP implementations are **not** vendored or linked by this branch. The declarations in `include/hybrid_native_ports.h` are the namespaced ABI required from the three upstream DSP ports:

- AMB-1: `amb_en_*`
- DUB-1: `dub_en_*`
- ZEN-1: `zen_fm_*`

AMB-1 and DUB-1 both export `en_*` upstream and have incompatible parameter enums and globals. They must be compiled as isolated translation units with complete symbol namespacing; simply including both source files in Felucca's single translation unit is unsafe. ZEN-1's `fumi.c` also includes its own FM core, tuning, reverb, patch and bed dependencies and has its own parameter namespace.

The current AMB/DUB MIDI note callbacks intentionally do nothing: these are generative instruments, not conventional MIDI note allocators. Their native start/stop, clock, freeze, performance and parameter commands need explicit mapping in the next port stage. ZEN-1 note-on/off is mapped to its native `fm_note_vel` API.

## Build status

- Host core tests: existing test suite.
- Adapter contract tests: compile against fake native-port stubs; not a DSP audio test.
- JieLi target build: still builds the upstream GHOULBOX reference only.
- No HYBRID `.fwsc` is produced by this branch.
- Do not flash a reference artifact as HYBRID firmware.

## Required next step

Vendor the exact upstream DSP source/dependency sets, apply collision-free symbol and enum namespacing to AMB/DUB, migrate the Felucca build from one translation unit to isolated objects (or an equivalent safe LTO strategy), then register GHOULBOX as slot D and run a real AC79 target link with memory/pool checks.
