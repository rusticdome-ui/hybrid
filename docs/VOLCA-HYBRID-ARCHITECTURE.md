# SMK-37 VOLCA HYBRID — architecture and implementation

Status: **shared-clock sequencer core, AC79 link integration and browser performance panel are implemented in this branch. The latest AC79 link and Chromium validations are pending; no physical-board certification is claimed.**

## Design decision: add a sequencer/coordination layer; preserve original DSPs

The existing native DSP objects remain independently compiled and namespaced:
- AMB-1 (amb_en_*)
- DUB-1 (dub_en_*)
- ZEN-1 (zen_fm_*)
- GHOULBOX remains the upstream firmware engine in the AC79 reference package, but the VOLCA sequencer does not depend on its presence.

No Korg firmware or DSP algorithms are copied. The VOLCA names describe performance roles and phrase styles.

## VOLCA family and phrase roles

| Family | Phrase role | Important limitation |
|---|---|---|
| Bass | 16-step bass line with accents/syncopation | Uses the selected native engine's note/control path; not a Volca Bass clone |
| Keys | Chord/lead phrase | ZEN-1 is the current HYBRID overlay with a conventional note API |
| Drum | GM-style percussion triggers | Requires a drum-capable destination; not a new drum DSP |
| FM | Arpeggiated melodic phrase | A phrase role, not proof of a new six-operator FM implementation in AMB/DUB/ZEN |
| Sample | Sample trigger phrase | Contains note/trigger events only; no sample audio or sample bank is embedded |

The GHOULBOX AC79 build path also appends five factory phrase records (IDs 23–27) to the upstream pattern table, without reordering existing pattern IDs. These records are loaded through the existing GHOULBOX pattern workflow; they are separate from the bounded HYBRID sequencer phrase tables.

## Shared-clock sequencer

New files:
- include/hybrid_sequencer.h
- src/hybrid_sequencer.c
- tests/test_hybrid_sequencer.c

Properties:
- Four fixed lanes corresponding to HYBRID slots AMB-1, DUB-1, ZEN-1 and optional GHOULBOX.
- One sample-rate-derived internal clock or exact external MIDI-clock pulse input, 40–240 BPM, 24 PPQN and 16th-note steps.
- Up to 16 pattern-family entries in a chain; the chain advances at each 16-step bar.
- Variation A/B, a four-step fill overlay, per-lane mute and solo.
- Immutable factory note/velocity tables; no heap allocation, no dynamic pattern buffers and no new sample/PCM buffers.
- Start/Stop releases active notes. Internal mode emits MIDI Start/Clock/Stop on the existing USB-MIDI output. External mode follows the native GHOULBOX MIDI-clock pulse callback without echoing the incoming clock. Note events route to registered native engine callbacks.
- The AC79 audio hook advances the internal-clock sequencer once per audio block, before the existing GHOULBOX mix and HYBRID overlay render; external mode advances on the exact incoming 24-PPQN pulses.

AMB-1 and DUB-1 are self-running generative engines in the current adapter. Their note callbacks intentionally remain no-ops until a source-accurate performance/clock control mapping is validated. ZEN-1 accepts the sequencer's note events. The sequencer does not fabricate note support for AMB/DUB.

## Simulator performance panel

The browser AudioWorklet has the same 24-PPQN clock and 16-step scheduler, with four independently muteable/soloable lanes. The panel provides:
- Bass, Keys, Drum, FM and Sample family selection.
- Pattern chains of up to 16 IDs (1–5).
- Variation A/B and a fill on the last four steps.
- Four-lane mute/solo and Start/Stop.
- Sequencer max/average execution time, clock ticks and step counters in browser metrics.

The three original AMB/DUB/ZEN WASM renderers remain required. GHOULBOX WASM is optional in the simulator; when absent, lane 4 and GHOULBOX Program Change are inactive, while the other lanes and transport remain available. Browser timing is not an AC79 CPU measurement.

## AC79 profiling and memory policy

The existing AC79 profiler now has a separate sequencer timing channel (TIMER4), alongside AMB, DUB, ZEN, GHOULBOX and mixer timing. It continues to expose:
- CPU ticks/calls/frames/last/max per profiled engine and sequencer.
- Stack watermark and peak sampled usage.
- HYBRID scratch-buffer occupancy/capacity.
- DMA underrun count.

Static POOL, RAM, XIP and NOINIT usage must be taken from the final linked ELF/map. The runtime buffer counters are **not** a live measurement of every private GHOULBOX/DSP POOL allocation. GitHub-hosted CI cannot measure the physical board's live CPU, stack or DMA underruns.

The latest successful final AC79 map for the first integrated sequencer build (Actions run [#43113694960](https://github.com/rusticdome-ui/hybrid/actions/runs/43113694960), commit 7a7983cc) reported:

| Region | Used | Capacity | Remaining |
|---|---:|---:|---:|
| XIP | 608,688 B | 619,520 B | 10,832 B |
| RAM (.data + .bss) | 83,248 B | 98,304 B | 15,056 B |
| POOL | 335,296 B | 344,064 B | 8,768 B |
| NOINIT | 15,564 B | 15,696 B | 132 B |

These values include the first integrated C sequencer object and its AC79 adapter hook. Subsequent external-clock pulse-lock changes are still awaiting a fresh final link/map, so these are the latest confirmed baseline rather than the final margins for the current HEAD. NOINIT and POOL are particularly tight; do not add large runtime buffers or assume external SDRAM.

## Validation gates

1. Host tests for tempo bounds, 24-PPQN clock, chain advancement, variation, fill, mute/solo and note release.
2. AC79 target compile/link including hybrid_sequencer.o; final ELF/map must report positive RAM and POOL margins and retain all original DSP symbols.
3. Chromium AudioWorklet test for real original DSP WASM output, VOLCA controls, shared clock, chain and transport; no underruns in the defined test window.
4. Physical SMK-37 Pro run with four DSP engines/overlays enabled: measure TIMER4 maxima, stack high-water, static POOL link occupancy and DMA underruns over a sustained session.

A successful host or Chromium test is not a physical-board certification. No firmware artifact should be flashed until the final AC79 build and board-level validation are explicitly confirmed.
