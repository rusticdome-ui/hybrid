# AC79 single-project working cache

The four project slots remain four independent flash objects. The existing storage layer's A/B sector pair, sequence number, payload CRC, header CRC and commit-last write protocol are unchanged.

The firmware keeps only one packed project working cache in `.noinit`. It is an expendable cache, not persistent storage: the BSS validity flag is cleared during boot, so bytes left in `.noinit` are never trusted after reset. A cache miss loads the requested object with `st_load`, validates/imports it, bounds its values, and repacks it before publishing the cache slot.

Project listing, rename, load and song-chain preparation fetch each requested slot on demand. Song-chain preparation copies the required motion, step and timing data into the chain's own runtime source before the single cache is reused for the next project.

A failed flash save does not publish the staging buffer into the cache. The storage A/B protocol keeps the previous committed copy authoritative. A successful save or rename updates the working cache only after `st_save` returns success.

If the flash device is unavailable, the UI's existing `SAVED (RAM)` indication means volatile single-slot operation: only the one cached slot is available and it is lost on reset or when another slot replaces it. It is not represented as four persistent RAM projects.

The AC79 linker report now fails the build if free `.noinit` space is below 2,048 bytes. This is a static linker check; physical flash power-cut, DMA and long-duration hardware tests remain separate acceptance gates.
