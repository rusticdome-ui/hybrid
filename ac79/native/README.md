# smk37-hybrid-native

Standalone SMK-37 Pro / AC79 target, built separately from Felucca. This target is an XIP application using the m-bootloader application hand-off contract; it is not a replacement mask-ROM bootloader.

## Confirmed platform inputs

Pinned JieLi SDK: AC79NN_SDK_V1.2.1_2023-12-13.

- cpu/wl82/sdk_ld_sfc.c: SFC/XIP link origin is 0x02000120.
- cpu/wl82/sdk_ld_sdram.c: separate SDRAM link profile; not used by the SMK-37 Pro no-external-RAM application profile.
- include_lib/driver/cpu/wl82/asm/hwi.h: ISR_ENTRY=0x01c7fe00, IRQ_ALNK_IDX=11, IRQ_SPI2_IDX=37, IRQ_SARADC_IDX=24, and software IRQ 122.
- cpu/wl82/setup.c: the SDK setup_arch() performs early watchdog, clock and interrupt setup for SDK applications.

The SDK linker uses a generic RAM0 formula that reserves TLB, boot-info and update areas. It is not, by itself, the application RAM map for every board/bootloader combination.

## Application entry and memory profile

The standalone ELF targets the SMK-37 Pro m-bootloader hand-off contract documented by the open m-bootloader project:

- Application entry: 0x02000120, with interrupts disabled and the boot-info pointer in r0
- Startup switches to application-owned supervisor/user stacks, copies .data, zeros runtime RAM, and then enters HYBRID
- SMK-37 application flash extent: physical [0x4000, 0x9d000), mapped into the XIP window; linker capacity from the entry is 0x98ee0 bytes
- Conservative application RAM: [0x01c40000, 0x01c7c000), 0x3c000 bytes, avoiding bootloader live RAM, download plug-in slots and retained boot/update data
- No external SDRAM/PSRAM is used

The reset stub is therefore an application entry shim, not the silicon's raw reset handler. The raw mask-ROM reset and full exception ABI remain bootloader responsibilities and are not fully described by the public JieLi SDK.

The full linker script is linker/smk37-hybrid-native.ld. It defines .entry, XIP .text, RAM .hybrid_data, .hybrid_dma, .hybrid_bss, .hybrid_noinit, .hybrid_pool, and copy/zero tables. .noinit is intentionally excluded from zero initialization. Link-time assertions protect the app entry and stack reservation.

## Physical peripheral IRQ table

hybrid_native_irq.c installs a 128-slot peripheral handler pointer table at the SDK ISR_ENTRY address and configures the WL82 ICFG source nibbles. The table programming and ICFG gate sequence follow a hardware-tested pi32v2 application example; the source numbers/base are checked against the pinned JieLi hwi.h.

The app boot-info object and decoded flash header occupy part of the same retained window. _start passes the boot-info pointer to C; the runtime copies the six-word object and 32-byte decoded header before the native IRQ table reuses that window. All unused IRQ slots are fail-stop; ALNK0 is registered at IRQ 11, priority 4. This is the peripheral IRQ dispatch table, not a claim that the CPU's complete reset/exception vector ABI has been reconstructed.

## Native HYBRID audio path

- hybrid_native_board.c initializes TIMER4 and the native IRQ table, calls hybrid_audio_init(), then starts hybrid_ac79_audio_startup().
- hybrid_ac79_audio_board.c uses TIMER4 as the 24 MHz crystal counter and routes IRQ attachment through the native IRQ layer. It does not write a Felucca vector array.
- hybrid_ac79_audio.c places double-buffered ALNK0 DMA memory in .hybrid_dma, registers ALNK0 IRQ 11 and renders the HYBRID wavetable core plus sequencer directly into DMA blocks.
- hybrid_profile.c reads TIMER4 directly in standalone mode.
- The native target does not call Felucca audio_init() and links no Felucca startup objects or libraries.

The current native board adapter still contains hardware assumptions for ALNK0 pin mux, PC6 DAC enable and clock routing. These are source-derived, not yet verified electrically on the user's physical SMK-37 Pro.

## Build and validation

tools/build_smk37_native.py:

1. checks the pinned SDK's XIP entry, ISR base and ALNK0 IRQ number;
2. compiles the HYBRID core, wavetable, sequencer, profiler, native entry, IRQ layer and ALNK0/DMA adapter with JieLi pi32v2;
3. links with the complete native linker script;
4. inspects ELF symbols/sections and rejects Felucca symbol leakage.

The AC79 workflow uploads the standalone ELF, MAP, section/symbol listings and report separately from the legacy Felucca validation package.

## Explicit limits

- This ELF is compatible with the documented m-bootloader application hand-off; it must not be assumed to boot under an arbitrary stock bootloader without confirming that bootloader's hand-off ABI.
- It is an ELF validation image only. No .fwsc package or production flashing identity is generated.
- No physical SMK-37 Pro boot, DAC/codec electrical test, ALNK0 DMA soak test, CPU load measurement or underrun measurement has yet been performed.
- Matrix, SPI2/ADC controls, LCD, MIDI input/output and the rest of the independent BSP are not migrated into this target yet.
- The legacy Felucca validation path remains unchanged as a separate reference. PR #13 remains Draft and must not be merged without explicit approval.

Do not flash this ELF as production firmware.
