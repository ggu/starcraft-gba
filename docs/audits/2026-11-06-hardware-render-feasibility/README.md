# Hardware-backed renderer feasibility, 2026-11-06

Read-only source audit of `94bc32f8cee27b001390433ce919c8e0b91a0ed4`.
Recommendation: do not allocate OBJ tiles in production yet. First measure
current render scopes and build a HOST-only composition feasibility oracle.
Individual OBJ units may be useful in a narrow scene, but existing VRAM owners,
painter ordering and crowded scanlines make them a renderer redesign, not a
free switch. The older Mode 0 probe is a separate, larger project, with no
measured full-loop win in its documented workload. No new performance or
hardware claim follows from this audit.

## Memory and hardware budget

Mode 4 has two 240×160 indexed pages, `[06000000,06009600)` and
`[0600a000,06013600)`. The 2,560-byte first gap `[06009600,0600a000)` is
not free: the sampled mixer reserves 1,600 bytes and the audio observation
snapshot uses two bytes for every unit and building slot. With the largest
240 + 160 pools that leaves only 160 bytes, before any other reservation.
The second gap `[06013600,06014000)` is 2,560 bytes: optional death-VFX VRAM
storage starts at its beginning, and the last 32 bytes are the text-box queue
at `[06013fe0,06014000)`. Do not assume the rest is uniformly free across
products or borrow persistent state without lifetime and guard proof.

Bitmap modes allow OBJ tile indices 512–1023, the upper 16 KiB at
`[06014000,06018000)`, not the whole 32 KiB object address space. The project
uses that same region for `SC64_TERRAIN_CACHE_VRAM`; P7 additionally reserves
4,816 bytes for projectile state at its start. A 240-byte cached row gives:

| Profile with terrain cache | Projectile bytes | Cached rows / bytes | Unused tail |
| --- | ---: | ---: | ---: |
| Without P7 arena | 0 | 68 / 16,320 | 64 bytes |
| With P7 arena | 4,816 | 48 / 11,520 | 48 bytes |

The other rows of the 108-row cache already occupy EWRAM. Thus the tail fits
one 8×8 8bpp OBJ tile without the P7 arena and not one with it. For example,
reserving 1,024 bytes for four distinct 16×16 8bpp images displaces four
240-byte rows (960 EWRAM bytes) in the first case, or five rows (1,200 bytes)
in P7, before descriptors, alignment and an atomic update strategy. These are
capacity calculations, not evidence the images or slot count suffice. The
previous six-profile links reported minima of 796 bytes EWRAM and 120 bytes
IWRAM guard headroom, from different products; they are link slack rather than
a dynamic-stack guarantee, and must be remeasured on the eventual candidate.
The HUD cache can use the opposite Mode 4 page (`SC64_HUD_PAGE_BANDS`), so
those pixels are not an unowned scratch arena either. `SC64_RANGE_WORK` uses
reserved planes in route-field storage, not a free VRAM allocation.

