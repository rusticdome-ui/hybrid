# SMK-37 HYBRID — native DSP integration and VOLCA sequencer

The hybrid-memory-reduction branch contains an AC79 integration path for the M-VAVE SMK-37 Pro reference firmware and a browser-based HYBRID simulator.

## Current implementation

- Four HYBRID slots: A=AMB-1, B=DUB-1, C=ZEN-1, D=GHOULBOX (optional to the VOLCA sequencer).
- Original AMB-1, DUB-1 and ZEN-1 DSP sources are compiled as isolated, namespaced AC79 objects and connected through native adapters.
- The upstream GHOULBOX engine/mixer remains the base firmware engine in the AC79 reference build; HYBRID overlays are mixed after its native mix.
- Signed Q15 four-slot mixer, per-slot volume/pan/mute/solo, MIDI zones and fixed render quantum.
- Bounded shared-clock VOLCA sequencer: four lanes, internal BPM or external MIDI IN pulse lock (24 PPQN), 16-step patterns, up to 16 chain entries, A/B variation, fills, mute and solo.
- AC79 TIMER4 profiler channels for AMB/DUB/ZEN/GHOULBOX/mixer/sequencer, stack watermark, buffer occupancy and underruns.
- Browser simulator with original DSP WASM renderers, VOLCA performance panel and optional GHOULBOX WASM renderer.

## Important engine limits

AMB-1 and DUB-1 are self-running generative instruments. Their current HYBRID note callbacks intentionally do nothing; they continue to render their original DSP, but arbitrary sequencer notes are not mapped to invented controls. ZEN-1 has a native note/velocity API and receives sequencer note events. GHOULBOX's own sequencer and internal engine table remain upstream code.

Bass, Keys, Drum, FM and Sample are performance/phrase roles, not claims that five new Korg DSPs have been implemented. The Sample phrase contains triggers only and requires a compatible loaded sample source. The browser's GHOULBOX renderer is optional; if absent, the other three native DSPs and sequencer transport still run. The generic sequencer core itself has no GHOULBOX dependency. The current AC79 reference-link workflow nevertheless uses Felucca/GHOULBOX as its startup/audio base; a fully GHOULBOX-free AC79 image would need a separate board startup, codec and audio-transport base, which is not claimed here.

## Host tests

    cc -std=c99 -Wall -Wextra -Werror -Iinclude src/hybrid_core.c tests/test_hybrid_core.c -o test_hybrid_core
    ./test_hybrid_core
    cc -std=c99 -Wall -Wextra -Werror -Iinclude src/hybrid_sequencer.c tests/test_hybrid_sequencer.c -o test_hybrid_sequencer
    ./test_hybrid_sequencer

## Build and certification status

The workflow .github/workflows/ac79-hybrid-linker.yml compiles the native DSP objects, HYBRID mixer, adapters, profiler and VOLCA sequencer with the JieLi pi32v2 toolchain, then attempts the final AC79 link and emits XIP/RAMTEXT/RAM/POOL/NOINIT margins. The first post-sequencer linked baseline passed in Actions run #43113694960; later external-clock pulse-lock changes are pending a fresh link. The workflow .github/workflows/deploy-simulator.yml builds and tests the browser simulator with Chromium and deploys GitHub Pages on an eligible branch push.

**A successful Actions run is not proof of hardware operation.** Final RAM/POOL/XIP/NOINIT margins must come from the latest successful linked ELF/map. Runtime CPU/stack/underrun values require a physical SMK-37 Pro session. The profiler's buffer counters cover registered HYBRID scratch buffers; they do not replace a live measurement of all private upstream POOL allocations. Do not flash a reference package until the final AC79 link and physical-board checks are confirmed.

## Licensing

SPDX-License-Identifier: GPL-3.0-only. Retain upstream copyright, license and third-party notices for all imported engine code.
