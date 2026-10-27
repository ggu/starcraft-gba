# Full P7 mode-4 drawing profile

The opt-in `P7_RENDER_FLAGS` profile combines the full P7 FACT V2/contact
runtime with the mode-4 drawing paths already used by the canonical race
products. It enables word sprite stores, scene clipping, HUD buffer reuse,
window-band copies, static dirty rows, minimap blitting, available packed
unit frames and fast vital bars. It does not enable the mode-0 tile renderer
or move the mapped blitter into IWRAM. The earlier FACT V2 targets retain
their exact flags and products.

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
Make-selected ARM replay is byte-identical to the measured drawing candidate
(`1e939342…1e226`). All 20 state checkpoints and the terminal record match.

| Window | Frames | FACT V2/contact cycles | Drawing profile cycles | Reduction |
| --- | ---: | ---: | ---: | ---: |
| Opening | 600 | 2,061,085 | 2,019,435 | 2.02% |
| Attack | 1,400 | 2,325,286 | 2,299,446 | 1.11% |

The measurements include update and draw time in mGBA. They do not establish
full-mission performance or physical-GBA behavior, and remain far below the
30-fps target. The full winning-tape and fresh-process SRAM ARM results for
the core FACT V2 profile are recorded separately; they are not new ARM
save/load qualification for these additional drawing flags.

## Fit and identities

| Product | Unpadded bytes | Padded bytes | BSS end | IWRAM end |
| --- | ---: | ---: | --- | --- |
| Bounded replay | 12,864,196 | 16,777,216 | `0x0203d570` | `0x03004e20` |
| Playable probe | 12,850,640 | 16,777,216 | `0x0203dd80` | `0x03004e20` |

The playable ROM SHA-256 is
`4029ebfaa35d49bf858c7846e5a490c9352a5a4fb976b10091a98ca4f25e90e5`.
The bounded replay uses 2,196 bytes of measured main stack, leaving 9,580
bytes above the reserved boundary. All original guards remain unchanged.

A separate candidate additionally enabling `GBA_MAPPED_BLIT_IWRAM` failed
the unchanged IWRAM guard at `0x030051a0`; it is excluded from the profile.
No reserve was reduced to make it fit.

The private source snapshot, compiler arguments, frame hashes, controller
trace, ELF maps and measured ROM identities are under
`.local/build/p7-render-profile-20261028/`. No generated game assets, ROMs,
saves or captures are distributed with this source change.
