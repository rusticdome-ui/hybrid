# SMK-37 HYBRID — DAW workstation and VOLCA sequencer

The `hybrid-memory-reduction` branch contains the SMK-37 AC79 integration work and a browser DAW-style workstation.

## Current product architecture

- Three original DSPs: AMB-1, DUB-1 and ZEN-1, compiled independently and preserved internally as selectable soundbanks; they are no longer presented as independent instrument panels.
- Soundbank UI: select AMB-1, DUB-1 or ZEN-1 and choose numbered patches; selection is sent to each original DSP's PRESET encoder. Official patch names/counts are not yet exported, so the UI uses neutral numbered positions.
- Fourth HYBRID mixer slot is now **PCM Sample Player**, not a GHOULBOX instrument.
- Browser sampler: eight stereo one-shot slots; imports audio files supported by Web Audio (WAV/AIFF/OGG/MP3 in Chromium), resampled by the browser to the active session rate; 20-second limit per slot.
- DAW-style UI: arrangement view, 8-bar clip overview, shared family chain, transport, BPM, variation A/B, fill, track mute/solo, piano-roll phrase preview, mixer and sample browser.
- Bounded shared-clock sequencer: 16-step phrases, up to 16 family entries in the chain, internal BPM or external MIDI IN clock at 24 PPQN.
- Q15 four-channel mixer with per-slot volume, pan, mute, solo and routing.
- Original native DSP WASM renderers remain the audio sources for AMB/DUB/ZEN. Sample PCM is submitted to the fourth mixer slot before the Q15 mix.

## Important limits

The browser's eight sample slots are session-memory only. They are not stored in the SMK-37 flash and do not yet stream from an SD card or external storage. The DAW arrangement currently edits a shared family chain; independent per-track clip patterns, editable piano-roll notes, automation lanes, save/load projects and recording/overdub are future work. The piano roll is a phrase preview, not yet a note editor.

The GHOULBOX renderer, program-change controls, GHOULBOX lane and GHOULBOX WASM build have been removed from the browser simulator and its deployment workflow. The AC79 reference build still uses the Felucca board startup/audio base because the current repository does not yet contain a replacement AC79 startup, codec and DMA transport implementation. This is a remaining hardware-firmware dependency, not a HYBRID instrument slot. A fully GHOULBOX-free installable AC79 image is **not** claimed.

The AC79 sample slot is reserved in the mixer interface, but the on-device sample-bank loader/player is not implemented yet. Browser sample playback is real PCM through the WebAudio AudioWorklet and HYBRID Q15 mixer; it must not be confused with AC79 sample playback.

## Build and validation

- `src/hybrid_sequencer.c`: bounded shared-clock sequencer.
- `src/hybrid_core.c`: Q15 mixer, routing and scenes.
- `src/hybrid_native_adapters.c`: original AMB/DUB/ZEN AC79 adapters.
- `simulator/audio-worklet.js`: original DSP WASM renderers and eight-slot PCM sampler.
- `simulator/index.html`: DAW-style workstation.
- `tests/test_audio_worklet_browser.js`: Chromium regression for sample imports, non-zero PCM, arrangement, transport, variation, fills and mute/solo.

The latest AC79 ELF baseline previously reported XIP 10,380 B free, RAM 15,056 B, POOL 8,768 B, RAMTEXT 21,540 B and NOINIT 132 B. Those are historical measurements from the earlier linked build; the current source/tool changes require a fresh AC79 run before treating them as current margins. NOINIT and POOL remain critical.

A passing browser test proves the browser path only. It does not certify physical-board sample playback, CPU, stack, DMA underruns or an installable GHOULBOX-free AC79 image.
