# SMK-37 HYBRID Virtual Lab — DAW and PCM sampler

The browser workstation loads the shared C Q15 mixer as WebAssembly and the original AMB-1, DUB-1 and ZEN-1 renderers as independent WASM modules.

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

The original AMB-1, DUB-1 and ZEN-1 WASM renderers are used for the first three mixer channels. The fourth is the PCM sampler; no GHOULBOX renderer is included in the browser app or simulator CI.

The AC79 reference build still uses Felucca for board startup, codec, MIDI/USB and DMA audio transport. The AC79 sample-bank loader/player is not implemented yet. The browser sampler is real PCM playback through the AudioWorklet and C mixer, but it is not evidence of sample playback on the physical SMK-37.

Browser tests do not certify target CPU, stack, POOL, NOINIT, DMA underruns or hardware operation.
