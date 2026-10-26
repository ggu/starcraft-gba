# Minimap source alignment

The 24-pixel minimap raster starts at screen x=197. Previously its temporary
source buffer started on a four-byte boundary, so source and destination
could not align together for the shared sprite compositor's four-pixel path.
Word-sprite builds now align the buffer and offset its start by `x & 3`.
The 24-byte stride preserves that phase on every row. Three padding bytes
cover all four phases; transparent pixels and clipping retain their existing
behavior. No persistent state or save fields change.

## Verification

`make test-sc64-minimap-terrain` runs the independent pixel oracle against
scalar, ordinary blit and word-compositor builds under ASan/UBSan. Each runs
320 cases covering all four horizontal phases, negative coordinates, right
and vertical clipping, poisoned destinations and transparent/random masks.
The full-render Z7, canonical Terran and canonical Protoss framebuffer
fixtures also passed their full-clear comparisons. `./dev test` passed all
554 Python checks and its host fixtures.

The paired diagnostic ARM builds used the full-render Z7 vector plus
`SC64_DIAGNOSTIC_TERRAIN`, identical compiler flags and the same 12,000-tick
normal-input tape. Both matched 34 projected checkpoints over 11,989 play
samples, ending at frame 11,726 without a mission outcome or fault. These
checks compare projected gameplay state, not complete world state or ARM
pixels; the pixel oracles above run on the host.

| Mean cycles per play sample | Before | Aligned |
| --- | ---: | ---: |
| Minimap counter | 77,641 | 76,604 |
| HUD counter | 105,848 | 104,810 |
| Outer audio/render | 576,345 | 575,686 |
| Update plus audio/render | 1,071,082 | 1,070,424 |

This is a small measured improvement: 1.34% of the minimap counter, 0.114%
of outer rendering and 0.061% of combined time in this sample. It does not
establish near-30-fps performance, a full Z7 win, or physical hardware behavior.

The diagnostic ELF retained IWRAM/BSS ends `0x03004ca8` / `0x0203d6ec`,
leaving 856 / 2,324 bytes below the unchanged guards. Its ROM load end grew
16 bytes, leaving 1,373,624 bytes. These diagnostic margins are distinct from
the production probe's margins. The temporary array adds three padding
bytes; there is no new BSS allocation. No stack
high-water measurement is claimed.

Timing recipes, tapes and emulator artifacts remain private under `.local/`.
No game assets, ROMs, captures or saves accompany this report. Canonical
campaign qualification remains 22/30.
