# Reuse resolved source-effect records

Runtime `7fad8a1` reuses immutable image, animation and sampled-frame records
already resolved by projectile and child-overlay drawing. The keyed public
image entry point remains available. Drawing still uses the requested image
key for palette selection, re-resolves an aliased projectile image key, and
samples animation zero when the active phase has no animation. The separate
NULL-animation frame used for overlay placement remains unchanged. Children
retain their own lifetime, trigger, birth position and drawing behavior even
when the parent frame is absent. No source bank, trajectory, weapon rule,
animation clock, palette, saved field or unit layout changes.

The new asset-free trace fixture runs in the default sanitizer suite. It
checks NULL-active animation-zero drawing and image-key re-resolution; a
private mutation removing the latter fails its assertion. The prepared-bank
fixture checks rendered pixels, LUT pointer/row metadata and source-frame
geometry against a separately assembled raster, including a deliberately
different alias LUT index. Candidate and baseline pass that fixture under
ASan/UBSan. Two inherited assertions counted old per-row blit calls; they now
check the authored projectile and LO-child image identities, allowing the
rectangle batching already merged in PR130.

Default build/tests pass: 649 Python checks, 20 optional skips, eight profile
checks and native fixtures. Source-boundary and whitespace checks pass. A
freeze of 743 tracked runtime/test/Makefile sources remains unchanged across
the qualification. Prepared builds use captured direct compiler arguments
and existing absolute inputs; no imported Make recipes or asset extraction
or generation were used. The default suite uses authored synthetic fixtures.

All four normal ARM links pass unchanged memory guards and reserves:

| Product | IWRAM end | BSS end |
| --- | --- | --- |
| Terran | `0x03004df0` | `0x0203708c` |
| Zerg Z6 | `0x03004f40` | `0x020370f8` |
| Protoss P7 | `0x03004b70` | `0x020382b8` |
| Showcase | `0x03004fd0` | `0x0203df38` |

This adds no global RAM or cache. In the inspected normal P7 disassembly,
local stack reservations grow by 8 bytes in trace drawing, 16 in the overlay
helper and 8 in the resolved raster helper, with the same register pushes.
These bounded local-frame observations are not a measured whole-game stack
high-water mark; no stack reserve was reduced.

The 72 normal-menu skirmish/save continuations pass across both maps, all
human races, 1–3 AIs and focused three-AI compositions. Actual ARM in-place and
fresh-process SRAM continuation matches the host in all three race products
from frame 9001 to 9901. All six owned SRAM emulator processes terminated and
were waited on. Cache markers describe the existing VFX duration cache, not
this stack-local reuse or the direct-firing cache.

All three race products also match sampled displayed Mode 4 RGB555 and world
checkpoints. The baseline is the immutable PR130 `d8bf5b8` probe with the same
probe template; checkpoint hashing is outside measured intervals and the
next two ticks are excluded. All three owned pixel/timing processes are
terminal. Across the same 37,859 normal-menu Blood Bath frames against three
mixed AIs, cycle-equivalent rates are:

| Profile | Baseline fps | Candidate fps | Slowest window, baseline → candidate |
| --- | ---: | ---: | ---: |
| Terran | 28.941 | 28.988 | 28.058 → 28.316 |
| Zerg Z6 | 28.705 | 28.758 | 28.028 → 28.028 |
| Protoss P7 | 28.754 | 28.791 | 25.006 → 25.391 |
| Aggregate | 28.789 | 28.835 | — |

Aggregate counted cycles fall from 22,062,956,732 to 22,027,789,640, about
0.16%. These fixed-camera unattended defeats and sampled image checks do not
establish campaign completion, an every-frame image proof, a frame-time
ceiling, steady 30 fps or GBA SP performance. The all-30-mission scope remains
unfinished and the delivered catalog stays T10/Z6/P7.

Private reports remain under `.local/build/vfx-resolved-images-release/`,
`vfx-resolved-images-rgb/`, `vfx-resolved-images-fresh-sram/` and
`vfx-resolved-synth/`. No Actions, GDB or SD writes were used. No game assets,
generated game data, ROMs, saves, captures, private harnesses or native
decompilation are tracked.
