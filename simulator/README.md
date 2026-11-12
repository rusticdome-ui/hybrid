# SMK-37 HYBRID Virtual Lab — DAW and PCM sampler

The browser workstation loads the shared C Q15 mixer as WebAssembly and keeps the original AMB-1, DUB-1 and ZEN-1 renderers internally as selectable soundbanks, not as independent instrument panels.

## Run locally

1. Extract `smk37-hybrid-virtual-lab.zip`.
2. Run `python3 -m http.server 8000`.
3. Open `http://localhost:8000` and press **Ligar áudio**.

Use HTTP/HTTPS rather than `file://` so WebAssembly modules can be fetched.

## DAW workstation

The interface includes an arrangement overview, shared family chain, BPM/transport, A/B variations, fills, track mute/solo, phrase preview, mixer and an eight-slot sample browser.

The arrangement edits a shared chain of Bass, Keys, Drum, FM and Sample phrases. Independent per-track clips, editable piano-roll notes, automation, project save/load and recording are not yet implemented.

## PCM sampler

Eight stereo one-shot slots accept audio files decoded by the browser (WAV/AIFF/OGG/MP3 depending on browser support). Each slot is limited to 20 seconds. Audio is resampled to the current AudioContext rate and submitted as Q15 PCM to the fourth HYBRID mixer channel. Samples remain in session memory only.

Sample family notes 36, 38, 42 and 46 trigger sampler slots 1–4. Other slots can be previewed from the sample browser.

## DSP and validation limits

The original AMB-1, DUB-1 and ZEN-1 WASM renderers remain internally mapped to the first three mixer channels. The soundbank selector now uses names and counts read from the original source trees at their `smk37` refs:

- **AMB-1 — 8 presets:** Garden, Departures, Voices, Afternoon, Night Bus, Shoreline, Glass, Tides.
- **DUB-1 — 8 presets:** Basic, Chamber, Steppers, Fog, Concrete, Night Drive, Pressure, Haze.
- **ZEN-1 — 14 selectable FuMi instrument voices:** Koto, Koto II, Sho, Shakuhachi, Dragon Flt, Bamboo Flt, Kokyu, Taiko, Hyoshigi, Mokugyo, Koto Pluck, Koto Ring, Koto Warm, Harp.

ZEN-1 also defines **9 scene-layer patches** (Soft Pad, Soft Swell, Deep Pad, Drop, Mallet, Bell, Rin, Bamboo, Furin). These are assigned to ZEN scene parts, not the main instrument's 14-entry `P_VOICE` bank, so they are disclosed separately rather than incorrectly offered as main-instrument presets. AMB-1 and DUB-1's named entries are the eight `PIECES` presets in each original DSP source. The selector sends relative detents through the original PRESET encoder (role 2). The fourth mixer slot is the PCM sampler; no GHOULBOX renderer is included in the browser app or simulator CI.

The AC79 reference build still uses Felucca for board startup, codec, MIDI/USB and DMA audio transport. The AC79 sample-bank loader/player is not implemented yet. The browser sampler is real PCM playback through the AudioWorklet and C mixer, but it is not evidence of sample playback on the physical SMK-37.

Browser tests do not certify target CPU, stack, POOL, NOINIT, DMA underruns or hardware operation.

## Soundbank patch selection

The independent AMB-1/DUB-1/ZEN-1 engine panels are hidden from the user interface. Select a soundbank in **SOUNDBANK SELECT** and choose a named preset/voice or use Previous/Next. The list is bank-specific and bounded to the source-defined count: 8 AMB-1 presets, 8 DUB-1 presets and 14 ZEN-1 FuMi voices. The ZEN-1 scene-only layer bank contains 9 additional patch definitions, documented above but not mixed into the main instrument selector. Selection is sent as relative detents to the original DSP PRESET encoder; it does not replace or emulate the DSP.