GBA OAM has 128 descriptors, but there is also a per-scanline rendering budget;
it is not safe to assume 128 densely overlapping units render. Nintendo's
[AGB Programming Manual, section 6.3](https://device.report/m/992fdfb85cddf170aad00db0129cce1bc49acbe216c078aa4f0073ffb8791afc.pdf)
gives the normal non-affine formula `(308*4-6)/width` with the usual HBlank
setting: at most 38 32-pixel-wide or 19 64-pixel-wide entries on one line in
that idealized uniform case, before other OBJ entries and real edge cases.
A frame-level descriptor count alone is insufficient. 8bpp OBJ uses a shared
256-color OBJ palette; 4bpp offers palette banks but requires compatible
per-image conversion. Per-owner banks must not silently recolor currently
shared indexed source art. The map sprite import uses transparent index zero;
owner colors also appear in selection and fallback shapes, so palette equality
must be checked on actual paths, including effects and imported art.

## Exactness constraints

`draw_map` paints terrain/fog, static resources and building art, building
overlays, far death effects, all selection ellipses, units in slot order with
body/cargo/vitals, near death effects, tracers and cursor/menu. HUD and modal
presentation follow. This is painter order, not a simple per-y sort. A single
OBJ priority layer above the framebuffer would put units over later software
tracers, UI, later units and HUD; placing it below would hide them behind the
opaque Mode 4 background. Selection, effects, overlaps and interleaving need
an exact compositor or a conservative whole-frame software fallback. Existing
map clipping is y `[12,120)` and then restored for HUD; a hardware window
must also preserve menus, pause, result, text, scrolling and both pages. OAM
order, transparency, palette changes and frame/page presentation must be
included in the oracle. No unit, race, animation, owner, effect, AI or scene
may disappear when a budget is exhausted.

The default-off `SC64_TILE_RENDERER` already explores Mode 0: BG3 terrain,
BG2 fog, BG1 static art, eight 64×64 OBJ composites for a 38,400-byte software
buffer, and BG0 HUD. It is limited to imported race missions; skirmish falls
back to Mode 4. It takes the whole OBJ arena and disables the terrain/projectile
owners, and its historical P7 build omitted sampled audio because Mode 0
occupies its VRAM gap. `docs/VALIDATION.md` records 2,095 HOST frame RGB555
matches and 17 mGBA checkpoints for that old probe, but no full-loop gain:
opening 12.694→12.632 and neutral 11.188→11.156 fps equivalents. Draw savings
were largely consumed by 50–90k cycles of tile copies and moving static art;
possible missed-VBlank tearing was not qualified. This does not establish
behavior or performance on current source, other products, or hardware.

## Measured ceiling and next experiment

On an earlier qualified P7 source, the 1,000-call diagnostic reported
602,805 mean render cycles per advancing loop, including 369,861 map and
125,193 HUD cycles, versus 2,200,226 uninstrumented full-loop cycles. The
instrumented loop had 3.32% observer overhead; scopes are nested and cannot be
added or treated as attainable savings. A different older Zerg diagnostic
reported 110,030 mean unit-draw cycles and 122,640 background cycles per call;
these are distinct workloads and stale source, not current savings estimates.
Even eliminating that entire old unit-draw scope would not by itself close the
P7 gap to the approximately 559,000-cycle 30 fps period. The current 2.53%
full-field route result is a different passive custom workload and cannot be
combined with those figures.

A bounded next step, with no ROM allocation, is a HOST-only candidate planner:
collect actual visible unit/effect rectangles and painter positions from
normal scenes; prove for each whole frame whether the desired OBJ set fits
OAM, the per-line cycle budget, palette/transparency, clipping, tile bytes,
transfer deadline and painter ordering. Otherwise select the existing complete
software frame. Start with no OBJ rather than choosing an arbitrary actor cap;
measure eligible frames, distinct animated tile bytes, palette use, per-line
occupancy, and fallback frequency across both pages, all races, 1–3 AI,
scrolling, overlap, HUD/menu/pause, effects, save/load and dense battles. The
independent oracle must compare final composited RGB555 for every pixel and
persistent state with the normal renderer, including source-black versus OBJ
transparent index zero. Exercise over-budget and forced-fallback cases, OAM
ties and late UI overlap. This requires a real GBA-priority/window/scanline
model; the existing tile-probe HOST compositor implements only its configured
subset and cannot establish arbitrary OBJ behavior.

Only if that census shows material eligible coverage should a guarded shared
ARM prototype propose a concrete arena redistribution and descriptor budget,
with P7 projectile/audio/text/death lifetimes intact, worst-profile BSS/IWRAM
and stack guards, atomic tile/OAM updates and normal-source tests for all
products. Use independent fully composited framebuffer/palette captures and
paired guest cycles, including upload/fallback costs and tails; retain full
software fallback for every rejected frame. Measure current useful map, unit,
HUD and presentation waits first, on busy and sparse ordinary inputs, so the
prototype has an evidenced target. Mode 0 should only be reopened with a plan
for sampled audio, skirmish, compatible caches, memory and VBlank safety.
