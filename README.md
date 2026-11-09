# SMK-37 HYBRID — common core (integration scaffold)

Initial isolated common-core implementation for the M-VAVE SMK-37 Pro hybrid firmware.

## Included
- Four engine slots: A=AMB-1, B=DUB-1, C=ZEN-1, D=GHOULBOX.
- Engine callback contract (init/reset/note/control/render/panic).
- MIDI routing GLOBAL / ZONE / OFF, channel filter, velocity window and transpose.
- Per-channel volume, pan, mute and solo.
- Four-way stereo summing in signed Q15 with output saturation.
- Sixteen in-memory scenes and optional per-engine 64-byte scene-state hooks.
- Linear volume/pan transition; discrete controls switch at fade completion.
- Fixed render quantum up to 128 frames; no heap allocation in audio callback.

## Host test
```sh
cc -std=c99 -Wall -Wextra -Werror -Iinclude src/hybrid_core.c tests/test_hybrid_core.c -o test_hybrid_core
./test_hybrid_core
```

## Integration status and limitations
This is a tested host-side scaffold, **not a complete firmware image**. It does not yet include or instantiate the AMB-1, DUB-1, ZEN-1 or GHOULBOX engine sources, and has not been built with the JieLi SDK or tested on an SMK-37 Pro.

Felucca's build compiles `firmware/src/felucca.c` as a single translation unit; integration must include the core in a deliberate location and connect MIDI/key events and the audio callback. Do not map HYBRID slots to GHOULBOX's four internal tracks: GHOULBOX is one engine in slot D, retaining its own internal engines and sequencer.

Remaining work includes engine adapters, persistent scene storage/versioning/CRC, per-engine FX, equal-power pan, CPU/polyphony arbitration, audio HAL/DMA integration, and note ownership tracking so zone changes cannot strand sustained notes. Scene hooks currently hold at most 64 bytes per engine.

## Licensing
SPDX-License-Identifier: GPL-3.0-only. When importing upstream engine code, retain all copyright, license and third-party notices and verify compatibility for every imported file.


## GitHub Actions: SMK-37 build pipeline

The workflow `.github/workflows/smk37-hybrid.yml` runs on pushes, pull requests and manual dispatch.

- Compiles and runs the isolated HYBRID core host tests.
- Downloads the Linux x86-64 JieLi pi32v2 toolchain.
- Checks out JieLi AC79 SDK `AC79NN_SDK_V1.2.1_2023-12-13`.
- Checks out the upstream `Felucca` `ghoulbox-smk37` branch and builds its SMK-37 Pro reference package with `tools/build.py --board smk37 --model pro`.
- Uploads `ghoulbox-smk37.fwsc` as a 14-day Actions artifact, alongside the upstream source commit identifier.

**Important:** this is currently a GHOULBOX reference package build, not an integrated SMK-37 HYBRID firmware. The hybrid core tests do not imply that AMB-1, DUB-1, ZEN-1 and GHOULBOX have been integrated into the device firmware. Do not flash the reference artifact as the HYBRID build.
