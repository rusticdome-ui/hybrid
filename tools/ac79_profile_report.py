#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Emit linker-section-derived AC79 memory and runtime-profiler report."""
import argparse
import re
from pathlib import Path

XIP_CAPACITY = 0x97400
RAMTEXT_CAPACITY = 0x6000
RAM_CAPACITY = 96 * 1024
POOL_CAPACITY = 0x54000
NOINIT_CAPACITY = 0x3D50

def read_sections(path):
    sections = {}
    pattern = re.compile(r"^\s*\d+\s+(\S+)\s+([0-9a-fA-F]{8,16})\s+", re.M)
    for name, raw_size in pattern.findall(path.read_text(errors="replace")):
        sections[name] = int(raw_size, 16)
    return sections

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--sections", required=True, type=Path)
    p.add_argument("--output", required=True, type=Path)
    a = p.parse_args()
    sections = read_sections(a.sections)
    required = {".text", ".ram_text", ".data", ".bss", ".pool", ".noinit"}
    missing = sorted(required - sections.keys())
    if missing:
        raise SystemExit("Missing final ELF sections: " + ", ".join(missing))
    ram_used = sections[".data"] + sections[".bss"]
    rows = [
        (".text / XIP", sections[".text"], XIP_CAPACITY),
        (".ram_text", sections[".ram_text"], RAMTEXT_CAPACITY),
        (".data + .bss", ram_used, RAM_CAPACITY),
        (".pool", sections[".pool"], POOL_CAPACITY),
        (".noinit", sections[".noinit"], NOINIT_CAPACITY),
    ]
    lines = [
        "# SMK-37 HYBRID - AC79 memory and profiler report", "",
        "Generated from objdump section sizes of the final linked ELF and SMK-37 linker region capacities.", "",
        "## Static memory (linker-derived)", "",
        "| Region | Used | Capacity | Remaining | Status |",
        "|---|---:|---:|---:|---|",
    ]
    for name, used, capacity in rows:
        lines.append(f"| {name} | {used} B | {capacity} B | {capacity-used} B | {'PASS' if used <= capacity else 'OVER'} |")
    lines += [
        "", "## Runtime instrumentation linked into firmware", "",
        "| Metric | Instrumentation |", "|---|---|",
        "| AMB-1 / DUB-1 / ZEN-1 | TIMER4 calls, frames, total/last/max ticks |",
        "| GHOULBOX | TIMER4 around native mix_block() |",
        "| HYBRID sequencer | TIMER4 duration for shared-clock scheduling |",
        "| HYBRID mixer | TIMER4 aggregate duration |",
        "| DMA underruns | increments when DMA advances before render completes |",
        "| Stack watermark | sampled local stack-marker high-water against linker stack windows |",
        "| Audio scratch buffers | current block occupancy / registered capacity |",
        "", "## Hardware measurements", "",
        "**Pending physical SMK-37 run.** GitHub-hosted CI cannot measure live DSP CPU,",
        "stack high-water or underruns. The firmware snapshot API exposes these counters",
        "for debugger/CDC diagnostics; this report does not invent runtime values.", "",
        "Stack watermark is sampled rather than a guard-band proof. Buffer occupancy",
        "covers HYBRID audio scratch buffers only; it excludes private allocations in",
        "upstream engines and the Felucca pool. POOL is reported from the final static ELF",
        "section, not from a live allocator high-water counter."
    ]
    a.output.write_text("\n".join(lines) + "\n")
    print(a.output.read_text())
    if any(used > capacity for _, used, capacity in rows):
        raise SystemExit("Final ELF exceeds an AC79 static memory region budget")

if __name__ == "__main__":
    main()
