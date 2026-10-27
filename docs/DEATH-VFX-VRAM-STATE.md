# Optional death-effect VRAM storage

`SC64_DEATH_VFX_VRAM_STORAGE` places the transient death-effect pool, alive
shadow, and result-outro counters in the padding after the second Mode-4 page.
The option is off by default and the canonical campaign flags do not enable
it. Builds using the tile renderer reject the option because that renderer
owns a different VRAM layout.

The mode-4 frame pages occupy `0x06000000`–`0x060095ff` and
`0x0600a000`–`0x060135ff`. The 2.5 KiB pad after the second page is
`0x06013600`–`0x06013fff`; this feature starts at `0x06013600` and compile-time
bounds its state before the P7 projectile arena at `0x06014000`. The earlier
audio gap is not free: imported game audio stores its live actor snapshot at
`0x06009c40`. Compile-time assertions keep this feature beyond the entire
snapshot and before the projectile arena.

The death pool retains the existing `Sc64DeathVfx` interface through a
halfword storage facade. Rows are loaded into ordinary RAM staging objects
before the existing eviction, timing, and rendering helpers use them. The
alive shadow is packed into halfwords, and reset/metadata operations use
halfword accesses. No byte stores or byte-oriented `memset` target hardware
VRAM. The pool remains presentation-only and does not change `RaceSave` or
mission state.

## Focused evidence

The opt-in `test-sc64-death-vfx-z7-vram` target builds the same full-render Z7
host profile with and without the storage flag. Its synthetic regression
fixture covers combat, reuse, load-reset, fog, and result-outro behavior and
passes under ASan/UBSan. A separate paired 900-update synthetic combat
workload reaches the same fixture outcome and matches the canonical state,
save, and RGB555 framebuffer digests. These are regression checks, not a
canonical mission-win replay:

| Check | Baseline | VRAM storage |
| --- | --- | --- |
| Outcome | win, frame 901 | win, frame 901 |
| State digest | `372b8bb0` | `372b8bb0` |
| Save digest | `db37d166` | `db37d166` |
| RGB555 framebuffer digest | `45262b01` | `45262b01` |

The corrected optional ARM link pair uses the full-render Z7 vector and passes
the linker guards. Baseline BSS ends at `0x0203dfe8`; the candidate ends at
`0x0203de68`, freeing 384 bytes. Both end IWRAM at `0x03004d10`. Inspection of
the candidate instructions shows halfword stores for VRAM reset, row,
seen-shadow, and counter writes. The storage path adds a 320-byte local pool
staging array in `death_vfx_scan`; whole-mission timing and stack-canary runs
remain unqualified.
