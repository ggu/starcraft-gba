# Optional death-effect VRAM storage

`SC64_DEATH_VFX_VRAM_STORAGE` places the transient death-effect pool, alive
shadow, and result-outro counters in the unused Mode-4 VRAM tail. The option is
off by default and the canonical campaign flags do not enable it. Builds using
the tile renderer reject the option because that renderer owns a different
VRAM layout.

The mode-4 frame pages occupy `0x06000000`–`0x060095ff` and
`0x0600a000`–`0x060135ff`. The existing sampled-audio arena starts at
`0x06009600`; the mixer uses its first 1,600 bytes. This feature starts at
`0x06009c40` and compile-time bounds its state to the remaining 960 bytes.
The audio VBlank handler and DMA buffers do not use this tail.

The death pool retains the existing `Sc64DeathVfx` interface through a
halfword storage facade. Rows are loaded into ordinary RAM staging objects
before the existing eviction, timing, and rendering helpers use them. The
alive shadow is packed into halfwords, and reset/metadata operations use
halfword accesses. No byte stores or byte-oriented `memset` target hardware
VRAM. The pool remains presentation-only and does not change `RaceSave` or
mission state.

## Focused evidence

The opt-in `test-sc64-death-vfx-z7-vram` target builds the same full-render Z7
host profile with and without the storage flag. Its combat, reuse, load-reset,
fog, and result-outro fixture passes under ASan/UBSan. A paired 900-update
combat workload reaches the same outcome and matches the canonical state,
save, and RGB555 framebuffer digests:

| Check | Baseline | VRAM storage |
| --- | --- | --- |
| Outcome | win, frame 901 | win, frame 901 |
| State digest | `372b8bb0` | `372b8bb0` |
| Save digest | `db37d166` | `db37d166` |
| RGB555 framebuffer digest | `45262b01` | `45262b01` |

The optional ARM link pair uses the full-render Z7 vector and passes the
unchanged linker guards. The baseline BSS ends at `0x0203dfe8`; the candidate
ends at `0x0203de68`, freeing 384 bytes. Both end IWRAM at `0x03004d10`.
Inspection of the generated ARM instructions shows halfword stores for the
VRAM reset, row, seen-shadow, and counter writes. The storage path adds a
320-byte local pool staging array in `death_vfx_scan`; the ARM link does not
measure runtime performance or stack use. Keep the option experimental until
whole-mission timing and stack-canary runs qualify it on the intended profile.
