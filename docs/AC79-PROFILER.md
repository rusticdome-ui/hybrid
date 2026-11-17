# AC79 HYBRID runtime profiler

Uses the AC79 TIMER4 clock at 24 MHz. Collects HYBRID Wavetable call counts,
rendered frames, last/max/total ticks, aggregate mixer timing, late-DMA
underruns, sampled stack high-water and explicitly registered buffer usage.

The snapshot API is available to debugger/diagnostic integration. GitHub
Actions can report linker-derived static memory, but cannot report live CPU,
stack or underrun values without a physical SMK-37 attached.

Stack high-water is a sampled estimate using linker-defined stack windows,
not a guard-page proof. Buffer figures cover only registered HYBRID scratch
and wavetable state; they do not represent the full firmware POOL allocator.
