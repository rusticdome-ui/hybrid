#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build and attach the independent HYBRID wavetable engine to the AC79 SMK-37 target."""
import argparse, subprocess
from pathlib import Path
CFLAGS=["-target","pi32v2","-mcpu=r3","-mfprev1","-ffp-contract=off","-Oz","-fno-unroll-loops","-ffunction-sections","-fdata-sections","-fno-common","-fno-builtin","-DHYBRID_ENABLE_PROFILE=1","-Wall","-Wno-unused-function","-DBOARD_SMK37=1"]
def run(args):
 print("+"," ".join(map(str,args)),flush=True);subprocess.run(list(map(str,args)),check=True)
def replace_once(path,old,new,label):
 s=path.read_text()
 if new in s:return
 if s.count(old)!=1:raise SystemExit(f"{label}: expected exactly one source anchor, found {s.count(old)}")
 path.write_text(s.replace(old,new))

def patch_single_project_cache(fw):
 project=fw/"project.c"
 replace_once(project,
  'project_store_t proj_slot[4] __attribute__((section(".noinit")));',
  'project_store_t proj_cache __attribute__((section(".noinit")));\nstatic uint8_t proj_cache_slot;\nstatic uint8_t proj_cache_valid;',
  "single project cache declaration")
 old='''static void proj_fetch(uint32_t slot)
{
    project_store_t *q = &proj_slot[slot & 3u];
    int n;
    proj_wire_gen++;
    n = st_load(OBJ_PROJECT0 + (slot & 3u), &proj_wire, sizeof proj_wire);
    if (!proj_import(&proj_scratch, &proj_wire, n))
        memset(q->raw, 0, 4);
    else {
        proj_bound(&proj_scratch);
        if (!proj_pack(q, &proj_scratch))
            memset(q->raw, 0, 4);
    }
}'''
 new='''/* One decoded working slot. Flash A/B objects remain authoritative; .noinit
 * cache bytes are never trusted until this session loads and validates them. */
static int proj_cache_load(uint32_t slot)
{
    slot &= 3u;
    if (proj_cache_valid && proj_cache_slot == slot) {
        if (proj_import(&proj_scratch, &proj_cache, sizeof proj_cache))
            return 1;
        proj_cache_valid = 0;
    }
    proj_cache_valid = 0;
#if FELUCCA_FLASH
    if (flash_ok) {
        int n;
        proj_wire_gen++;
        n = st_load(OBJ_PROJECT0 + slot, &proj_wire, sizeof proj_wire);
        if (!proj_import(&proj_scratch, &proj_wire, n))
            return 0;
        proj_bound(&proj_scratch);
        if (!proj_pack(&proj_cache, &proj_scratch))
            return 0;
        proj_cache_slot = (uint8_t)slot;
        proj_cache_valid = 1;
        return 1;
    }
#endif
    return 0; /* RAM-only mode has only the one explicitly saved cache slot. */
}'''
 replace_once(project,old,new,"on-demand project cache loader")
 save_old='memcpy(&proj_slot[slot & 3u], &proj_wire, sizeof proj_wire);'
 save_new='memcpy(&proj_cache, &proj_wire, sizeof proj_wire);\n        proj_cache_slot = (uint8_t)(slot & 3u);\n        proj_cache_valid = 1;'
 source=project.read_text()
 begin=source.index("static int project_save_as(")
 end=source.index("static int project_save(", begin)
 save_region=source[begin:end]
 if save_region.count(save_old)!=2: raise SystemExit("project save cache update: expected exactly two save paths")
 project.write_text(source[:begin]+save_region.replace(save_old,save_new)+source[end:])
 replace_once(project,
  'if (!proj_import(&proj_scratch, &proj_slot[slot & 3u], sizeof(project_store_t)))\n        return 0;',
  'if (!proj_cache_load(slot))\n        return 0;',
  "project info on-demand load")
 replace_once(project,
  'if (!proj_import(p, &proj_slot[slot & 3u], sizeof(project_store_t))) {',
  'if (!proj_cache_load(slot)) {',
  "project rename on-demand load")
 replace_once(project,
  '    memcpy(&proj_slot[slot & 3u], &proj_wire, sizeof proj_wire);\n    if (proj_cur == (slot & 3u))',
  '    memcpy(&proj_cache, &proj_wire, sizeof proj_wire);\n    proj_cache_slot = (uint8_t)(slot & 3u);\n    proj_cache_valid = 1;\n    if (proj_cur == (slot & 3u))',
  "project rename cache update")
 old='''static void project_load(uint32_t slot)
{
#if FELUCCA_FLASH
    if (flash_ok && !proj_import(&proj_scratch, &proj_slot[slot & 3u], sizeof(project_store_t))) proj_fetch(slot);
#endif
    if (!proj_import(&proj_scratch, &proj_slot[slot & 3u], sizeof(project_store_t))) { ui_message("EMPTY SLOT"); return; }
    proj_bound(&proj_scratch);'''
 new='''static void project_load(uint32_t slot)
{
    if (!proj_cache_load(slot)) {
        ui_message(flash_ok ? "EMPTY SLOT" : "RAM ONLY: EMPTY");
        return;
    }
    proj_bound(&proj_scratch);'''
 replace_once(project,old,new,"project load on-demand")
 replace_once(project,
  'static void persist_boot(void)                    /* before settings_init / panel_init */\n{',
  'static void persist_boot(void)                    /* before settings_init / panel_init */\n{\n    proj_cache_valid = 0; /* .noinit cache is untrusted after every reset. */',
  "invalidate cache at boot")
 old='''    {   /* projects: fill empty RAM slots from flash, so the slot list is right after power-on */
        uint32_t i;
        for (i = 0; i < 4u; i++)
            if (!proj_import(&proj_scratch, &proj_slot[i], sizeof(project_store_t)))
                proj_fetch(i);
    }
'''
 replace_once(project,old,"    /* Projects are fetched individually on demand; no four-slot RAM mirror. */\n","remove eager four-project preload")
 replace_once(project,
  'static int project_used(uint32_t slot) { return proj_import(&proj_scratch, &proj_slot[slot & 3u], sizeof(project_store_t)); }',
  'static int project_used(uint32_t slot) { return proj_cache_load(slot); }',
  "chain project presence check")
 replace_once(project,
  'if (!proj_import(p, &proj_slot[i], sizeof(project_store_t))) return 3u + i;',
  'if (!proj_cache_load(i)) return 3u + i;',
  "chain source on-demand load")
 if "proj_slot[" in project.read_text():
  raise SystemExit("single-cache patch incomplete: residual proj_slot[] reference")


