# AC79 memory audit — SMK-37 HYBRID

Date: 2026-11-09  
Branch: `hybrid-ac79-profiler`  
Diagnostic source run: [GitHub Actions #57](https://github.com/rusticdome-ui/hybrid/actions/runs/42717305990)  
Source commit: `04fd71e26d1277cbe320fdf77fc887d0640a9e44`  
Artifact: `ac79-linker-diagnostics-04fd71e26d1277cbe320fdf77fc887d0640a9e44`

## Method and limits

The per-object values below were read from the JieLi pi32v2 `objdump -h` output for the six HYBRID relocatable objects. They are pre-link, pre-garbage-collection section sizes; they are not a substitute for a successful final ELF map. GHOULBOX is built as a single translation unit (`felucca.o`) and is represented by its independently built baseline ELF section totals.

## HYBRID object sizes

| Object | .text | .data | .bss | .pool |
|---|---:|---:|---:|---:|
| AMB-1 (`amb.o`) | 28,364 | 80 | 26,594 | 262,824 |
| DUB-1 (`dub.o`) | 57,254 | 162 | 46,565 | 262,824 |
| ZEN-1 (`fumi.o`) | 37,550 | 40 | 10,488 | 135,300 |
| HYBRID core | 1,746 | 0 | 0 | 0 |
| Native adapters/mixer | 870 | 0 | 11,652 | 0 |
| Runtime profiler | 502 | 0 | 44 | 0 |
| **Total** | **126,286** | **282** | **95,343** | **660,948** |

All values are bytes. The .bss column includes each object's individual .bss.* sections.

## Baseline and exact POOL reconciliation

GHOULBOX-only baseline ELF:

- `.text`: 574,932 B
- `.data + .bss`: 88,380 B
- `.pool`: 300,132 B
- `.noinit`: 15,564 B

Felucca linker script `firmware/app.ld` reserves:

- RAM: 96 KiB = 98,304 B
- POOL: 0x54000 = 344,064 B
- XIP: 0x8DFBC = 909,244 B
- RAMTEXT: 24 KiB
- NOINIT: 0x3D50 bytes

The measured POOL arithmetic is exact:

`300,132 B GHOULBOX baseline + 660,948 B HYBRID objects = 961,080 B`

`961,080 B - 344,064 B POOL capacity = 617,016 B overflow`

This equals the linker-reported POOL overflow in Actions run #57. It confirms that the three DSP object pools account for the full reported POOL excess in that build.

The DSPs also add 95,625 B of .data + .bss at object level, before any additional linker-generated/common-symbol effects. The linker reported 88,180 B RAM overflow. Do not treat the pre-link object total as final RAM placement.

The linker reported XIP overflow of 372,292 B. The DSP objects contain 126,286 B of .text pre-GC; the remaining final XIP footprint needs a successful linked map to attribute reliably.

## External RAM feasibility

The AC79 family SDK documentation describes 578 KiB on-chip SRAM and says 2 MiB / 8 MiB external SDRAM is supported only on selected packages. That is a capability of selected AC791N packages, not proof that the SMK-37 Pro board has SDRAM fitted.

The public SMK-37 Open firmware's board linker template explicitly sets `SDRAM_SIZE = 0`. The current Felucca SMK-37 linker script also defines only XIP, RAMTEXT, RAM, POOL and NOINIT regions; it does not declare an external SDRAM region. No public board configuration or source inspected so far enables external SDRAM for the SMK-37.

**Engineering decision:** treat the SMK-37 Pro as having no usable external RAM until the physical board/package and SDRAM initialization are verified. Do not move HYBRID buffers to an assumed SDRAM address. The installed flash is non-volatile storage, not extra runtime RAM.

## Next technical actions

1. Finish a reliable diagnostic link/map and inspect the pi32v2 runtime archives for `__floatsisf`.
2. Verify the actual SMK-37 chip marking/package and inspect the PCB for a separate SDRAM device.
3. If no external RAM is present, design a global memory plan across GHOULBOX, AMB, DUB and ZEN. Reducing AMB/DUB alone cannot solve the 617,016 B POOL excess.
4. Only then choose reduced buffer capacities, fixed-point/streaming alternatives, or a reduced simultaneous-engine profile.
