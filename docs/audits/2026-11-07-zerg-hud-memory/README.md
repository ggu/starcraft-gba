# Zerg HUD memory probe: measured scope and open gate

This optional probe reuses the existing framebuffer-page HUD implementation.
It does not change normal release vectors, actor/effect limits or linker reserves.
Compiled source is `9f185cfdddbf235cff1197fbb0416d458f53b895`, based on the
combined experimental research/Q8/Flash source `8be82458`. The separate physical
comparison driver is `b42a21e5cabaeb51b174eb2cbc77dc128d6fc77e`.

## Resource and authored-render checks

Four actual guarded links (Z9/Z10, OFF/ON), four separate HOST poisoned-page
checks and the artifact-role negative fixture pass with all 593 prepared inputs
unchanged. Each HOST check compares 22 render cases against full-clear output,
including resource/building updates, save/load and restart. Normalized compiler
vectors differ only in the HUD flag and output paths. Each product's EWRAM
margin rises from 132 to 12,588 bytes: **12,456 bytes net saved**. IWRAM margin
remains 1,936 bytes. These are linker margins, not runtime stack or hardware FPS.

Private build receipt SHA256:
`1cdc270283ac514c3ed4ecc1d9ced2344071d2c8f5d3292e1b6b0c76e5287dd9`.

## Actual Z9 ordinary custom comparison

The first two Z9/Z10 guest attempts reused a tape that relied on P10's default
human race and failed their final intended-scene gate. Those attempts are retained.
A separate profile-aware prefix preserves the authenticated original tape and
shifts later inputs by eight calls, explicitly choosing map 0, Protoss human and
one Terran AI. Early live lobby and match checks pass. No guest memory/register
injection or constructed save was used.

The corrected Z9 run compares all 3,056 physical input ticks. OFF/ON named world
state, full displayed indexed framebuffer and palette match throughout, including
3,005 advancing and 51 other ticks. Both guests produce identical genuine
128 KiB Flash saves with valid commit/checksum and erased legacy area:
`1da1ce0d75fcf02406f2e5a4d48f2db5ea21e34b77c3ab98ef52b82a318660f1`.

A diagnostic-only repeat retains the unchanged conservative verdict and records
all saved, uninterrupted-reference and fresh-restored states and view hashes.
Both variants match each other at the uninterrupted and fresh-load checkpoints.
Each fresh-restored named world state exactly matches its uninterrupted reference:
wallet 376 minerals/12 gas, job slot 8, timer 4,998, frame 3,006 (saved timer 4,999
and frame 3,005). Palette, display mode and play-screen state also match.
The **fresh versus uninterrupted pixel hashes differ**, so the overall strict
comparison fails. The driver does not publish a full-success receipt.

Private comparison SHA256:
`44a1595b760e5e9fe371436865ad83f63cce6933029a91b18121a95b150d2cab`.

The loader unconditionally adds a 55-update toast at y=108 and resets transient
UI/effects/caches. Those are source-backed reasons immediate cross-path images
can differ; the retained hashes alone do not locate the observed pixel difference.
Record raw images and compare after the documented transient expires before
claiming fresh-versus-uninterrupted render continuity. Preserve exact OFF/ON
state/pixel/palette comparisons at every equivalent phase. The corrected Z10
guest path is not yet run. No rollout or hardware qualification follows.

Recorded cycle distributions include every prefix tick, including pause/save and
other nonadvancing calls. They are not a busy-frame FPS or performance result.
The public audit includes authored metadata only; game data, binaries, raw frames,
physical saves and native logs remain private.
