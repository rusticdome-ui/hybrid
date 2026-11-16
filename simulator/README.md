# SMK-37 HYBRID Virtual Lab

Browser workstation for the independent HYBRID Wavetable synthesizer and the retained MSFA FM6/DX7 renderer.

- Wavetable audio is generated in AudioWorklet and mixed through the C Q15 core.
- FM6/DX7 uses the independent MSFA WebAssembly renderer.
- The VOLCA-style sequencer, MIDI input, chord mode and 16-scene mixer remain available.
- No AMB-1, DUB-1 or ZEN-1 DSPs or source assets are loaded.
- SHRUB is a product inspiration only; this repository contains no proprietary SHRUB code or presets.

The standalone simulator requires `hybrid_core.wasm`, `dx7_fm6.wasm`, `audio-worklet.js`, `dx7-factory-banks.js` and this `index.html`.
