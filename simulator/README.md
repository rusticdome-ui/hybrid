# SMK-37 HYBRID Virtual Lab — C core bridge

The browser simulator loads `hybrid_core.c` compiled to WebAssembly. The C implementation performs MIDI filtering, GLOBAL/ZONE/OFF routing, transpose, velocity windows, Q15 summing and saturation, per-slot volume/pan/mute/solo, and 16 in-memory scenes.

## Run locally

1. Extract `smk37-hybrid-virtual-lab.zip` into a folder.
2. Open a terminal in that folder and run `python3 -m http.server 8000`.
3. Open <http://localhost:8000> in a modern browser and press **Ligar áudio**.

`index.html` loads `hybrid_core.wasm` from the same folder. Use a local HTTP server rather than `file://`, because browsers commonly block WebAssembly fetches from local files.

## Build and run

Open the published GitHub Pages URL over HTTPS. GitHub Actions compiles `src/hybrid_core.c` with `simulator/wasm_bridge.c` into `simulator/hybrid_core.wasm`, runs host tests and deploys the directory.

## Four engine test adapters

The WASM build registers four deterministic C signal generators in the AMB-1, DUB-1, ZEN-1 and GHOULBOX slots. These adapters are explicitly test sources, not the proprietary AC79 DSP engines. They allow the real shared C mixer to be exercised across all four slots in a browser. The AC79-specific native adapters remain firmware-target code.

## Validation limits

Host tests and WASM smoke tests validate the common C core and bridge, not AC79 timing, real engine DSP output, DMA/underruns, target RAM/POOL/XIP, or physical hardware behaviour. Scenes in this build are session-memory only.
## Original AMB-1, DUB-1 and ZEN-1 browser adapters

The native-engine workflow builds the upstream host renderers as three independent WASM modules:
`simulator/native/amb1.wasm`, `dub1.wasm` and `zen1.wasm`. The browser adapter in
`native-engines.js` loads these modules and forwards their original exports:

- **MIDI Note On/Off:** the host instruments expose their physical keyboard state, not a
  general MIDI-note function. Notes F3–G5 (MIDI 53–79) are mapped to the 27-key
  `plat_keys()` bitfield. Notes outside this range are ignored by the native host path.
  The host keyboard interface has no per-note velocity, so velocity is not represented
  in these three original renderers. The HYBRID C mixer still receives its regular MIDI
  events for routing/filtering.
- **Panel controls:** button bits use the upstream AMB/DUB/ZEN panel enumeration
  (FX, SEL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ, PLAY, REC, OCTDN, OCTUP);
  encoder roles are SELECT, ALGO, PRESET, K1–K4. The native master is a separate
  0–4096 control from HYBRID's final output master.
- **ZEN-1 editor:** `web_param` exposes native parameter indices and
  `web_patch`/`web_patch_set` accepts the upstream 128-byte patch representation.
  The UI intentionally requires a 128-byte binary patch; it does not translate
  third-party patch formats.
- **PCM:** each module's original `web_render` output is converted from float stereo
  to interleaved Q15 and submitted to HYBRID slots A/B/C before the C mixer processes
  the block. The fourth GHOULBOX slot remains on its separate path until its original
  renderer has a browser WASM port. If a native module cannot load, the UI reports the
  failure and the explicitly TEST ONLY fallback remains available.

Web MIDI is available in browsers that support `requestMIDIAccess` (normally HTTPS or
localhost). Note On/Off are channel-filtered. CC16–CC22 are mapped to the seven native
encoder roles of the selected motor; other CC messages are not remapped.

The WASM workflow artifact contains the three upstream engine modules. They must be
published alongside `index.html`, `native-engines.js` and `hybrid_core.wasm` under
the `simulator/native/` path for the public browser UI to load them. A successful
AC79 firmware build alone does not create these browser assets.
