# SMK-37 HYBRID Virtual Lab — C core bridge

The browser simulator loads `hybrid_core.c` compiled to WebAssembly. The C implementation performs MIDI filtering, GLOBAL/ZONE/OFF routing, transpose, velocity windows, Q15 summing and saturation, per-slot volume/pan/mute/solo, and 16 in-memory scenes.

## Build and run
Open the published GitHub Pages URL over HTTPS. GitHub Actions compiles `src/hybrid_core.c` with `simulator/wasm_bridge.c` into `simulator/hybrid_core.wasm`, runs host tests and deploys the directory.

## Four engine test adapters
The WASM build registers four deterministic C signal generators in the AMB-1, DUB-1, ZEN-1 and GHOULBOX slots. These adapters are explicitly test sources, not the proprietary AC79 DSP engines. They allow the real shared C mixer to be exercised across all four slots in a browser. The AC79-specific native adapters remain firmware-target code.

## Validation limits
Host tests and WASM smoke tests validate the common C core and bridge, not AC79 timing, real engine DSP output, DMA/underruns, target RAM/POOL/XIP, or physical hardware behaviour. Scenes in this build are session-memory only.
