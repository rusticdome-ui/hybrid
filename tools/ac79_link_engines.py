#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile isolated AMB-1, DUB-1 and ZEN-1 AC79 objects and attach them to Felucca."""
import argparse, os, re, shutil, subprocess
from pathlib import Path

CFLAGS = ["-target", "pi32v2", "-mcpu=r3", "-mfprev1", "-ffp-contract=off", "-Oz", "-fno-unroll-loops", "-ffunction-sections", "-fdata-sections",
          "-fno-common", "-fno-builtin", "-Wall", "-Wno-unused-function", "-DBOARD_SMK37=1"]
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



def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected one anchor, found {count}")
    return text.replace(old, new)


def apply_memory_profile(amb, dub, zen, felucca):
    """Compact profile: preserve all four engines, trim duplicate ambience DSP memory."""
    for root, label in ((amb, "AMB-1"), (dub, "DUB-1")):
        path = root / "amb.c"
        src = path.read_text()
        start = src.find("#define DS(n) (((n) * 14818 + 5000) / 10000)")
        end = src.find("/* ------------------------------------------------- echo: tape", start)
        if start < 0 or end < 0:
            raise SystemExit(f"{label}: Dattorro plate section anchors changed")
        stubs = """/* HYBRID compact profile: plate reverb is omitted; GHOULBOX remains the native reverb engine. */
static float pl_decay = 0.5f, pl_decay_t = 0.5f, pl_dd2 = 0.5f;
static void plate_init(void) {}
static inline void reverb(float in, float *ol, float *or_)
{
    (void)in;
    *ol = 0.0f;
    *or_ = 0.0f;
}

"""
        src = src[:start] + stubs + src[end:]
        src = replace_once(src, "#define TAPE_N 62000", "#define TAPE_N 6200", f"{label} tape capacity")
        src = replace_once(src, "#define PLEX_N 7100", "#define PLEX_N 4096", f"{label} Echoplex capacity")
        if label == "DUB-1":
            src = replace_once(src, "#define CL_LEN 8192", "#define CL_LEN 4096", "DUB cloud capacity")
        path.write_text(src)

    # ZEN keeps its FM instrument, patches, keyboard voices and its tape/depth tone shaping.
    # Its optional nature-recording bed/sample bank and private plate are removed in compact mode.
    fumi = zen / "fumi.c"
    src = fumi.read_text()
    src = replace_once(src, '#include "reverb.c"\n', "", "ZEN reverb include")
    src = replace_once(src, '#include "bed.c"                                   /* (needs BLK) */',
                       """/* HYBRID compact profile: omit the optional sample/ambience bed. */
static void plate_init(void) {}
static void plate_size(float v01) { (void)v01; }
static inline void reverb(float in, float *ol, float *or_)
{
    (void)in;
    *ol = 0.0f;
    *or_ = 0.0f;
}
static void bed_init(void) {}
static void bed_control(void) {}
static void bed_render(float *ml, float *mr) { (void)ml; (void)mr; }
static void bed_set(int which, int pct) { (void)which; (void)pct; }
static void bed_mod(int which, int pct) { (void)which; (void)pct; }
static void bed_style(int which, int style) { (void)which; (void)style; }
static void shot_start(int i, int seg, int vel, int pan, float rate)
{ (void)i; (void)seg; (void)vel; (void)pan; (void)rate; }
static void bed_drop(int vel, int pan, int pad) { (void)vel; (void)pan; (void)pad; }""",
                       "ZEN bed include")
    src = replace_once(src, "    bed.drip = 0;                                 /* (and a shower's drips: silence is silence) */\n    for (i = 0; i < BED_DROPS; i++)\n        bed.d[i].left = 0;",
                      "    /* Optional sample-bed state is absent in HYBRID compact profile. */",
                      "ZEN quiet bed state")
    src = replace_once(src, "            for (i = 0; i < FM_NSHOT; i++)              /* and the frogs' calls */\n                bed.shot[i].data = 0;\n            bed.drip = 0;                          /* and a shower's drips (the scene is left) */",
                      "            /* Optional sample-bed voices are not part of the HYBRID compact profile. */",
                      "ZEN voice-all-off bed state")
    src = replace_once(src, "    bed_control();\n    bed_render(mix[0], mix[1]);",
                      "    /* Optional sample-bed render omitted in HYBRID compact profile. */",
                      "ZEN bed render")
    src = replace_once(src, "        case C_BED: bed_set(c.a, c.b); break;\n        case C_BED_MOD: bed_mod(c.a, c.b); break;\n        case C_BED_STYLE: bed_style(c.a, c.b); break;\n        case C_FROG: shot_start(c.a, c.b, c.vel, c.pan, c.f); break;\n        case C_DROP: bed_drop(c.vel, c.pan, c.what); break;",
                      "        case C_BED: case C_BED_MOD: case C_BED_STYLE: case C_FROG: case C_DROP: break;",
                      "ZEN bed command dispatch")
    fumi.write_text(src)

    # Reduce GHOULBOX's shared delay/HALL ring to half size, scaling every HALL tap and base.
    fx = felucca / "firmware/src/fx.c"
    src = fx.read_text()
    src = replace_once(src, "#define DLY_LEN 65536u", "#define DLY_LEN 32768u", "GHOULBOX delay capacity")
    replacements = {
        "static const uint16_t HL_D[H_N] = {2792, 189, 142, 504, 368, 917, 5922, 2394, 4948, 1231, 5609, 3532, 4207};":
        "static const uint16_t HL_D[H_N] = {1368, 93, 70, 247, 180, 449, 2902, 1173, 2425, 603, 2748, 1731, 2061};",
        "#define HL_B_I1 2793u": "#define HL_B_I1 1369u",
        "#define HL_B_I2 2983u": "#define HL_B_I2 1463u",
        "#define HL_B_I3 3126u": "#define HL_B_I3 1534u",
        "#define HL_B_I4 3631u": "#define HL_B_I4 1782u",
        "#define HL_B_AL 4000u": "#define HL_B_AL 1963u",
        "#define HL_B_D1L 4918u": "#define HL_B_D1L 2413u",
        "#define HL_B_APL 10841u": "#define HL_B_APL 5316u",
        "#define HL_B_D2L 13236u": "#define HL_B_D2L 6490u",
        "#define HL_B_AR 18185u": "#define HL_B_AR 8916u",
        "#define HL_B_D1R 19417u": "#define HL_B_D1R 9520u",
        "#define HL_B_APR 25027u": "#define HL_B_APR 12269u",
        "#define HL_B_D2R 28560u": "#define HL_B_D2R 14001u",
        "#define HL_AL 894u": "#define HL_AL 438u",
        "#define HL_AR 1208u": "#define HL_AR 592u",
        "#define HL_EXC 21": "#define HL_EXC 10",
        "_Static_assert(HL_B_D2R + 4207u + 1u == DLY_LEN / 2u, \"HALL's lines fill the back half of dly_buf\");":
        "_Static_assert(HL_B_D2R + 2061u + 1u <= DLY_LEN / 2u, \"HALL's compact lines fit the back half of dly_buf\");"
    }
    for old, new in replacements.items():
        src = replace_once(src, old, new, "GHOULBOX HALL compact geometry")
    fx.write_text(src)

    # Place HYBRID object .bss in POOL, leaving the scarce 96 KiB RAM for Felucca itself.
    ld = felucca / "firmware/app.ld"
    src = ld.read_text()
    old = """    .bss (NOLOAD) : { _bss_start = .; *(.bss*) *(.sbss*) *(COMMON) . = ALIGN(4); _bss_end = .; } > RAM
    .pool (NOLOAD) : { _pool_start = .; *(.pool*) . = ALIGN(4); _pool_end = .; } > POOL"""
    new = """    .pool (NOLOAD) : {
        _pool_start = .;
        *(.pool*)
        build/hybrid/amb.o(.bss* COMMON)
        build/hybrid/dub.o(.bss* COMMON)
        build/hybrid/fumi.o(.bss* COMMON)
        build/hybrid/hybrid_core.o(.bss* COMMON)
        build/hybrid/hybrid_native_adapters.o(.bss* COMMON)
        build/hybrid/hybrid_profile.o(.bss* COMMON)
        build/hybrid/hybrid_sequencer.o(.bss* COMMON)
        . = ALIGN(4); _pool_end = .;
    } > POOL
    .bss (NOLOAD) : { _bss_start = .; *(.bss*) *(.sbss*) *(COMMON) . = ALIGN(4); _bss_end = .; } > RAM"""
    src = replace_once(src, old, new, "Felucca linker HYBRID BSS placement")
    ld.write_text(src)

