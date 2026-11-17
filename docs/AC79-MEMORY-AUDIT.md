# AC79 memory audit — HYBRID Wavetable migration

The previous memory audit measured a different firmware composition with multiple external DSP objects. Those object sizes and linker margins are not applicable to the current Wavetable-only engine build.

The new AC79 workflow generates a fresh report from the final linked ELF using `tools/ac79_profile_report.py`. It reports .text/XIP, .ram_text, .data + .bss, .pool and .noinit against the SMK-37 linker capacities.

No current memory totals are asserted in this document. They must come from the successful workflow artifact for the exact commit being evaluated.

The report is static. Physical-board stack high-water, dynamic POOL allocator use, CPU time, DMA underruns and sustained audio stability still require a debugger/hardware run. The AC79 output remains validation-only until a dedicated HYBRID package identity and flashing procedure are verified.