def patch_editor_backup(fw):
 backup=fw/"editor_backup.c"
 replace_once(backup,
  '''    if (id >= 2u && id <= 5u) {
        if (project_used(id - 2u)) *len = sizeof proj_slot[0];
        return (const uint8_t *)&proj_slot[id - 2u];
    }''',
  '''    if (id >= 2u && id <= 5u) {
        uint32_t slot = id - 2u;
        if (project_used(slot)) {
            memcpy(ED_BK_RAW, &proj_cache, sizeof proj_cache);
            *len = sizeof proj_cache;
        }
        return ED_BK_RAW;
    }''',
  "backup project export through the single cache")
 replace_once(backup,
  '''    if (ed_bk_id >= 2u && ed_bk_id <= 5u) {
        memset(&proj_slot[ed_bk_id - 2u], 0, sizeof proj_slot[0]);
        if (proj_cur == ed_bk_id - 2u)
            proj_cur = PROJ_NO_SLOT;                     /* (another project there now: the music keeps its name) */
        if (ed_bk_len) memcpy(&proj_slot[ed_bk_id - 2u], raw, ed_bk_len);
    } else if (ed_bk_id == 1u) {''',
  '''    if (ed_bk_id >= 2u && ed_bk_id <= 5u) {
        uint32_t slot = ed_bk_id - 2u;
        if (proj_cache_valid && proj_cache_slot == slot)
            proj_cache_valid = 0;
        if (ed_bk_len) {
            memset(&proj_cache, 0, sizeof proj_cache);
            memcpy(&proj_cache, raw, ed_bk_len);
            proj_cache_slot = (uint8_t)slot;
            proj_cache_valid = 1;
        }
        if (proj_cur == slot)
            proj_cur = PROJ_NO_SLOT;                     /* (another project there now: the music keeps its name) */
    } else if (ed_bk_id == 1u) {''',
  "backup project import through the single cache")

