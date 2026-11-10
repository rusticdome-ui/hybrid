#!/usr/bin/env python3
"""Build and run HYBRID native adapters against the real upstream DSP sources on the host.

This deliberately does not compile or link tests/native_port_stubs.c.
"""
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import ac79_link_engines as linker

def run(args):
    print("+", " ".join(map(str, args)), flush=True)
    subprocess.run(list(map(str, args)), check=True)

def main():
    if len(sys.argv) != 4:
        raise SystemExit("usage: test_native_adapters_host.py AMB_DIR DUB_DIR ZEN_DIR")
    amb, dub, zen = map(Path, sys.argv[1:])
    cc = shutil.which(os.environ.get("CC", "cc"))
    nm = shutil.which("nm")
    objcopy = shutil.which("objcopy") or shutil.which("llvm-objcopy")
    if not cc or not nm or not objcopy:
        raise SystemExit("host test requires cc, nm and objcopy")
    with tempfile.TemporaryDirectory(prefix="smk37-hybrid-native-") as td:
        out = Path(td)
        amb_obj, dub_obj, zen_obj = out / "amb.o", out / "dub.o", out / "zen.o"
        common = ["-O2", "-ffp-contract=off", "-std=c99", "-Wall", "-Wextra", "-Werror"]
        run([cc, *common, "-Wdouble-promotion", "-DEN_HOST", "-DBOARD_SMK37=1",
             f"-I{amb / 'firmware/src/dsp'}", "-c", amb / "firmware/src/dsp/amb.c", "-o", amb_obj])
        run([cc, *common, "-Wdouble-promotion", "-DEN_HOST", "-DBOARD_SMK37=1",
             f"-I{dub / 'firmware/src/dsp'}", "-c", dub / "firmware/src/dsp/amb.c", "-o", dub_obj])
        run([cc, *common, "-Wno-unused-function", "-DFM_HOST",
             f"-I{zen / 'firmware/src/dsp'}", "-c", zen / "firmware/src/dsp/fumi.c", "-o", zen_obj])
        linker.namespace("amb_", [amb_obj], Path(nm), Path(objcopy), out)
        linker.namespace("dub_", [dub_obj], Path(nm), Path(objcopy), out)
        linker.namespace("zen_", [zen_obj], Path(nm), Path(objcopy), out)
        for obj, required in ((amb_obj, "amb_en_render"), (dub_obj, "dub_en_render"),
                              (zen_obj, "zen_fm_render")):
            if required not in linker.defined(Path(nm), obj):
                raise SystemExit(f"real native symbol missing after namespace: {required}")
        core_obj, profile_obj, adapter_obj = out / "hybrid_core.o", out / "hybrid_profile.o", out / "hybrid_native_adapters.o"
        inc = [f"-I{ROOT / 'include'}", f"-I{ROOT / 'tests/fakes'}"]
        run([cc, *common, *inc, "-c", ROOT / "src/hybrid_core.c", "-o", core_obj])
        run([cc, *common, *inc, "-c", ROOT / "src/hybrid_profile.c", "-o", profile_obj])
        run([cc, *common, *inc, "-c", ROOT / "src/hybrid_native_adapters.c", "-o", adapter_obj])
        exe = out / "native-adapters-test"
        run([cc, *common, *inc, ROOT / "tests/test_native_adapters_host.c",
             amb_obj, dub_obj, zen_obj, core_obj, profile_obj, adapter_obj, "-lm", "-o", exe])
        run([exe])

if __name__ == "__main__":
    main()