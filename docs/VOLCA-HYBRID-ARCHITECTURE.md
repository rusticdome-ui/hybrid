# SMK-37 VOLCA HYBRID — architecture and implementation plan

Status: **sequencer phrase pack implemented in the AC79 build path; complete Volca-style performance mode is not yet implemented or hardware-certified.**

## Design decision: extend the existing sequencer, do not replace it

The upstream GHOULBOX firmware already provides the strongest low-cost foundation:

- Four independent parts and one shared transport.
- Per-part pattern length, division, swing and gate.
- Step accents, slides, ties, probability and ratchets.
- MIDI clock Start/Continue/Stop and clock-loss handling.
- Pattern recording/playback, arpeggiator and parameter-motion recording.
- A native engine table that includes ANALOG (0), PHASE (2), SAMPLE (4), DRUM (10) and FM6 (12).

The HYBRID overlay remains above the original GHOULBOX mixer. AMB-1, DUB-1 and ZEN-1 continue to be compiled as isolated, namespaced DSP objects. This feature does not replace their renderers, GHOULBOX's four internal tracks, or the native sequencer.

## Volca-inspired family mapping

The five families are selectable sound/phrase roles, not five new DSP implementations:

| Family | Existing GHOULBOX engine | Factory phrase | Notes |
|---|---|---|---|
| Bass | ANALOG (engine 0) | VOLCBASS (23) | Acid/bass line with accents and slides |
| Keys | ANALOG (0) or PHASE (2) | VOLCKEYS (24) | Off-beat chord/lead notes; enable the track's chord mode for chord stabs |
| Drum | DRUM (10) | VOLCDRUM (25) | GM kick/snare/closed/open-hat triggers |
| FM | FM6 (12) | VOLCFM (26) | Six-operator FM arpeggio phrase |
| Sample | SAMPLE (4) | VOLCSAMP (27) | Trigger pattern for a loaded sample set; the pattern does not contain sample audio |

The five phrases are appended after the existing factory patterns. Existing pattern numbers 1–22 and all scene references remain unchanged. Each pattern is loaded through the existing **SEQ > PATTERNS** workflow, on whichever of the four parts is selected.

There are four GHOULBOX parts, so up to four of these roles can run simultaneously inside GHOULBOX. The fifth role remains available by changing the engine/pattern on a part; this first implementation does not claim five independent GHOULBOX parts.

AMB-1 and DUB-1 are generative engines whose current native MIDI note callbacks intentionally do nothing; their performance/clock/freeze controls need a separate, source-accurate command mapping before they can be treated as conventional note sequencer destinations. ZEN-1 has a native note/velocity API and remains available as a HYBRID overlay. No synthetic substitute is introduced for any of them.

## Memory and CPU policy

The current successful AC79 map for base commit `df735cfd63bb3db79f6dc8ff1ac6fe117e35afad` reported:

| Region | Used | Capacity | Free |
|---|---:|---:|---:|
| XIP | 607,584 B | 619,520 B | 11,936 B |
| RAM (.data + .bss) | 83,248 B | 98,304 B | 15,056 B |
| POOL | 335,212 B | 344,064 B | 8,852 B |
| NOINIT | 15,564 B | 15,696 B | 132 B |

These are linker-map values from Actions run [#42895951511](https://github.com/rusticdome-ui/hybrid/actions/runs/42895951511), not runtime measurements on a physical board.

The phrase pack is constant factory data compiled into the firmware image. It adds no sequencer state, heap allocation, PCM buffer, voice, or sample RAM. Its exact XIP delta and the final region margins must be re-measured in the next AC79 link run. Keep the new code/data small: NOINIT has only 132 B of reported margin and POOL only 8,852 B.

No external SDRAM is assumed. The SMK-37 Pro board/package must be physically verified before any design uses external RAM. The current runtime profiler can measure CPU ticks, stack watermark, registered buffers and underruns on hardware; CI can only validate static linker occupancy and host/WASM behaviour.

## Implementation in this branch

`tools/ac79_link_engines.py` now appends five named 16-step phrases to the upstream GHOULBOX pattern table when constructing the compact AC79 target build. The insertion is guarded by an exact source anchor, validates all five names, and is idempotent. It does not edit upstream DSP algorithms or the existing pattern IDs.

## Validation gates

1. Host validation: verify all five phrase names, 16 notes and 16 flag bytes; verify the insertion is idempotent and old pattern IDs are preserved.
2. AC79 target validation: compile and link the original GHOULBOX, AMB-1, DUB-1 and ZEN-1 objects; inspect final XIP/RAM/POOL/NOINIT sections and symbols.
3. Sequencer regression: use the upstream GHOULBOX tests for pattern loading, MIDI transport, ratchet, gate, swing, ties/slides and sample-set missing behaviour.
4. Runtime validation on a physical SMK-37 Pro: profile all four parts plus the three HYBRID overlays, measure CPU maxima, stack high-water, pool use and DMA underruns over a sustained session.
5. Only after these gates, add a Volca performance UI for quick family/engine selection, pattern chains, per-part mutes and performance macros.

## Explicit non-goals of this first increment

- No new Volca DSP code or reverse-engineered Korg firmware.
- No claim of five simultaneous GHOULBOX parts.
- No fabricated sample bank or generated replacement audio.
- No changes to original AMB-1, DUB-1, ZEN-1 or GHOULBOX DSP algorithms.
- No claim of hardware certification from GitHub Actions alone.
