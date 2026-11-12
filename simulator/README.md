# SMK-37 HYBRID Virtual Lab

The browser workstation combines the HYBRID C Q15 mixer with a real six-operator FM6/DX7 synthesizer based on Google's MSFA engine, compiled to WebAssembly and rendered inside an AudioWorklet. DUB-1, AMB-1, ZEN-1 and the Sample Browser have been removed from the simulator audio path and deployment workflow.

## Run locally

1. Build the simulator with the GitHub Actions workflow or download its artifact.
2. Run `python3 -m http.server 8000` from the extracted simulator directory.
3. Open `http://localhost:8000` in desktop Chrome or Edge and press **Ligar áudio**.

Use HTTP/HTTPS rather than `file://` so WebAssembly modules can be fetched.

## Current simulator features

- Hardware-inspired SMK-37 workstation layout, transport, tempo and arrangement.
- 37-key virtual keyboard with mouse, touch and computer-keyboard input.
- Real DX7-compatible FM synthesis: six operators, 32 algorithms, per-operator envelopes, velocity response, LFO, pitch envelope and 16-note polyphony.
- Four online DX7 voice banks A–D (128 voices, 32 per bank), loaded into the FM6/DX7 engine and selectable while audio is running.
- Web MIDI note input, program selection, sustain, modulation wheel, aftertouch and pitch bend.
- Chord mode with selectable voicings: major, minor, dominant 7, major 7, minor 7, sus2, sus4, diminished, diminished 7, augmented, major 6, minor 6, half-diminished and dominant 9.
- Note-off releases every note in a chord; switching chord mode off restores single-note playing.
- Shared C Q15 mixer, Web Audio AudioWorklet and MIDI clock support.

## DX7 voice banks and provenance

The included 128 voices come from the public M-VAVE SMK37 OPEN repository, `app/factory_dx7.inc`, documented upstream as extracted from **FM-1_015.fwsc**. They are compatible DX7 VMEM voices, but are **not verified SMK-37 Pro V016 factory banks**. Each 32-voice bank can be exported as a 4,104-byte DX7 bulk SysEx file.

The supplied Pro V016 update package was audited separately. Its JLFS map declares a separate `VM` region at flash address `0x9D000` with size 143,360 bytes, but that region is outside the extracted `flash.bin` image. The package therefore does not contain the device's VM contents needed to recover and verify its factory voice bank. The application image itself does not expose a directly readable DX7 SysEx bank or factory patch-name table. The four online banks are deliberately not labelled as V016 factory banks.

## FM6/DX7 synthesis engine

The simulator vendors the Google MSFA synthesis core from `webaudiomodules/webdx7` (`src/c/msfa`), also used by Dexed. The engine runs in its own WebAssembly module inside the AudioWorklet. Its rendered mono PCM is transferred into mixer channel B as stereo Q15 samples. MIDI and bank/program changes are dispatched directly to the synth without a blocking ring-buffer write on the real-time thread.

The MSFA source retains its original per-file copyright and license headers. The Apache-2.0 license and HYBRID integration changes are documented in `dx7/LICENSE` and `dx7/NOTICE`. This browser build uses standard equal temperament; non-standard tuning features from the full Dexed application are not included.

## Validation limits

CI compiles the MSFA DX7 core to WebAssembly, renders all 128 voices and checks non-zero PCM, bank/program loading, MIDI note dispatch, Q15 mixing, chord voicings and AudioWorklet operation. It does not certify physical AC79 CPU/RAM/POOL/DMA behaviour or operation on the physical keyboard.
