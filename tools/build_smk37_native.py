#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build and inspect a Felucca-free SMK-37 Pro HYBRID application ELF."""
import argparse
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFLAGS = [
    "-target", "pi32v2", "-mcpu=r3", "-mfprev1", "-Oz",
    "-ffp-contract=off", "-ffreestanding", "-fno-builtin",
    "-fno-unwind-tables", "-fno-asynchronous-unwind-tables",
    "-ffunction-sections", "-fdata-sections", "-fno-common",
    "-DHYBRID_ENABLE_PROFILE=1", "-DHYBRID_NATIVE_STANDALONE=1",
    "-Wall", "-Wextra", "-Werror",
]
SOURCES = [
    "ac79/native/src/hybrid_native_entry.S",
    "ac79/native/src/hybrid_native_memory.c",
    "ac79/native/src/hybrid_native_libc.c",
    "ac79/native/src/hybrid_native_startup.c",
    "ac79/native/src/hybrid_native_irq.c",
    "ac79/native/src/hybrid_native_board.c",
    "src/hybrid_core.c",
    "src/hybrid_wavetable.c",
    "src/hybrid_native_adapters.c",
    "src/hybrid_profile.c",
    "src/hybrid_sequencer.c",
    "ac79/platform/src/hybrid_ac79_audio.c",
    "ac79/platform/src/hybrid_ac79_audio_board.c",
]


def run(args):
    print("+", " ".join(map(str, args)), flush=True)
    subprocess.run([str(x) for x in args], check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--toolchain", type=Path, required=True)
    parser.add_argument("--sdk", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    tc, sdk, out = args.toolchain.resolve(), args.sdk.resolve(), args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    clang, linker, objdump = tc/"pi32v2/bin/clang", tc/"pi32v2/bin/ld", tc/"common/bin/objdump"
    for tool in (clang, linker, objdump):
        if not tool.is_file():
            raise SystemExit(f"missing JieLi tool: {tool}")
    sfc, hwi = sdk/"cpu/wl82/sdk_ld_sfc.c", sdk/"include_lib/driver/cpu/wl82/asm/hwi.h"
    if not sfc.is_file() or not hwi.is_file():
        raise SystemExit("pinned SDK checkout is missing sdk_ld_sfc.c or WL82 hwi.h")
    sfc_text, hwi_text = sfc.read_text(errors="replace"), hwi.read_text(errors="replace")
    if not re.search(r"ORIGIN\s*=\s*0x2000120", sfc_text, re.I):
        raise SystemExit("SDK SFC linker entry 0x02000120 was not confirmed")
    if not re.search(r"ISR_ENTRY\s+0x1c7fe00", hwi_text, re.I):
        raise SystemExit("SDK WL82 ISR_ENTRY 0x01c7fe00 was not confirmed")
    if not re.search(r"IRQ_ALNK_IDX\s+11", hwi_text):
        raise SystemExit("SDK ALNK0 source IRQ 11 was not confirmed")
    includes = [
        "-I"+str(ROOT/"include"),
        "-I"+str(ROOT/"ac79/native/include"),
        "-I"+str(ROOT/"ac79/platform/include"),
    ]
    objects = []
    for rel in SOURCES:
        src = ROOT/rel
        if not src.is_file():
            raise SystemExit(f"missing native source: {src}")
        obj = out/(src.stem+(".entry.o" if src.suffix==".S" else ".o"))
        cmd = ([clang,"-target","pi32v2","-mcpu=r3","-mfprev1","-c",src,"-o",obj]
               if src.suffix==".S" else [clang,*CFLAGS,*includes,"-c",src,"-o",obj])
        run(cmd)
        objects.append(obj)
    elf, map_file = out/"smk37-hybrid-native.elf", out/"smk37-hybrid-native.map"
    script = ROOT/"ac79/native/linker/smk37-hybrid-native.ld"
    run([linker,"--gc-sections","-T",script,*objects,"-Map",map_file,"-o",elf])
    file_text = subprocess.check_output([str(objdump),"-f",str(elf)],text=True,errors="replace")
    sym_text = subprocess.check_output([str(objdump),"-t",str(elf)],text=True,errors="replace")
    sec_text = subprocess.check_output([str(objdump),"-h",str(elf)],text=True,errors="replace")
    if not re.search(r"start address 0x0*2000120", file_text, re.I):
        raise SystemExit("ELF entry point is not 0x02000120")
    (out/"symbols.txt").write_text(sym_text)
    (out/"sections.txt").write_text(sec_text)
    required = [
        "_start","hybrid_native_reset_c","hybrid_native_irq_init",
        "hybrid_native_irq_attach","hybrid_native_main","hybrid_audio_init",
        "hybrid_ac79_audio_startup","hybrid_ac79_audio_irq","hybrid_wavetable_ops",
    ]
    missing = [name for name in required if not re.search(r"\b"+re.escape(name)+r"\b",sym_text)]
    if missing:
        raise SystemExit("native ELF missing required symbols: "+", ".join(missing))
    forbidden = re.findall(r"\b(?:fm1_[A-Za-z0-9_]*|felucca_[A-Za-z0-9_]*)\b",sym_text)
    if forbidden:
        raise SystemExit("Felucca dependency leaked into native ELF: "+", ".join(sorted(set(forbidden))))
    for section in (".entry",".text",".hybrid_dma",".hybrid_bss",
                    ".hybrid_copy_table",".hybrid_zero_table"):
        if section not in sec_text:
            raise SystemExit(f"native ELF missing section {section}")
    sdk_commit = subprocess.check_output(["git","-C",str(sdk),"rev-parse","HEAD"],text=True).strip()
    report = [
        "# SMK-37 HYBRID native ELF", "",
        "- Target: SMK-37 Pro / AC79 WL82, pi32v2 r3",
        "- Entry ABI: m-bootloader application hand-off at 0x02000120 (not mask-ROM reset)",
        f"- Pinned SDK checkout: {sdk_commit}",
        "- SDK SFC entry confirmed from cpu/wl82/sdk_ld_sfc.c",
        "- SDK peripheral vector base confirmed from hwi.h: 0x01c7fe00",
        "- ALNK0 source confirmed from hwi.h: IRQ 11",
        "- Flash window: 0x02000120 + 0x98ee0 bytes",
        "- Application RAM: 0x01c40000 + 0x3c000 bytes",
        "- Link inputs: HYBRID core, wavetable, sequencer, profiler, native ALNK0/DMA and native startup only",
        "- No Felucca objects, startup, libraries or calls are linked",
        "- ELF validation only; not a packaged or flashable firmware release",
        "- Physical SMK-37 Pro boot/audio validation remains pending", "",
        "## Required symbols", "",
        *[f"- {name}" for name in required], "",
        "## Memory sections", "", sec_text.rstrip(),
    ]
    (out/"NATIVE-ELF-REPORT.md").write_text("\n".join(report)+"\n")
    print(f"Native ELF validated: {elf}")


if __name__ == "__main__":
    main()
