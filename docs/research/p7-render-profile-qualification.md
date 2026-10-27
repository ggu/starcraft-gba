# Full P7 mode-4 drawing profile

The opt-in `P7_RENDER_FLAGS` profile combines the full P7 FACT V2/contact
runtime with the mode-4 drawing paths already used by the canonical race
products. It enables word sprite stores, scene clipping, HUD buffer reuse,
window-band copies, static dirty rows, minimap blitting, available packed
unit frames and fast vital bars. It does not enable the mode-0 tile renderer
or move the mapped blitter into IWRAM. The earlier FACT V2 targets retain
their exact flags and products.

It also enables `SC64_P7_ARBITER_AURA_ONCE`: once a target is covered by an
allied Arbiter during a source tick, later Arbiters skip its repeated geometry
checks. Aura is a boolean union built afresh each tick; no persistent cache,
membership rule or detection cadence changes. The separate sanitizer oracle
compares all packed cache words against an all-pairs reference over 80
randomized worlds, including overlapping Arbiters, distinct source owners
sharing a live owner, moved/dead/loaded index entries and inactive-index
fallback. Run it with `make test-sc64-p7-arbiter-aura-once`.

```sh
# Prepared private P7 inputs are required:
make test-sc64-p7-render
make test-sc64-p7-render-campaign-controller
make protoss-p7-render-probe
```

The playable output is `build/StarCraft-Protoss-P7-Render-Probe-GBA.gba`,
identified as `SC P7 DRAW` / `S7DR`. Canonical progression remains 22/30.

## Display and gameplay regression

Reference and candidate ASan/UBSan builds run the same 2,095 displayed frames:
menu launch, neutral play, four camera pans, pause/resume, a field-menu
attack-move and combat. Every frame's RGB555 hash, camera position and the
terminal simulation digest match. The shared fixture now calls `gba_init()`
before `game_init()` so its RGB oracle includes the actual low-index HUD
palette, as well as the imported terrain/sprite palette.

A separate full winning-tape host replay matches all 67 state records and
the strict verdict from the [cache profile](p7-fact-v2-qualification.md).
The public adaptive controller also wins at call 39,976 / frame 39,738 with
all three heroes; its 1,667 input transitions pass the edge validator. These
are host results, distinct from the bounded ARM run below.

## Bounded ARM measurement

The reference's unpadded load image is byte-identical to the previously
measured FACT V2/contact replay (`774a71f8…150174`). The new public
Make-selected ARM replay is byte-identical to the measured drawing/aura
candidate (`d7a4e24a…ce6e9c`). All 20 state checkpoints and the terminal record
match.

| Window | Frames | FACT V2/contact cycles | Drawing profile cycles | Reduction |
| --- | ---: | ---: | ---: | ---: |
| Opening | 600 | 2,061,085 | 2,021,775 | 1.91% |
| Attack | 1,400 | 2,325,286 | 2,261,289 | 2.75% |

The drawing paths alone measured 2,019,435 / 2,299,446 cycles. Adding the
aura short-circuit costs 0.12% in the opening and reduces attack cycles by
1.66% relative to that drawing-only variant. Both component comparisons
preserve the state records; the profile favors the measured combat saving.

The measurements include update and draw time in mGBA. They do not establish
full-mission performance or physical-GBA behavior, and remain far below the
30-fps target. The full winning-tape and fresh-process SRAM ARM results for
the core FACT V2 profile are recorded separately; they are not new ARM
save/load qualification for these additional drawing flags.

## Fit and identities

| Product | Unpadded bytes | Padded bytes | BSS end | IWRAM end |
| --- | ---: | ---: | --- | --- |
| Bounded replay | 12,864,220 | 16,777,216 | `0x0203d570` | `0x03004e20` |
| Playable probe | 12,850,664 | 16,777,216 | `0x0203dd80` | `0x03004e20` |

The playable ROM SHA-256 is
`cfd55cdbbeac1445c10db6885b0c200abede661dba87ef9e87054203f51599d3`.
The bounded replay uses 2,196 bytes of measured main stack, leaving 9,580
bytes above the reserved boundary. All original guards remain unchanged.

A separate candidate additionally enabling `GBA_MAPPED_BLIT_IWRAM` failed
the unchanged IWRAM guard at `0x030051a0`; it is excluded from the profile.
No reserve was reduced to make it fit.

The private source snapshot, compiler arguments, frame hashes, controller
trace, ELF maps and measured ROM identities are under
`.local/build/p7-render-profile-20261028/` (including `public-aura/`),
`.local/build/p7-arbiter-candidates-20261028/` and
`.local/build/p7-render-aura-20261028/`. No generated game assets, ROMs, saves
or captures are distributed with this source change.