def main():
 p=argparse.ArgumentParser()
 p.add_argument("--felucca",type=Path,required=True)
 p.add_argument("--toolchain",type=Path,required=True)
 p.add_argument("--hybrid",type=Path,default=Path(__file__).resolve().parents[1])
 a=p.parse_args();tc=a.toolchain.resolve();clang=tc/"pi32v2/bin/clang"
 if not clang.is_file():raise SystemExit(f"missing JieLi compiler: {clang}")
 out=a.felucca/"build/hybrid";out.mkdir(parents=True,exist_ok=True)
 inc=a.hybrid/"include";fw=a.felucca/"firmware/src";hal=a.felucca/"firmware/hal"
 patch_single_project_cache(fw)
 patch_editor_backup(fw)
 objects=["hybrid_core","hybrid_wavetable","hybrid_native_adapters","hybrid_profile","hybrid_sequencer","hybrid_ac79_scan","hybrid_ac79_scan_hw","hybrid_ac79_startup","hybrid_ac79_calibration","hybrid_ac79_audio","hybrid_ac79_audio_board"]
 for name in objects:
  src=(a.hybrid/"ac79/platform/src"/(name+".c")) if name.startswith("hybrid_ac79_") else (a.hybrid/"src"/(name+".c"));obj=out/(name+".o")
  if not src.is_file():raise SystemExit(f"missing HYBRID source: {src}")
  run([clang,*CFLAGS,"-I"+str(inc),"-I"+str(fw),"-I"+str(hal),"-I"+str(a.hybrid/"ac79/platform/include"),"-c",src,"-o",obj])
 build=a.felucca/"tools/build.py";build_src=build.read_text()
 anchor='''    tc("pi32v2/bin/ld", "-T", ld, OUT / "crt0.o", OUT / "fm1_vec.o", OUT / "fm1_isr.o",
       OUT / "felucca.o", "-o", elf)'''
 args=",\n       ".join('OUT / "hybrid" / "'+n+'.o"' for n in objects)
 replacement='''    tc("pi32v2/bin/ld", "--gc-sections", "-T", ld, OUT / "crt0.o", OUT / "fm1_vec.o", OUT / "fm1_isr.o",
       OUT / "felucca.o",
       '''+args+''',
       "-Map", OUT / "felucca.map", "-o", elf)'''
 if replacement not in build_src:
  if build_src.count(anchor)!=1:raise SystemExit("Felucca linker invocation changed; refusing unsafe patch")
  build.write_text(build_src.replace(anchor,replacement))
 ld=a.felucca/"firmware/app.ld"
 old='''    .bss (NOLOAD) : { _bss_start = .; *(.bss*) *(.sbss*) *(COMMON) . = ALIGN(4); _bss_end = .; } > RAM
    .pool (NOLOAD) : { _pool_start = .; *(.pool*) . = ALIGN(4); _pool_end = .; } > POOL'''
 new='''    .pool (NOLOAD) : {
        _pool_start = .;
        *(.pool*)
        build/hybrid/hybrid_core.o(.bss* COMMON)
        build/hybrid/hybrid_wavetable.o(.bss* COMMON)
        build/hybrid/hybrid_native_adapters.o(.bss* COMMON)
        build/hybrid/hybrid_profile.o(.bss* COMMON)
        build/hybrid/hybrid_sequencer.o(.bss* COMMON)
        build/hybrid/hybrid_ac79_audio.o(.bss* COMMON)
        . = ALIGN(4); _pool_end = .;
    } > POOL
    .bss (NOLOAD) : { _bss_start = .; *(.bss*) *(.sbss*) *(COMMON) . = ALIGN(4); _bss_end = .; } > RAM'''
 replace_once(ld,old,new,"AC79 POOL placement")
 audio=a.felucca/"firmware/src/audio.c"
 replace_once(audio,"/* registers: hal/fm1_audio.h */","/* registers: hal/fm1_audio.h */\nextern void hybrid_audio_process(int32_t *, uint32_t, int32_t);\nextern void hybrid_profile_begin(int);\nextern void hybrid_profile_end(int, uint32_t);\nextern void hybrid_profile_underrun(void);\nextern void hybrid_audio_seq_process(uint32_t);","audio declarations")
 replace_once(audio,"    mix_block(out, n);","    hybrid_audio_seq_process(n);\n    hybrid_profile_begin(4);\n    mix_block(out, n);\n    hybrid_profile_end(4, n);\n    hybrid_audio_process(out, n, fx_usb_fixed ? MASTER_FULL : (int32_t)song.master_q12);","audio mixer hook")
 replace_once(audio,"        if (fm1_audio_free_half() != half)\n            felucca_dbg.late++;","        if (fm1_audio_free_half() != half) {\n            felucca_dbg.late++;\n            hybrid_profile_underrun();\n        }","audio DMA underrun hook")
 main_c=a.felucca/"firmware/src/main.c"
 replace_once(main_c,"static void felucca_init(void)","extern int hybrid_audio_init(void);\nextern void *hybrid_audio_render_context(void);\nextern int hybrid_ac79_audio_startup(void *);\nextern int hybrid_ac79_scan_startup(void);\n\nstatic void felucca_init(void)" ,"main declaration")
 replace_once(main_c,"    fm1_adc_init();","    fm1_adc_init();\n    /* Start independent raw SPI2/ADC capture; legacy event decoding remains active. */\n    (void)hybrid_ac79_scan_startup();","native scanner startup")
 replace_once(main_c,"    gb_scene_load(ui_scene);","    gb_scene_load(ui_scene);\n    (void)hybrid_audio_init();","engine initialization")
 replace_once(main_c,"    audio_init();","    audio_init();\n    /* Rebind ALNK0 IRQ 11 to the independent HYBRID Q15 renderer. */\n    (void)hybrid_ac79_audio_startup(hybrid_audio_render_context());","native ALNK0 renderer startup")
 usb=a.felucca/"firmware/src/usb.c"
 replace_once(usb,"    mo_w++;\n    }\n}\n\n/* ------------------------------------------------------- descriptors --- */","    mo_w++;\n    }\n}\n\nvoid hybrid_midi_out_event(uint32_t pkt) { midi_out_event(pkt); }\n\n/* ------------------------------------------------------- descriptors --- */","MIDI output bridge")
 seq=a.felucca/"firmware/src/seq.c"
 replace_once(seq,"static void keyboard_block(void)","extern void hybrid_audio_note_on(uint8_t, uint8_t);\nextern void hybrid_audio_note_off(uint8_t);\nextern void hybrid_audio_panic(void);\nextern void hybrid_audio_seq_sync(uint8_t, uint16_t, uint8_t);\n\nstatic void keyboard_block(void)","sequencer declarations")
 replace_once(seq,"    if (midi_route != (uint8_t)song.g[G_ROUTE]) {","    hybrid_audio_seq_sync((uint8_t)song.playing, (uint16_t)song.g[G_BPM], (uint8_t)song.g[G_CLOCK]);\n    if (midi_route != (uint8_t)song.g[G_ROUTE]) {","sequencer transport sync")
 replace_once(seq,"    for (i = 0; i < NTRK; i++) {\n        track_t *t = &trk[i];\n        if (((pr >> i) & 1u)","    if (pr) hybrid_audio_panic();\n    for (i = 0; i < NTRK; i++) {\n        track_t *t = &trk[i];\n        if (((pr >> i) & 1u)","sequencer panic")
 replace_once(seq,"            key_on(k, &trk[kb_trk[k]]);","            key_on(k, &trk[kb_trk[k]]);\n            hybrid_audio_note_on(kb_note[k], (uint8_t)key_on_v);","keyboard note-on")
 replace_once(seq,"            key_off(k, &trk[kb_trk[k] % NTRK]);","            key_off(k, &trk[kb_trk[k] % NTRK]);\n            hybrid_audio_note_off(kb_note[k]);","keyboard note-off")
 clock=a.felucca/"firmware/src/midi_clock.c"
 replace_once(clock,"static __attribute__((noinline)) void midi_clock_transport(uint32_t status, uint32_t ms)\n{","extern void hybrid_audio_seq_clock_pulse(void);\nextern void hybrid_audio_seq_sync(uint8_t, uint16_t, uint8_t);\nstatic __attribute__((noinline)) void midi_clock_transport(uint32_t status, uint32_t ms)\n{","clock declarations")
 replace_once(clock,"        seq_start();\n    } else if (status == 0xFBu)","        seq_start();\n        hybrid_audio_seq_sync((uint8_t)song.playing, (uint16_t)song.g[G_BPM], (uint8_t)song.g[G_CLOCK]);\n    } else if (status == 0xFBu)","external Start")
 replace_once(clock,"        song.playing = 1;\n    } else if (status == 0xFCu)","        song.playing = 1;\n        hybrid_audio_seq_sync((uint8_t)song.playing, (uint16_t)song.g[G_BPM], (uint8_t)song.g[G_CLOCK]);\n    } else if (status == 0xFCu)","external Continue")
 replace_once(clock,"        seq_stop();\n    }\n}","        seq_stop();\n        hybrid_audio_seq_sync((uint8_t)song.playing, (uint16_t)song.g[G_BPM], (uint8_t)song.g[G_CLOCK]);\n    }\n}","external Stop")
 replace_once(clock,"static __attribute__((noinline)) void midi_clock_pulse(uint32_t ms)\n{\n    uint32_t q;","static __attribute__((noinline)) void midi_clock_pulse(uint32_t ms)\n{\n    uint32_t q;\n    hybrid_audio_seq_clock_pulse();","external MIDI clock")
 midi=a.felucca/"firmware/src/midi_control.c"
 replace_once(midi,"typedef struct {","extern void hybrid_audio_midi_note_on(uint8_t,uint8_t,uint8_t);\nextern void hybrid_audio_midi_note_off(uint8_t,uint8_t);\nextern void hybrid_audio_midi_panic(uint8_t);\n\ntypedef struct {","MIDI control declarations")
 replace_once(midi,"        if (!midi_local_held(&trk[id - 1u], nn[i]))\n                input_off(&trk[id - 1u], nn[i]);","        if (!midi_local_held(&trk[id - 1u], nn[i])) {\n                input_off(&trk[id - 1u], nn[i]);\n                hybrid_audio_midi_note_off(nn[i], (uint8_t)ch);\n            }","MIDI note-off")
 replace_once(midi,"        if (!midi_note_held(t, nn[i]) && !midi_local_held(t, nn[i]))\n            input_on(t, nn[i], vel);","        if (!midi_note_held(t, nn[i]) && !midi_local_held(t, nn[i])) {\n            input_on(t, nn[i], vel);\n            hybrid_audio_midi_note_on(nn[i], (uint8_t)vel, (uint8_t)ch);\n        }","MIDI note-on")
 replace_once(midi,"        for (i = 0; i < NTRK; i++)\n            if (mask & (1u << i))\n                midi_silence_track(i);\n        break;","        for (i = 0; i < NTRK; i++)\n            if (mask & (1u << i))\n                midi_silence_track(i);\n        hybrid_audio_midi_panic((uint8_t)ch);\n        break;","MIDI channel panic")
 (out/"HYBRID-WAVETABLE-MANIFEST.txt").write_text("Independent HYBRID wavetable engine; no AMB-1, DUB-1 or ZEN-1 objects.\nObjects: "+", ".join(objects)+".o\n")
 print("HYBRID wavetable AC79 objects compiled and linked; legacy AMB-1/DUB-1/ZEN-1 sources are not fetched or linked.")
if __name__=="__main__":main()
