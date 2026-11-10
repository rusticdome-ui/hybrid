# SMK-37 HYBRID Virtual Lab

Standalone browser simulator for the four-engine HYBRID control surface.

## Run locally

Open `index.html` in a current desktop or mobile browser. Press **Ligar áudio** and allow audio playback. No package install, server, or physical keyboard is required.

## Included

- Four engine strips: AMB-1, DUB-1, ZEN-1 and GHOULBOX.
- Per-engine gain, pan, routing (GLOBAL / ZONE / OFF), mode, mute and solo.
- Master level, MIDI channel selector, upper/lower zone gate, transpose, velocity and crossfade.
- Virtual note keyboard and computer-keyboard note triggers.
- 16 session-only scene slots, chord trigger and 32-note stress trigger.
- Synthetic Web Audio voices and activity meters.

## Important limits

This is a **host-side interaction/audio mock**, not an emulator of the JieLi AC79 target. It does not load the compiled DSP engines, reproduce their exact synthesis/effects, measure target CPU/RAM/POOL/XIP, validate DMA/underruns, or replace firmware/hardware tests. The displayed voice/meter activity is illustrative.

Scenes are kept in page memory only and are lost when the page is reloaded. The MIDI channel selector is a UI control; browser MIDI device I/O is not connected.

## GitHub Pages

The workflow `.github/workflows/deploy-simulator.yml` publishes the `simulator/` directory when the branch is pushed. In repository Settings → Pages, set the build/deployment source to **GitHub Actions**. Once the workflow succeeds, the live URL is shown in the workflow deployment environment.
