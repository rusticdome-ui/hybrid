# AC79 HYBRID runtime profiler

Uses the AC79 TIMER4 clock at 24 MHz. Collects per-engine call counts,
rendered frames, last/max/total ticks, aggregate mixer timing, late-DMA
underruns, sampled stack high-water and explicitly registered buffer usage.

AMB-1, DUB-1 and ZEN-1 are timed around native render calls. GHOULBOX is
timed around native mix_block(). The HYBRID overlay has an aggregate bucket.
Stack high-water is a sampled estimate using linker-defined stack windows,
not a guard-page proof. Buffer figures cover only registered HYBRID scratch
and native buffers, not private upstream DSP allocations.

The snapshot API is available to debugger/diagnostic integration. GitHub
Actions can report linker-derived static memory, but cannot report live CPU,
stack or underrun values without a physical SMK-37 attached.
