# SMK-37 HYBRID — Wavetable workstation and VOLCA sequencer

The `hybrid-wavetable-shrub-style` branch replaces the former AMB-1, DUB-1 and ZEN-1 integrations with one original HYBRID wavetable synthesizer. It is an independent design inspired by the publicly described workflow and sound-design concepts of modern wavetable instruments. It does not contain SHRUB/Xfer source code, firmware, presets or proprietary assets.

## Instrument architecture

- **HYBRID Wavetable PRO:** native C engine for AC79 and a real-time AudioWorklet renderer for the browser. Dual oscillators, four custom Q15 table slots, 1–4 voice unison, bounded detune, sub/noise layers, ADSR, low-pass filter and four modulation routes (LFO, velocity, key tracking, envelope to pitch/cutoff/oscillator mix/amplitude).
- **FM6/DX7:** retained as a separate six-operator MSFA renderer with 128 DX7 voices in four banks.
- **PCM ingress:** reserved stereo sample slot; browser sample playback remains a separate feature and AC79 sample playback is not claimed.
- **Q15 mixer:** four fixed slots: Wavetable, FM6/DX7 PCM, reserved and PCM/sample.
- **Shared controls:** MIDI, keyboard, chord mode, 16-scene mixer, routing, mute/solo, VOLCA-style sequencer and transport.

## Implementation boundaries

The browser's Wavetable renderer runs inside AudioWorklet and submits stereo Q15 PCM into the C HYBRID mixer. The AC79 target uses the independent C wavetable renderer directly in the native audio callback. Native control-thread APIs expose PCM wavetable import/clear, parameter control, modulation routing and CRC32 preset save/load; callers provide the decoded PCM and preset buffers, with no heap allocation. Audio-file decoding and the preset browser remain browser-side. FM6/DX7 remains a separate browser renderer; this branch does not claim a native AC79 FM6 implementation.

The former AMB-1, DUB-1 and ZEN-1 DSP objects, adapters, upstream source checkouts, profile labels and validation steps are removed from the HYBRID build path. The AC79 workflow now compiles only the HYBRID core, wavetable engine, sequencer and profiler alongside the SMK-37 board startup/audio base.

## Validation

Host tests cover the Q15 core, wavetable note/render lifecycle, sequencer, stress blocks and profiler. Chromium tests exercise AudioWorklet Wavetable PCM, FM6/DX7 bank loading, chords, note-off and zero underruns. The AC79 workflow compiles and links the C engine with the JieLi pi32v2 toolchain and emits ELF/BIN/map plus static memory diagnostics.

A passing host or browser test is not physical-board certification. The latest AC79 ELF reports XIP 562,480/619,520 B, RAMTEXT 3,044/24,576 B, DATA+BSS 82,952/98,304 B, POOL 312,112/344,064 B and NOINIT 15,564/15,696 B. The linker gate preserves at least 8,192 B in POOL and 128 B in NOINIT; NOINIT currently has only 132 B free. Hardware CPU load, stack high-water, DMA underruns and long-run stability still require physical SMK-37 testing. The AC79 artifact is marked validation-only until its package identity and flashing procedure are independently verified.
