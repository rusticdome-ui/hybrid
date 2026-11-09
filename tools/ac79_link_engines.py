#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile isolated AMB-1, DUB-1 and ZEN-1 AC79 objects and attach them to Felucca."""
import argparse, os, re, shutil, subprocess
from pathlib import Path

CFLAGS = ["-target", "pi32v2", "-Os", "-ffunction-sections", "-fdata-sections",
          "-fno-builtin", "-Wall", "-Wno-unused-function", "-DBOARD_SMK37=1"]
# fumi.c textually includes tuning.c, smooth.c, bend.c, fm6_core.c, fm6_patch.c, reverb.c and bed.c. Compile it once to avoid duplicate definitions.
ZEN = ("fumi.c",)

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
    p.add_argument("--hybrid",type=Path,default=Path(__file__).resolve().parents[1])
    a=p.parse_args()
    tc=a.toolchain.resolve()
    clang=tc/"pi32v2/bin/clang"
    def find_tool(candidates, fallback):
        for candidate in candidates:
            if candidate.is_file() and os.access(candidate, os.X_OK):
                return candidate
        found=shutil.which(fallback)
        if found:
            return Path(found)
        raise SystemExit(f"missing {fallback}; checked: "+", ".join(map(str,candidates)))
    # JieLi releases may omit common/bin/nm and common/bin/objcopy. LLVM host
    # tools can inspect and rewrite the symbol table in pi32v2 ELF objects.
    nm_candidates=[tc/"common/bin/nm",tc/"pi32v2/bin/llvm-nm",*tc.rglob("nm"),*tc.rglob("llvm-nm")]
    objcopy_candidates=[tc/"common/bin/objcopy",tc/"pi32v2/bin/llvm-objcopy",*tc.rglob("objcopy"),*tc.rglob("llvm-objcopy")]
    nm=find_tool(nm_candidates,"llvm-nm")
    # JieLi common/bin/objcopy rejects --redefine-syms; prefer host LLVM.
    host_objcopy=shutil.which("llvm-objcopy")
    objcopy=Path(host_objcopy) if host_objcopy else find_tool(
        [tc/"pi32v2/bin/llvm-objcopy",*tc.rglob("llvm-objcopy")],"llvm-objcopy")
    if not clang.is_file(): raise SystemExit(f"missing JieLi compiler: {clang}")
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

    # Compile the common mixer and the live adapter bridge into the application.
    hybrid_inc=a.hybrid/"include"
    hybrid_core_obj=out/"hybrid_core.o"
    hybrid_adapter_obj=out/"hybrid_native_adapters.o"
    hybrid_profile_obj=out/"hybrid_profile.o"
    compile_one(clang,a.hybrid/"src/hybrid_core.c",hybrid_core_obj,[hybrid_inc,fwsrc,hal])
    compile_one(clang,a.hybrid/"src/hybrid_native_adapters.c",hybrid_adapter_obj,[hybrid_inc,fwsrc,hal])
    compile_one(clang,a.hybrid/"src/hybrid_profile.c",hybrid_profile_obj,[hybrid_inc,fwsrc,hal])

    required={"amb_en_init","amb_en_set","amb_en_play","amb_en_render","amb_en_panic",
              "dub_en_init","dub_en_set","dub_en_play","dub_en_render","dub_en_panic",
              "zen_fm_init","zen_fm_set","zen_fm_note_vel","zen_fm_render","zen_fm_panic"}
    missing=sorted(required-exports)
    if missing: raise SystemExit("required adapter ABI missing: "+", ".join(missing))
    build_py=a.felucca/"tools/build.py"
    src=build_py.read_text()
    anchor='''    tc("pi32v2/bin/ld", "-T", ld, OUT / "crt0.o", OUT / "fm1_vec.o", OUT / "fm1_isr.o",
       OUT / "felucca.o", "-o", elf)'''
    zen_link_args = ", ".join(f'OUT / "hybrid" / "{Path(name).stem}.o"' for name in ZEN)
    replacement = f"""    tc("pi32v2/bin/ld", "-T", ld, OUT / "crt0.o", OUT / "fm1_vec.o", OUT / "fm1_isr.o",
       OUT / "felucca.o", OUT / "hybrid" / "amb.o", OUT / "hybrid" / "dub.o",
       {zen_link_args}, OUT / "hybrid" / "hybrid_core.o",
       OUT / "hybrid" / "hybrid_native_adapters.o", OUT / "hybrid" / "hybrid_profile.o", "-o", elf)"""
    if replacement not in src:
        if src.count(anchor)!=1:
            raise SystemExit("Felucca AC79 linker invocation changed; refusing unsafe patch")
        build_py.write_text(src.replace(anchor,replacement))

    def patch_source(path, old_text, new_text, label):
        text = path.read_text()
        if new_text in text:
            return
        if text.count(old_text) != 1:
            raise SystemExit(f"{label}: expected exactly one source anchor")
        path.write_text(text.replace(old_text, new_text))

    audio_c=a.felucca/"firmware/src/audio.c"
    patch_source(audio_c, "/* registers: hal/fm1_audio.h */",
                 "/* registers: hal/fm1_audio.h */\nextern void hybrid_audio_process(int32_t *, uint32_t, int32_t);",
                 "audio.c declaration")
    patch_source(audio_c, "    mix_block(out, n);",
                 "    mix_block(out, n);\n    hybrid_audio_process(out, n, fx_usb_fixed ? MASTER_FULL : (int32_t)song.master_q12);",
                 "audio.c mixer hook")

    main_c=a.felucca/"firmware/src/main.c"
    patch_source(main_c, "static void felucca_init(void)",
                 "extern int hybrid_audio_init(void);\n\nstatic void felucca_init(void)",
                 "main.c declaration")
    patch_source(main_c, "    gb_scene_load(ui_scene);",
                 "    gb_scene_load(ui_scene);\n    (void)hybrid_audio_init();",
                 "main.c engine init")

    seq_c=a.felucca/"firmware/src/seq.c"
    patch_source(seq_c, "static void keyboard_block(void)",
                 "extern void hybrid_audio_note_on(uint8_t, uint8_t);\nextern void hybrid_audio_note_off(uint8_t);\n\nstatic void keyboard_block(void)",
                 "seq.c declarations")
    patch_source(seq_c, "            key_on(k, &trk[kb_trk[k]]);",
                 "            key_on(k, &trk[kb_trk[k]]);\n            hybrid_audio_note_on(kb_note[k], (uint8_t)key_on_v);",
                 "seq.c note-on")
    patch_source(seq_c, "            key_off(k, &trk[kb_trk[k] % NTRK]);",
                 "            key_off(k, &trk[kb_trk[k] % NTRK]);\n            hybrid_audio_note_off(kb_note[k]);",
                 "seq.c note-off")

    (out/"NAMESPACE-MANIFEST.txt").write_text(
        "Exported DSP globals: "+str(len(exports))+"\nObjects: "+", ".join(x.name for x in [amb_obj,dub_obj,*zen_objs,hybrid_core_obj,hybrid_adapter_obj,hybrid_profile_obj])+"\n")
    print("DSP objects compiled, namespaced and added to Felucca's final AC79 link.")
    print("HYBRID mixer hooks installed: GHOULBOX native mix plus AMB-1, DUB-1 and ZEN-1 overlays.")\n    print("AC79 runtime profiler linked: per-engine TIMER4 timing, stack samples, buffer registration and DMA underruns.")

if __name__=="__main__": main()
