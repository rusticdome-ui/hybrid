# SMK-37 HYBRID Virtual Lab

Browser workstation for the independent HYBRID Wavetable synthesizer and the retained MSFA FM6/DX7 renderer.

- Wavetable audio is generated in AudioWorklet and mixed through the C Q15 core.
- FM6/DX7 uses the independent MSFA WebAssembly renderer.
- The VOLCA-style sequencer, MIDI input, chord mode and 16-scene mixer remain available.
- No AMB-1, DUB-1 or ZEN-1 DSPs or source assets are loaded.
- SHRUB is a product inspiration only; this repository contains no proprietary SHRUB code or presets.

The standalone simulator requires `hybrid_core.wasm`, `dx7_fm6.wasm`, `audio-worklet.js`, `dx7-factory-banks.js` and this `index.html`.

## Wavetable PRO controls

The Wavetable panel now includes:
- Import of an audio file (WAV and browser-decodable formats) into one of four custom wavetable slots; the first channel is resampled to a 2048-point cycle.
- Four-way unison with adjustable detune, envelope-gated sub oscillator and noise layer.
- Four modulation routes: LFO, velocity, key tracking or envelope to pitch, filter cutoff, oscillator mix or amplitude.
- Presets stored in browser local storage, with JSON import/export. Custom source tables are included in the preset JSON.

These controls affect the AudioWorklet wavetable signal path before it is submitted to the C Q15 mixer. They are browser-simulator features; they are not yet wired into the native AC79 render loop. Presets stored in local storage are browser/profile-specific; use JSON export to move them between machines.
