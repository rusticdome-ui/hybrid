# Engine integration audit — SMK-37 HYBRID

Status: **integration not yet implemented; do not flash a HYBRID-labelled image**.

This audit is based on the upstream SMK-37 branches:
- GHOULBOX: `genkigenki/Felucca@ghoulbox-smk37`
- AMB-1: `genkigenki/fm1-amb@smk37`
- DUB-1: `genkigenki/fm1-dub@smk37`
- ZEN-1: `genkigenki/zen-1@smk37`

## Why the reference build is not an integration

Felucca's `tools/build.py` compiles `firmware/src/felucca.c` into one object (`felucca.o`) and links that object with the startup/ISR objects. `felucca.c` includes the platform, DSP, sequencer, audio, UI and storage implementation in a deliberate order. It is not a conventional multi-source CMake project.

GHOULBOX's existing engine model is also not four external engine slots: it has four internal tracks (`NPART=4`), a shared 12-voice budget, and its own 15-engine table. HYBRID's four top-level slots must therefore be a separate layer above GHOULBOX; the four GHOULBOX tracks must remain intact.

## Upstream engine interfaces

### AMB-1

- DSP interface: `firmware/src/dsp/amb.h`
- Implementation: `firmware/src/dsp/amb.c`
- Audio entry point: `en_render(int32_t *out_lr, uint32_t n, uint32_t gain_q12)`
- Control interface: `en_init`, `en_set`, `en_pool`, `en_play`, `en_next`, `en_freeze`, `en_panic`
- Its application calls the renderer directly from its own ALNK0 ISR and owns its own UI, input, storage and USB application lifecycle.

### DUB-1

- Forks AMB-1 and retains the same `en_*` DSP interface and the same source-level application/platform architecture.
- It cannot be linked alongside AMB-1 unchanged: the exported `en_*` symbols and globals overlap. A dual-engine build needs separate translation units plus a deliberate symbol namespace (or a source-level refactor), not two textual includes into Felucca's single translation unit.
- DUB's complete sound is the renderer output; its chord, ping, bass and drum lanes are internal layers and must remain together in HYBRID slot B.

### ZEN-1

- Instrument interface: `firmware/src/dsp/fumi.h`; implementation family under `firmware/src/dsp/`.
- Audio entry point: `fm_render(int32_t *out_lr, uint32_t n, uint32_t gain_q12)`.
- Scene/conductor implementation is a separate application layer using FM voice, part, sample and ambience APIs. Importing only `fumi.c` does not reproduce the full ZEN-1 scenes.
- ZEN-1 also has its own app boot, timer, audio ISR, UI, storage and USB lifecycle. Those platform files cannot be copied wholesale into Felucca.

## Required integration architecture

1. Convert the firmware build from one application translation unit to separately compiled objects (or compile each isolated engine as a namespaced object). Preserve the existing single-TU GHOULBOX build as the known-good baseline during this migration.
2. Add a stable adapter ABI for init, reset, MIDI note/control, render, panic and optional state save/restore. Engine state must be private to each adapter.
3. Build AMB and DUB as separate objects with distinct symbol prefixes. Confirm their source-level globals, queues and fast-math helpers are isolated.
4. Port the ZEN instrument and its required DSP dependencies as a separately compiled object first. Add scene/conductor code only after its app dependencies are mapped and adapted; do not claim full ZEN-1 scene parity from an instrument-only port.
5. Add the HYBRID mixer above the four adapters, with explicit CPU/voice budgets. GHOULBOX remains one adapter and retains all four of its own tracks and sequencer.
6. Add hardware-independent adapter tests, target link checks, memory/pool budget checks and package validation before producing a firmware artifact.
7. Validate boot, audio DMA deadline, MIDI/key ownership, storage compatibility and recovery on an actual SMK-37 Pro before recommending installation.

## Immediate build gates

A target package may only be labelled `SMK-37 HYBRID` when all are true:
- the final link contains the four adapter entry points (AMB, DUB, ZEN and GHOULBOX);
- the hybrid audio callback calls each enabled adapter and mixes its output;
- the target link and Felucca memory/pool checks pass;
- the package identity/version is HYBRID-specific and its SHA-256 is recorded;
- the build log and upstream source SHAs are attached to the artifact.

Until then, the existing Actions package is a **GHOULBOX reference package**, not HYBRID firmware.
