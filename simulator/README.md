# SMK-37 HYBRID Virtual Lab

The browser workstation uses the HYBRID C Q15 mixer as WebAssembly and the original DUB-1 host renderer as its only remaining legacy DSP. AMB-1, ZEN-1 and the Sample Browser have been removed from the simulator interface and browser audio-loading path.

## Run locally

1. Build the simulator with the GitHub Actions workflow or download its artifact.
2. Run `python3 -m http.server 8000` from the extracted simulator directory.
3. Open `http://localhost:8000` in desktop Chrome or Edge and press **Ligar áudio**.

Use HTTP/HTTPS rather than `file://` so WebAssembly modules can be fetched.

## Current simulator features

- Hardware-inspired SMK-37 workstation layout, transport, tempo and arrangement.
- 37-key virtual keyboard with mouse, touch and computer-keyboard input.
- Selectable DUB-1 source presets, with the eight names defined in the original DUB-1 source tree: Basic, Chamber, Steppers, Fog, Concrete, Night Drive, Pressure and Haze.
- Chord mode with selectable voicings: major, minor, dominant 7, major 7, minor 7, sus2, sus4, diminished, diminished 7, augmented, major 6, minor 6, half-diminished and dominant 9.
- Note-off releases every note in a chord; switching chord mode off restores single-note playing.
- Shared C Q15 mixer, Web Audio AudioWorklet and MIDI input/clock support.

DUB-1's eight source-defined preset names are not represented as M-VAVE factory presets. The SMK-37 Pro V016 factory bank is a separate resource and is not yet installed in the simulator.

## SMK-37 Pro V016 firmware audit

The supplied file was identified as the Pro V016 package. The offline FWSC extraction recovered and decoded the application image:

- Package SHA-256: `2d98ce72530e71d617384963423820536f094501299e2cfec0a8d3c3fb6bcfb0`
- Decoded application image: 619,704 bytes
- Decoded application SHA-256: `0f64fbf6ffc454eea686d78cf89f35e593229765bdcf166f10a8bd714abcb09a`
- Extracted `flash.bin`: 638,976 bytes

The package's JLFS map declares a separate `VM` region at flash address `0x9D000` with size 143,360 bytes, but that region is outside the extracted `flash.bin` image. The package therefore does not contain the device's VM contents needed to recover and verify the factory voice bank. The application image itself does not expose a directly readable DX7 SysEx bank or factory patch-name table.

**No factory soundbank has been fabricated or substituted.** The 128-voice resource in the public SMK37 Open source is documented as having been extracted from the FM-1 V015 recovery package, not this SMK-37 Pro V016 package; it is therefore not labelled as the Pro V016 factory bank here.

To complete the original-bank integration, export the four factory DX7 voice groups A–D from the physical SMK-37 Pro as standard 32-voice SysEx files (`.syx`) using the device's voice/library export function, then provide those four files. They can then be checked for SysEx structure, voice count, names, checksum and duplicates before being added as the genuine factory soundbank.

## Validation limits

The browser CI validates the original DUB-1 WASM renderer, note dispatch, PCM transfer, Q15 mixing, chord voicings and AudioWorklet operation. It does not certify physical AC79 CPU/RAM/POOL/DMA behaviour or operation on the physical keyboard.
