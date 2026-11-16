# SMK-37 HYBRID — Wavetable and VOLCA architecture

Status: independent HYBRID Wavetable implementation added to the browser AudioWorklet and AC79 native C path. Browser and AC79 validation workflows are being updated on the feature branch; no new CI result or physical-board certification is claimed yet.

## DSP architecture

- HYBRID Wavetable is the primary instrument: dual oscillators, four oscillator shapes, oscillator blend, detune, 16-voice polyphony, ADSR, low-pass filter and LFO.
- FM6/DX7 remains a separate six-operator MSFA renderer in the browser and feeds the shared Q15 mixer as stereo PCM.
- The fourth mixer slot remains reserved for PCM/sample ingress.
- The common C mixer retains four channels, scenes, routing, mute/solo and master gain.

The wavetable design is independent and uses no proprietary source, firmware or presets from SHRUB/Xfer Records. It is described as SHRUB-style only to communicate the broad sound-design workflow; no compatibility or sonic equivalence is claimed.

## Browser audio path

The AudioWorklet generates the wavetable signal in real time, submits interleaved stereo Q15 blocks to mixer channel A, and mixes FM6/DX7 PCM on channel B. Notes are routed by the active instrument selector. Note-off, panic, chord mode and the shared sequencer are preserved.

## AC79 audio path

The AC79 target compiles `hybrid_core.c`, `hybrid_wavetable.c`, `hybrid_native_adapters.c`, `hybrid_profile.c` and `hybrid_sequencer.c` with the JieLi pi32v2 toolchain. The native wavetable engine is registered in the HYBRID C core and rendered in the existing audio callback.

The current AC79 base still uses the SMK-37 board startup, codec and DMA transport from the checked-out upstream board project. The validation artifact remains marked validation-only until a dedicated HYBRID package identity and flashing procedure are verified. Browser FM6/DX7 does not imply a native AC79 FM6 implementation.

## Sequencer

The bounded C sequencer remains in `include/hybrid_sequencer.h` and `src/hybrid_sequencer.c`. The browser AudioWorklet mirrors the family-chain behaviour and can trigger the wavetable and FM6/DX7 engines. Internal and external 24-PPQN MIDI clock remain supported.

## Validation gates

1. Host C tests for Q15 mixer, wavetable lifecycle, sequencer and profiler.
2. Chromium test for non-zero wavetable PCM, note-off, FM6/DX7 banks, chords and zero AudioWorklet underruns.
3. AC79 cross-compilation and final ELF symbol verification, excluding all removed DSP symbols.
4. Static XIP/RAM/POOL/NOINIT report from the final ELF.
5. Physical SMK-37 testing for CPU load, stack watermark, DMA underruns, long-run stability and installable package identity.

Passing host or browser tests is not physical-board certification.
