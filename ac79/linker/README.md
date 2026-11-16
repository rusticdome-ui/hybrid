# AC79 HYBRID wavetable linker

This bridge compiles the independent HYBRID C engine and common runtime with the JieLi pi32v2 toolchain, then attaches those objects to the SMK-37 AC79 board audio build. It does not fetch, compile or link any external instrument DSP source.

## Objects

- `hybrid_core.o`: Q15 mixer, routing, scenes and PCM ingress.
- `hybrid_wavetable.o`: dual-oscillator wavetable instrument, envelopes, filter and LFO.
- `hybrid_native_adapters.o`: board audio, MIDI and sequencer bridge.
- `hybrid_profile.o`: TIMER4 and memory diagnostics.
- `hybrid_sequencer.o`: bounded shared-clock sequencer.

## Run

```sh
python3 tools/ac79_link_engines.py \
  --felucca /src/Felucca \
  --toolchain "$JIELI_TOOLCHAIN"
python3 /src/Felucca/tools/build.py --board smk37 --model pro --sdk "$AC79_SDK"
```

The helper fails closed if the toolchain, source files or expected board linker/audio anchors are missing. It emits a HYBRID Wavetable manifest and places HYBRID object BSS in the linker POOL region.

The output remains validation-only until the package identity and flashing path are replaced and verified. CI cannot measure live board CPU, stack, DMA underruns or long-run hardware stability.