def apply_volca_patterns(felucca):
    """Append five Volca-inspired factory phrases without changing existing pattern IDs."""
    path = felucca / "firmware/src/engines.c"
    src = path.read_text()
    names = ("VOLCBASS", "VOLCKEYS", "VOLCDRUM", "VOLCFM", "VOLCSAMP")
    present = [name for name in names if ('{"' + name + '"') in src]
    if len(present) == len(names):
        return
    if present:
        raise SystemExit("Partial Volca pattern set found; refusing duplicate or incomplete insertion")
    anchor = '''    {"TOMBBEAT", {36, 42, 0, 42, 45, 42, 36, 42, 36, 42, 45, 45, 38, 42, 39, 42},       /* 22 a procession */
     {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0}},
'''
    addition = anchor + '''    /* SMK-37 VOLCA HYBRID: appended IDs 23..27; existing factory IDs stay stable. */
    {"VOLCBASS", {36, 36, 43, 36, 0, 36, 46, 36, 36, 0, 48, 43, 36, 36, 0, 34},
     {1, 0, 2, 0, 0, 1, 0, 2, 1, 0, 0, 2, 1, 0, 0, 1}},
    {"VOLCKEYS", {60, 0, 64, 67, 0, 64, 67, 71, 60, 0, 64, 67, 0, 65, 69, 72},
     {1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0}},
    {"VOLCDRUM", {36, 42, 42, 42, 38, 42, 36, 42, 36, 42, 42, 42, 38, 42, 46, 42},
     {1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0}},
    {"VOLCFM",   {48, 55, 60, 64, 67, 64, 60, 55, 50, 57, 62, 65, 69, 65, 62, 57},
     {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}},
    {"VOLCSAMP", {36, 0, 38, 0, 42, 42, 0, 46, 36, 0, 40, 0, 42, 0, 46, 0},
     {1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0}},
'''
    if src.count(anchor) != 1:
        raise SystemExit("GHOULBOX factory pattern anchor changed; refusing unsafe Volca pattern insertion")
    src = src.replace(anchor, addition)
    pattern_re = re.compile(r'\{"(VOLCBASS|VOLCKEYS|VOLCDRUM|VOLCFM|VOLCSAMP)"\s*,\s*\{([^}]*)\}\s*,\s*\{([^}]*)\}')
    records = {m.group(1): m for m in pattern_re.finditer(src)}
    if set(records) != set(names):
        raise SystemExit("Volca factory pattern names or records are incomplete")
    for name, match in records.items():
        notes = [int(x.strip()) for x in match.group(2).split(",")]
        flags = [int(x.strip()) for x in match.group(3).split(",")]
        if len(notes) != 16 or len(flags) != 16:
            raise SystemExit(f"{name}: expected exactly 16 note and 16 flag values")
        if any(x < 0 or x > 127 for x in notes) or any(x < 0 or x > 7 for x in flags):
            raise SystemExit(f"{name}: note/flag value outside supported range")
    path.write_text(src)
    print("Volca factory patterns appended: " + ", ".join(names))


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
    apply_memory_profile(a.amb, a.dub, a.zen, a.felucca)
    apply_volca_patterns(a.felucca)
    # Compact the native GHOULBOX translation unit as well as the added DSPs.
    build_py=a.felucca/"tools/build.py"
    build_src=build_py.read_text()
    build_src=replace_once(build_src,
        'CFLAGS = ["-Os", "-ffunction-sections", "-fno-builtin", "-Wall", "-Wno-unused-function"]',
        'CFLAGS = ["-mcpu=r3", "-mfprev1", "-ffp-contract=off", "-Oz", "-fno-unroll-loops", "-ffunction-sections", "-fdata-sections", "-fno-builtin", "-Wall", "-Wno-unused-function"]',
        "Felucca size-optimized section flags")
    build_py.write_text(build_src)

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
    hybrid_seq_obj=out/"hybrid_sequencer.o"
    compile_one(clang,a.hybrid/"src/hybrid_core.c",hybrid_core_obj,[hybrid_inc,fwsrc,hal])
    compile_one(clang,a.hybrid/"src/hybrid_native_adapters.c",hybrid_adapter_obj,[hybrid_inc,fwsrc,hal])
    compile_one(clang,a.hybrid/"src/hybrid_profile.c",hybrid_profile_obj,[hybrid_inc,fwsrc,hal])
    compile_one(clang,a.hybrid/"src/hybrid_sequencer.c",hybrid_seq_obj,[hybrid_inc,fwsrc,hal])

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
    replacement = f"""    tc("pi32v2/bin/ld", "--gc-sections", "-T", ld, OUT / "crt0.o", OUT / "fm1_vec.o", OUT / "fm1_isr.o",
       OUT / "felucca.o", OUT / "hybrid" / "amb.o", OUT / "hybrid" / "dub.o",
       {zen_link_args}, OUT / "hybrid" / "hybrid_core.o",
       OUT / "hybrid" / "hybrid_native_adapters.o", OUT / "hybrid" / "hybrid_profile.o",
       OUT / "hybrid" / "hybrid_sequencer.o",
       "-Map", OUT / "felucca.map", "-o", elf)"""
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
                 "/* registers: hal/fm1_audio.h */\nextern void hybrid_audio_process(int32_t *, uint32_t, int32_t);\nextern void hybrid_profile_begin(int);\nextern void hybrid_profile_end(int, uint32_t);\nextern void hybrid_profile_underrun(void);",
                 "audio.c declaration")
    patch_source(audio_c, "    mix_block(out, n);",
                 "    hybrid_profile_begin(3);\n    mix_block(out, n);\n    hybrid_profile_end(3, n);\n    hybrid_audio_process(out, n, fx_usb_fixed ? MASTER_FULL : (int32_t)song.master_q12);",
                 "audio.c mixer hook")

    main_c=a.felucca/"firmware/src/main.c"
    patch_source(main_c, "static void felucca_init(void)",
                 "extern int hybrid_audio_init(void);\n\nstatic void felucca_init(void)",
                 "main.c declaration")
    patch_source(main_c, "    gb_scene_load(ui_scene);",
                 "    gb_scene_load(ui_scene);\n    (void)hybrid_audio_init();",
                 "main.c engine init")

    usb_c=a.felucca/"firmware/src/usb.c"
    patch_source(usb_c, "    mo_w++;\n    }\n}\n\n/* ------------------------------------------------------- descriptors --- */",
                 "    mo_w++;\n    }\n}\n\nvoid hybrid_midi_out_event(uint32_t pkt) { midi_out_event(pkt); }\n\n/* ------------------------------------------------------- descriptors --- */",
                 "USB MIDI output bridge")

    seq_c=a.felucca/"firmware/src/seq.c"
    patch_source(seq_c, "static void keyboard_block(void)",
                 "extern void hybrid_audio_note_on(uint8_t, uint8_t);\nextern void hybrid_audio_note_off(uint8_t);\nextern void hybrid_audio_panic(void);\n\nstatic void keyboard_block(void)",
                 "seq.c declarations")
    patch_source(seq_c, "    for (i = 0; i < NTRK; i++) {\n        track_t *t = &trk[i];\n        if (((pr >> i) & 1u)",
                 "    if (pr) hybrid_audio_panic();\n    for (i = 0; i < NTRK; i++) {\n        track_t *t = &trk[i];\n        if (((pr >> i) & 1u)",
                 "seq.c panic propagation")

    midi_control_c=a.felucca/"firmware/src/midi_control.c"
    patch_source(midi_control_c, "typedef struct {",
                 "extern void hybrid_audio_midi_note_on(uint8_t, uint8_t, uint8_t);\nextern void hybrid_audio_midi_note_off(uint8_t, uint8_t);\nextern void hybrid_audio_midi_panic(uint8_t);\n\ntypedef struct {",
                 "midi_control.c native MIDI declarations")
    patch_source(midi_control_c,
                 "        if (!midi_local_held(&trk[id - 1u], nn[i]))\n                input_off(&trk[id - 1u], nn[i]);",
                 "        if (!midi_local_held(&trk[id - 1u], nn[i])) {\n                input_off(&trk[id - 1u], nn[i]);\n                hybrid_audio_midi_note_off(nn[i], (uint8_t)ch);\n            }",
                 "MIDI note-off propagation")
    patch_source(midi_control_c,
                 "        if (!midi_note_held(t, nn[i]) && !midi_local_held(t, nn[i]))\n            input_on(t, nn[i], vel);",
                 "        if (!midi_note_held(t, nn[i]) && !midi_local_held(t, nn[i])) {\n            input_on(t, nn[i], vel);\n            hybrid_audio_midi_note_on(nn[i], (uint8_t)vel, (uint8_t)ch);\n        }",
                 "MIDI note-on propagation")
    patch_source(midi_control_c,
                 "        for (i = 0; i < NTRK; i++)\n            if (mask & (1u << i))\n                midi_silence_track(i);\n        break;",
                 "        for (i = 0; i < NTRK; i++)\n            if (mask & (1u << i))\n                midi_silence_track(i);\n        hybrid_audio_midi_panic((uint8_t)ch);\n        break;",
                 "MIDI CC120 propagation")
    patch_source(seq_c, "            key_on(k, &trk[kb_trk[k]]);",
                 "            key_on(k, &trk[kb_trk[k]]);\n            hybrid_audio_note_on(kb_note[k], (uint8_t)key_on_v);",
                 "seq.c note-on")
    patch_source(seq_c, "            key_off(k, &trk[kb_trk[k] % NTRK]);",
                 "            key_off(k, &trk[kb_trk[k] % NTRK]);\n            hybrid_audio_note_off(kb_note[k]);",
                 "seq.c note-off")

    (out/"NAMESPACE-MANIFEST.txt").write_text(
        "Exported DSP globals: "+str(len(exports))+"\nObjects: "+", ".join(x.name for x in [amb_obj,dub_obj,*zen_objs,hybrid_core_obj,hybrid_adapter_obj,hybrid_profile_obj])+"\n")
    print("DSP objects compiled, namespaced and added to Felucca's final AC79 link.")
    print("HYBRID mixer hooks installed: GHOULBOX native mix plus AMB-1, DUB-1 and ZEN-1 overlays.")
    print("AC79 runtime profiler linked: per-engine TIMER4 timing, stack samples, buffer registration and DMA underruns.")

if __name__=="__main__": main()
