#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile isolated AMB-1, DUB-1 and ZEN-1 AC79 objects and attach them to Felucca."""
import argparse, os, re, subprocess
from pathlib import Path

CFLAGS = ["-target", "pi32v2", "-Os", "-ffunction-sections", "-fdata-sections",
          "-fno-builtin", "-Wall", "-Wno-unused-function", "-DBOARD_SMK37=1"]
ZEN = ("fumi.c", "fm6_core.c", "fm6_patch.c", "tuning.c", "smooth.c", "bend.c", "reverb.c", "bed.c")

def run(args):
    print("+", " ".join(map(str, args)), flush=True)
    subprocess.run(list(map(str, args)), check=True)

def defined(nm, obj):
    out = subprocess.check_output([str(nm), "-g", "--defined-only", str(obj)], text=True)
    result = []
    for line in out.splitlines():
        m = re.match(r"^\s*[0-9a-fA-F]+\s+[A-Za-z]\s+(\S+)$", line)
        if m: result.append(m.group(1))
    return result

def compile_one(clang, src, obj, includes):
    run([clang, *CFLAGS, *[f"-I{x}" for x in includes], "-c", src, "-o", obj])

def namespace(prefix, objects, nm, objcopy, out):
    mapping = {}
    for obj in objects:
        for name in defined(nm, obj):
            mapping.setdefault(name, prefix + name)
    if not mapping: raise SystemExit(f"{prefix}: no global definitions found")
    symmap = out / f"{prefix}symbols.map"
    symmap.write_text("".join(f"{a} {b}\n" for a,b in sorted(mapping.items())))
    for obj in objects:
        tmp = obj.with_suffix(".namespaced.o")
        run([objcopy, f"--redefine-syms={symmap}", obj, tmp])
        tmp.replace(obj)
    return set(mapping.values())

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--amb",type=Path,required=True)
    p.add_argument("--dub",type=Path,required=True)
    p.add_argument("--zen",type=Path,required=True)
    p.add_argument("--felucca",type=Path,required=True)
    p.add_argument("--toolchain",type=Path,default=Path(os.environ.get("JIELI_TOOLCHAIN","")))
    a=p.parse_args()
    tc=a.toolchain.resolve()
    clang,nm,objcopy=tc/"pi32v2/bin/clang",tc/"common/bin/nm",tc/"common/bin/objcopy"
    for tool in (clang,nm,objcopy):
        if not tool.is_file(): raise SystemExit(f"missing JieLi tool: {tool}")
    out=a.felucca/"build/hybrid"; out.mkdir(parents=True,exist_ok=True)
    fwsrc,hal=a.felucca/"firmware/src",a.felucca/"firmware/hal"
    amb_obj,dub_obj=out/"amb.o",out/"dub.o"
    compile_one(clang,a.amb/"amb.c",amb_obj,[a.amb,fwsrc,hal])
    compile_one(clang,a.dub/"amb.c",dub_obj,[a.dub,fwsrc,hal])
    exports=namespace("amb_",[amb_obj],nm,objcopy,out)
    exports |= namespace("dub_",[dub_obj],nm,objcopy,out)
    zen_objs=[]
    for name in ZEN:
        src=a.zen/name
        if not src.is_file(): raise SystemExit(f"ZEN-1 dependency missing: {src}")
        obj=out/(src.stem+".o")
        compile_one(clang,src,obj,[a.zen,fwsrc,hal])
        zen_objs.append(obj)
    exports |= namespace("zen_",zen_objs,nm,objcopy,out)
    required={"amb_en_init","amb_en_set","amb_en_play","amb_en_render","amb_en_panic",
              "dub_en_init","dub_en_set","dub_en_play","dub_en_render","dub_en_panic",
              "zen_fm_init","zen_fm_set","zen_fm_note_vel","zen_fm_render","zen_fm_panic"}
    missing=sorted(required-exports)
    if missing: raise SystemExit("required adapter ABI missing: "+", ".join(missing))
    build_py=a.felucca/"tools/build.py"
    src=build_py.read_text()
    anchor='OUT / "felucca.o", "-o", elf)'
    replacement='OUT / "felucca.o", *sorted((OUT / "hybrid").glob("*.o")), "-o", elf)'
    if replacement not in src:
        if src.count(anchor)!=1: raise SystemExit("Felucca link anchor changed; refusing unsafe patch")
        build_py.write_text(src.replace(anchor,replacement))
    (out/"NAMESPACE-MANIFEST.txt").write_text(
        "Exported DSP globals: "+str(len(exports))+"\nObjects: "+", ".join(x.name for x in [amb_obj,dub_obj,*zen_objs])+"\n")
    print("DSP objects compiled, namespaced and added to Felucca's final AC79 link.")
    print("Audio ISR/mixer integration remains a separate required gate.")

if __name__=="__main__": main()
