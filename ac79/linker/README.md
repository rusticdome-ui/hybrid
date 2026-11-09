# AC79 isolated DSP linker

This bridge compiles AMB-1, DUB-1 and ZEN-1 as separate pi32v2 objects, namespaces engine-owned global symbols with JieLi `nm`/`objcopy`, and patches the checked-out Felucca `tools/build.py` final link to include those objects.

AMB-1 and DUB-1 both export `en_*` with incompatible parameter enums/state. They are compiled separately and exported as `amb_en_*` and `dub_en_*`. ZEN-1 is compiled from FuMi, FM6, tuning, smoothing, bend, reverb and bed sources and exported under `zen_*`.

## Run

```sh
python3 tools/ac79_link_engines.py \
  --amb /src/fm1-amb/firmware/src/dsp \
  --dub /src/fm1-dub/firmware/src/dsp \
  --zen /src/zen-1/firmware/src/dsp \
  --felucca /src/Felucca \
  --toolchain "$JIELI_TOOLCHAIN"
python3 /src/Felucca/tools/build.py --board smk37 --model pro
```

The helper fails closed if a source, tool, required adapter symbol, or expected Felucca linker anchor is missing. It emits `build/hybrid/NAMESPACE-MANIFEST.txt`.

**Important:** object-level linking does not yet connect the three adapters to Felucca's audio ISR/mixer. It is not yet a complete HYBRID firmware and must not be flashed as one. RAM/pool and runtime CPU headroom, audio callback registration, GHOULBOX slot D preservation and hardware testing remain gates. Upstream GPL-3.0-only notices must be retained.
