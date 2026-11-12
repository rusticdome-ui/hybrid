# SMK-37 VOLCA HYBRID — DAW and sampler architecture

Status: browser DAW UI and eight-slot PCM sample player are implemented in the current branch. AC79 still requires a fresh final link and the on-device sampler is not yet implemented.

## DSP architecture

The browser product contains three original instruments:
- AMB-1
- DUB-1
- ZEN-1

Their original WASM renderers are preserved and feed the HYBRID Q15 mixer. The fourth mixer channel is now the **PCM Sample Player**. GHOULBOX WASM, its Program Change control, its sequencer lane and its browser build dependency have been removed.

The current AC79 reference integration still uses Felucca as the board startup/audio/codec/DMA base. The HYBRID instrument layer no longer treats GHOULBOX as a fourth instrument, but removing Felucca entirely requires a replacement board startup, codec initialization, MIDI/USB plumbing and DMA audio transport. That replacement is outside this change and no GHOULBOX-free installable AC79 image is claimed.

## Browser sampler

The AudioWorklet provides eight stereo one-shot slots:
- Import WAV, AIFF, OGG or MP3 through Web Audio decode support.
- Browser decoding converts to the active AudioContext sample rate.
- Maximum 20 seconds per slot.
- Preview trigger per slot.
- Sample phrase triggers are routed to slot D / PCM Sample Player.
- PCM is mixed into the fourth HYBRID channel before the Q15 mixer.
- Eight sample slots are in browser memory only; they are not persisted or transferred to AC79 flash.

The VOLCA Sample phrase maps MIDI notes 36, 38, 42 and 46 to sampler slots 1–4. Other slots are available for manual preview. Sample voices are one-shot and retrigger per slot; per-slot envelope, trim, slice editing, reverse, timestretch and streaming are not yet implemented.

## DAW interface

The simulator now presents:
- Transport and BPM controls.
- Eight-bar arrangement overview with family clips.
- Bass, Keys, Drum, FM and Sample family selection.
- Variation A/B, fill, mute and solo.
- Piano-roll phrase preview.
- Mixer view with per-slot volume, pan and routing.
- Eight-slot sample browser and preview controls.

The arrangement currently edits one shared family chain, not independent clip sequences per track. The piano roll is a visual phrase preview, not yet an editable MIDI note grid. Recording/overdub, automation lanes, project persistence and independent track clip editing remain future work.

## Sequencer

The bounded C sequencer remains in:
- `include/hybrid_sequencer.h`
- `src/hybrid_sequencer.c`

It uses four lanes (AMB, DUB, ZEN and Sample), 16 steps, up to 16 family-chain entries, A/B variation, fills, mute/solo and 24-PPQN internal/external clock. The browser AudioWorklet mirrors the family-chain behaviour and triggers the sampler on the Sample family.

The AC79 adapter still routes AMB/DUB/ZEN through their original DSP callbacks. The fourth channel is reserved for sample PCM, but there is no AC79 sample loader, flash bank format or sample playback callback yet.

## Validation

The Chromium test must validate:
1. Original AMB/DUB/ZEN WASM startup.
2. Import of multiple WAV files into independent sampler slots.
3. Preview playback and non-zero output through the fourth Q15 mixer channel.
4. Zero AudioWorklet underruns during the test window.
5. Arrangement chain, family changes, A/B variation, fills, mute/solo and MIDI 24-PPQN clock.

The browser test does not prove AC79 hardware sample playback or target CPU/stack/POOL margins.

Historical AC79 baseline from the previous linked ELF:
- XIP free: 10,380 B
- RAM free: 15,056 B
- POOL free: 8,768 B
- RAMTEXT free: 21,540 B
- NOINIT free: 132 B

These are not refreshed measurements for the current branch after removal of the Volca factory-pattern injection. A fresh AC79 compile/link/report is required. NOINIT and POOL remain the tightest constraints.
