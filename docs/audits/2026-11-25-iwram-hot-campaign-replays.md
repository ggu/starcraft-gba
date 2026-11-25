# Campaign replays on the IWRAM-relinked race cartridges

[The IWRAM hot placement](2026-11-25-iwram-hot-placement.md) links
each race cartridge's hottest ROM functions into the part of the old
stack reserve that the stack cannot reach. Its audit left the
campaigns open: full replays on the relinked ROMs, and the stack
depth of Protoss 1–9, Terran 2, Zerg 3 and Zerg 10. This audit runs
every campaign route, checkpoint tape and pause save found on this
machine on the previous-link ROM and the relinked ROM of each race,
and compares the two runs.

There is no blocking finding. 71 of the 72 finished pairs match
completely, and the remaining one, the Protoss 4 tape that no longer
wins on the packaged ROM, is identical on both ROMs up to the same
driver error. 22 missions replay in full from HOME or the previous
chapter's victory to victory, and every other mission with a route,
checkpoint or pause save matches on the evidence it has. The Zerg 7
route is still running at the time of writing and is reported when
finished. No stack-tracked relinked run's main stack went above the
bound of the stricter checker on PR #882 (Terran 7,792, Zerg 5,848,
Protoss 5,940 bytes) or came within 512 bytes of `__iwram_end`. The
closest to its bound is Zerg 3: 5,720 bytes, 128 under. The closest to
the code is also Zerg 3, with 1,696 bytes free. The IRQ stack reached
296 bytes, against its bound of 304 of 416. The relinked ROM's
main-stack low-water equals the previous link's at every one of the
6,708 marks taken. Stack tracking covered 641,900 updates of 38 pairs;
the other routes are covered by the static bound the build checks.

## ROM pairs

| Cartridge | Previous link | Relinked | `__iwram_end` (previous, relinked) |
| --- | --- | --- | --- |
| Terran 10 | `eae5363f4815` | `d5c059027c8b` | `0x03004B78`, `0x03005AE0` |
| Zerg 10 | `b954fde5512b` | `1c219da7a440` | `0x03004FD0`, `0x03006108` |
| Protoss 10 | `ced2565d8350` | `c20eb18a20db` | `0x03004F08`, `0x03006018` |

SHA-256 prefixes. These are the `base` and `hot2` pairs that the
placement audit measured, linked from `71937af5` and `e9d0f6c1`.
A fresh `make` of `e9d0f6c1` reproduces the relinked Zerg and Protoss
ROMs byte for byte, and the Terran ROM except three header title
bytes. The previous-link Zerg and Protoss ROMs are the packaged
`5f758095` release bytes that the routes were recorded on. The
previous-link Terran ROM was built from other private art inputs: it
holds 5,584 more bytes of generated weapon-effect tables
(`sc64_weapon_source_*`) than the packaged Terran ROM
`1779e2ef70d4`. Both Terran ROMs of the pair share those inputs, and
every Terran 5 stage save this pair wrote equals the save the packaged
ROM wrote at that stage.

PR #882 has since merged `codex/source-release` through `1ea819ce`,
an engine change, so its tip `18e9d57c` builds other bytes. A second
pass on a pair built from that tip is described [below](#second-pass-on-the-pr-tip).

## Method

`tools/sc64_physical_record.py run` executes a replay tool unchanged
and records every emulator process the tool starts:

- every change of the held buttons, with its update index;
- a sample at the start, every 100 completed updates, at each SRAM
  export and at the end. A sample holds `frame`, `screen`, the
  mission, completion, bank and camera values, and SHA-256 hashes of
  the raw unit, building and mineral tables, both framebuffer pages,
  the BG and OBJ palettes and OBJ VRAM, plus the displayed page;
- the SHA-256 of every exported save.

`compare` requires the two records to be equal row for row and each
to be whole: a zero exit, at least one process, every periodic
sample, an export sample with each save hash, a final sample, and no
button, run, update or export command sent around the recorder. Guest
memory is only read; no run wrote any.

Each route ran once per ROM, with the same driver, arguments, tapes
and seed saves, in the same port-free mGBA runner. Some drivers
choose inputs from game state. They page through briefings,
acknowledge text boxes while the box word at `0x06013FE0` is set, or
walk the cursor to a target. Button rows are compared like the other
rows, so a matched pair also received identical inputs.

The drivers check what they always check: a pause save at a neutral
point reloaded in a fresh emulator process that must continue
identically, segment chains in which every segment is a new process
that loads the previous segment's save, and the victory save reloaded
from HOME in a new process and exported again byte for byte. A
handoff pair loads each victory save, presses A, acknowledges the next
mission's opening dialogue, pauses, saves, reloads that save in a new
process, plays 300 neutral updates and saves again. A briefing pair
starts from a blank cartridge, chooses the chapter on the campaign
menu, pages through its briefing, deploys, plays 400 updates, saves
and reloads.

## Missions

"Full replay" is a route from HOME, or from the previous chapter's
victory save, to this mission's victory. "Partial replay" is a late
checkpoint continued to victory or a tape that ends before victory.
"Save only" is pause saves continued with neutral input.

| Mission | Evidence | Inputs | Route updates per ROM | Emulator processes per ROM | Samples compared | Saves compared | Route ends at | Handoff reaches | Result |
| --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| Terran 1 | full replay | HOME tape to victory, pause and fresh reload at tick 6,222 | 10,204 | 5 | 120 | 5 | frame 10,120, victory | Terran 2 at frame 311 | matched |
| Terran 2 | full replay | HOME tape to victory, pause and fresh reload at tick 18,213 | 29,455 | 5 | 312 | 5 | frame 29,366, victory | Terran 3 at frame 340 | matched |
| Terran 3 | full replay | HOME, neutral countdown to victory, fresh reload at tick 26,993 | 54,091 | 5 | 559 | 5 | frame 54,000, victory | Terran 4 at frame 401 | matched |
| Terran 4 | full replay | HOME tape to victory, fresh reload at tick 4,991 | 9,018 | 5 | 108 | 5 | frame 8,911, victory | Terran 5 at frame 401 | matched |
| Terran 5 | full replay | HOME tape to tick 19,731, 21 stage scripts to frame 88,779, 13 blind segments, 4 finishing scripts | 147,997 | 65 | 1,642 | 43 | frame 145,471, victory | Terran 6 at frame 401 | matched |
| Terran 6 | full replay | HOME tape to victory, fresh reload at tick 19,659 | 23,961 | 5 | 258 | 5 | frame 23,536, victory | Terran 7 at frame 311 | matched |
| Terran 7 | full replay | HOME tape to victory, fresh reload at tick 2,334 | 4,288 | 5 | 61 | 5 | frame 3,931, victory | Terran 8 at frame 401 | matched |
| Terran 8 | full replay | HOME tape to victory, fresh reload at tick 2,761 | 143,335 | 5 | 1,452 | 5 | frame 139,216, victory | Terran 9 at frame 311 | matched |
| Terran 9 | full replay | HOME tape to victory, fresh reload at tick 36,000 | 48,073 | 5 | 499 | 5 | frame 47,266, victory; A then enters Terran 10 | Terran 10 at frame 311 | matched |
| Terran 10 | full replay | HOME tape to victory, fresh reload at tick 2,342 | 3,537 | 5 | 54 | 5 | frame 3,361, victory | Terran 10 at frame 311 (retry) | matched |
| Zerg 1 | full replay | HOME tape to the frame-984 pause, then 17 blind segments | 50,426 | 21 | 563 | 21 | frame 49,270, victory | Zerg 2 at frame 401 | matched |
| Zerg 2 | full replay | HOME, one blind tape | 10,511 | 4 | 121 | 4 | frame 10,209, victory | Zerg 3 at frame 401 | matched |
| Zerg 3 | full replay | Zerg 2 victory save, A, then 32 blind segments | 69,256 | 37 | 797 | 37 | frame 68,639, victory | Zerg 4 at frame 401 | matched |
| Zerg 4 | full replay | HOME blind tape to the frame-28,754 pause, then 8 blind segments | 70,793 | 12 | 742 | 12 | frame 64,147, victory | Zerg 5 at frame 401 | matched |
| Zerg 5 | full replay | Zerg 4 victory save, A, then 14 blind segments | 24,360 | 17 | 293 | 17 | frame 18,974, victory | Zerg 6 at frame 91 | matched |
| Zerg 6 | full replay | Zerg 5 victory save, A, then 52 blind segments | 178,582 | 55 | 1,929 | 55 | frame 177,044, victory | Zerg 7 at frame 401 | matched |
| Zerg 7 | full replay | HOME, one blind trace | running | | | | | | still running |
| Zerg 8 | full replay | HOME, one tape with text boxes acknowledged | 88,022 | 4 | 896 | 4 | frame 86,969, victory | Zerg 9 at frame 401 | matched |
| Zerg 9 | save only | three pause saves (frames 101, 27,817 and about 60,000) continued | 8,442 | 6 | 102 | 6 | frame 62,860, mission active | none | matched |
| Zerg 10 | partial replay | frame-60,123 pause save, tape suffix to victory | 3,597 | 6 | 62 | 6 | frame 63,523, victory | Zerg 10 at frame 401 (retry) | matched |
| Protoss 1 | full replay | HOME tape to victory | 15,522 | 4 | 171 | 4 | frame 15,305, victory | Protoss 2 at frame 318 | matched |
| Protoss 2 | partial replay | HOME tape to its end at call 35,618 (mission still active), then its pause save continued | 38,498 | 5 | 403 | 5 | frame 36,500, mission active | Protoss 3 at frame 401 | matched |
| Protoss 3 | save only | two pause saves (frames 101 and 20,000) continued | 6,628 | 8 | 99 | 8 | frame 23,300, mission active | Protoss 4 at frame 401 | matched |
| Protoss 4 | partial replay | HOME tape with a frame-986 pause and fresh reload; the tape does not win | 87,032 | 4 | 885 | 3 | frame 86,028, mission active | Protoss 5 at frame 401 | identical; both ROMs end the tape with the same driver error |
| Protoss 5 | save only | two pause saves (frames 101 and 2,700) continued | 6,628 | 8 | 99 | 8 | frame 6,000, mission active | Protoss 6 at frame 401 | matched |
| Protoss 6 | full replay | HOME tape to victory, fresh reload at call 65,803 | 94,434 | 6 | 1,251 | 5 | frame 92,986, victory | Protoss 7 at frame 93 | matched |
| Protoss 7 | partial replay | frame-76,682 pause save, 905 recorded input runs to victory | 6,289 | 6 | 89 | 6 | frame 80,828, victory | Protoss 8 at frame 343 | matched |
| Protoss 8 | full replay | HOME, one blind tape | 100,641 | 4 | 1,022 | 4 | frame 98,849, victory | Protoss 9 at frame 401 | matched |
| Protoss 9 | full replay | Protoss 8 victory save, A, then one blind tape | 137,977 | 6 | 1,406 | 6 | frame 134,481, victory | Protoss 10 at frame 97 | matched |
| Protoss 10 | full replay | HOME tape to victory, fresh reload at call 31,887 | 54,502 | 5 | 563 | 5 | frame 53,225, victory | Protoss 10 at frame 97 (retry) | matched |

"Route updates" counts the route's own pairs. Processes, samples and
saves are per ROM and also include the handoff and briefing pairs.
In all, the 72 finished pairs, routes, handoffs and briefings
together, ran 1,570,047 updates in 328 emulator processes per ROM and
compared 16,558 samples and 304 saves; every save written at the same
update by the two ROMs is byte-identical.

Notes on individual missions:

- **Terran.** `tools/sc64_t9_normal_replay.py` drives all ten tapes.
  Terran 2 and 3 ran through a copy that logs Terran 3's
  historical battle counts instead of stopping on them; neither ROM
  logged any. Terran 9 ran through the copy recorded on main that
  logs its 13 checkpoint differences from the historical tape (the
  economy divergence from input 15,000). Those 13 rows are identical
  on both ROMs. Terran 5 is the packaged route: its tape to tick
  19,731, 21 ordinary-control stage scripts to frame 88,045, 734
  neutral updates to a frame-88,779 pause, 13 recorded segments, and
  four scripts that finish the last buildings. It ran in two parts.
  The first part's frame-88,779 save is byte-identical to the save
  the second part started from.
- **Zerg 1, 3, 4, 5, 6** are segment chains; Zerg 3, 5 and 6 start
  from a recorded victory save of the previous chapter. All 123
  segment saves equal the saves the packaged runs wrote.
- **Zerg 9** has no winning route on this machine. Its three pause
  saves are the packaged frame-101 deployment save after the Zerg 8
  handoff, a frame-27,817 road save and the frame-60,060 save of the
  latest recovery attempt.
- **Zerg 10** continues a frame-60,123 pause save written by an
  earlier build to the recorded tape suffix and victory.
- **Protoss 2 and 4.** The surviving tapes do not win on the packaged
  ROM. Protoss 2's tape runs to its end with the mission active, and
  its pause save then continues for 2,500 updates. The Protoss 4 tape
  stops, on both ROMs, at call 86,977, where the driver expects the
  final text page. Both records are identical through that point and
  end with the same driver error, and the frame-986 pause save equals
  the packaged one.
- **Protoss 3 and 5** have no route. Each has the packaged frame-101
  deployment save from the previous chapter's handoff and one earlier
  build's mid-mission save (frames 20,000 and 2,700).
- **Protoss 7** continues a frame-76,682 save from an earlier build
  with the 905 recorded input runs of the packaged victory.
- **Protoss 6, 8, 9 and 10** are the regenerated host-controller tapes
  that won on the packaged ROM today. Protoss 9 starts from the
  Protoss 8 victory save.
- Victory saves equal the packaged runs' where those were recorded:
  Terran 5 `0a327a1841ff`, Zerg 1 `6122561e55f2`, Zerg 2
  `b17ca47aef03`, Zerg 3 `277b5e01e2e8`, Zerg 4 `a74833e79d0b`,
  Zerg 5 `bd4510087ba2`, Zerg 6 `58fe25565dfc`, Zerg 8
  `f0946a81d56c`, Zerg 10 `eb5b2035d881`, Protoss 1 `d28e7d5faff9`,
  Protoss 6 `b01855f7314b`, Protoss 7 `ad66dc354fd3`, Protoss 8
  `b8624a22d8e7`, Protoss 10 `4bb1ae15b1eb`.
- The finales' handoffs retry the finale, as on the packaged ROMs.
- Most handoffs start from the victory save that the route wrote on
  the same ROM. The Protoss 2–5 handoffs start from retained victory
  saves of earlier builds, because those missions have no winning
  route here, so they check the result screen and the next
  deployment, not a continuous replay into the next chapter.
- A Protoss 4 pair counts as identical rather than matched: the
  recorder's own `compare` requires a zero exit, and the driver exits
  with an error on both ROMs where the tape ends without victory.

## Displayed page

The race cartridges build with `GBA_ASYNC_PRESENT`. `gfx_end` sets
`flip_pending`, and the VBlank interrupt writes DISPCNT once two
hardware frames have passed since the previous flip. Whether the
interrupt has run when a sample is taken depends on how many cycles
the update took, which is what the relink changes. Raw DISPCNT
therefore differs between the two ROMs at a few samples:
53 of the 16,558 compared, all inside a pending flip.

The recorder compares the page due on screen instead: DISPCNT with
the page bit flipped while `flip_pending` is set, and logs raw DISPCNT
and the flag beside it. Probes on all three cartridges read DISPCNT,
`front`, `flip_pending` and both page hashes after each of 305
updates around a save. Raw DISPCNT differed at one update on Terran
and Zerg and at none on Protoss, and in each case one ROM had the
flip pending. The page due, both page hashes, `frame` and `screen`
were equal at every update.

## Stack

The runner records the lowest main-stack and IRQ-stack pointer after
every instruction and reports them at each sample, so a mission's
depth is known for every 100-update interval, every save and every
process end, from reset on. "Boot" is reset to HOME. The other phase
columns give the deepest interval that ended on that screen, so a
"Result screen" figure can come from the battle just before victory.
"Play" includes the campaign menu, briefing and deployment, which fall
in a mission's first interval. The stack top is `0x03007E00`; the IRQ stack top is
`0x03007FA0`.

| Cartridge | Bound (PR #882) | `__iwram_end` | Free at the bound | Deepest that keeps 512 bytes |
| --- | ---: | ---: | ---: | ---: |
| Terran 10 | 7,792 | `0x03005AE0` | 1,200 | 8,480 |
| Zerg 10 | 5,848 | `0x03006108` | 1,568 | 6,904 |
| Protoss 10 | 5,940 | `0x03006018` | 1,716 | 7,144 |

Every mission with a stack-tracked pair, on the relinked ROMs. The
placement audit had not measured Protoss 1–9, Terran 2, Zerg 3 and
Zerg 10; Zerg 9 and Protoss 10 are here as well:

| Mission | Tracked runs | Updates tracked | Boot | Play | Pause and save | Result screen | Deepest | Function at the deepest point | IRQ | Under the bound | Free above the code |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: |
| Terran 2 | `t2`, `t2-next` | 29,884 | 2,712 | 4,944 | 5,880 | 5,872 | 5,880 | `memcpy` | 296 | 1,912 | 3,112 |
| Zerg 3 | `z3`, `z3-next`, `z3-brief` | 70,215 | 2,680 | 5,720 | 5,720 | 5,032 | 5,720 | `gfx_blit_sprite_mapped` | 296 | 128 | 1,696 |
| Zerg 9 | `z9-start`, `z9-road`, `z9-late` | 8,442 | 2,680 | 5,032 | 5,032 | n/a | 5,032 | `gfx_ellipse_outline_rows` | 296 | 816 | 2,384 |
| Zerg 10 | `z10`, `z10-next`, `z10-brief` | 4,564 | 2,680 | 5,640 | 4,968 | 5,640 | 5,640 | `sc64_weapon_source_vfx_draw_image_frame_resolved` | 296 | 208 | 1,776 |
| Protoss 1 | `p1`, `p1-next` | 15,951 | 2,720 | 5,760 | 4,920 | 5,000 | 5,760 | `gfx_blit_sprite_mapped` | 296 | 180 | 1,896 |
| Protoss 2 | `p2`, `p2-cont`, `p2-next` | 38,915 | 2,720 | 5,760 | 5,680 | n/a | 5,760 | `gfx_blit_sprite_mapped` | 296 | 180 | 1,896 |
| Protoss 3 | `p3-start`, `p3-mid`, `p3-next`, `p3-brief` | 7,599 | 2,720 | 5,680 | 5,016 | n/a | 5,680 | `sc64_weapon_source_vfx_draw_raster` | 296 | 260 | 1,976 |
| Protoss 4 | `p4`, `p4-next` | 87,449 | 2,720 | 5,760 | 5,000 | n/a | 5,760 | `gfx_blit_sprite_mapped` | 296 | 180 | 1,896 |
| Protoss 5 | `p5-start`, `p5-mid`, `p5-next`, `p5-brief` | 7,613 | 2,720 | 5,680 | 5,680 | n/a | 5,680 | `sc64_weapon_source_vfx_draw_raster` | 296 | 260 | 1,976 |
| Protoss 6 | `p6`, `p6-export`, `p6-export-next` | 123,505 | 2,720 | 5,760 | 5,016 | 5,016 | 5,760 | `gfx_blit_sprite_mapped` | 296 | 180 | 1,896 |
| Protoss 7 | `p7`, `p7-next`, `p7-brief` | 7,302 | 2,720 | 5,736 | 5,000 | 5,736 | 5,736 | `gfx_blit_sprite` | 296 | 204 | 1,920 |
| Protoss 8 | `p8`, `p8-next` | 101,058 | 2,720 | 5,736 | 4,968 | 5,736 | 5,736 | `gfx_blit_sprite` | 296 | 204 | 1,920 |
| Protoss 9 | `p9`, `p9-next`, `p9-brief` | 138,974 | 2,720 | 5,760 | 4,968 | 5,736 | 5,760 | `gfx_blit_sprite_mapped` | 296 | 180 | 1,896 |
| Protoss 10 | `p10-next` | 429 | 2,720 | 4,960 | 4,920 | n/a | 4,960 | `gfx_blit_sprite` | 296 | 980 | 2,696 |

Byte counts. Terran 2 is the one Terran mission tracked here; the
placement audit measured the others' pause saves at up to 5,928 bytes.
The deepest points are drawing (sprite blitters, weapon effects and
selection ellipses) and the pause save's `memcpy`. The previous link
reached the same low-water at every mark. At each process end the
recorder also finds the lowest non-zero byte between `__iwram_end`
and the stack top. In all 107 processes it lies at or above the
tracked low-water, by 0–68 bytes where the deepest words pushed were
zero, so the tracked figure is the deeper of the two.

## Second pass on the PR tip

`make` and `make SC64_IWRAM_HOT=0` of PR #882's tip `18e9d57c` in one
tree give Terran `b8b38ec719ad`/`2ecbb8747ac4`, Zerg `d783814ad792`/`a3cbd4dc699f` and Protoss `57fae6c8bc5a`/`be835c5e9b6e` (relinked/previous). `__iwram_end` and the stack-check bounds
are those in the table above. The shorter pairs and the stack-critical
routes ran again on this pair:

26 pairs, all matched: `p1`, `p2`, `p3-brief`, `p3-mid`, `p3-start`,
`p5-brief`, `p5-mid`, `p5-start`, `p7`, `p7-brief`, `p9-brief`, `t1`,
`t10`, `t2`, `t4`, `t6`, `t7`, `z1-opening`, `z10`, `z10-brief`, `z2`,
`z3-brief`, `z5`, `z9-late`, `z9-road`, `z9-start`. They ran 202,510
updates per ROM and compared 2,213 samples and 69 saves.

Stack on the relinked tip ROMs:

| Mission | Tracked runs | Updates tracked | Boot | Play | Pause and save | Result screen | Deepest | Function at the deepest point | IRQ | Under the bound | Free above the code |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: |
| Terran 2 | `t2` | 29,455 | 2,712 | 4,944 | 4,168 | 5,872 | 5,872 | `memcpy` | 296 | 1,920 | 3,120 |
| Zerg 3 | `z3-brief` | 542 | 2,680 | 4,960 | 4,960 | n/a | 4,960 | `sc64_source_unit_attack_pose_resolve` | 296 | 888 | 2,456 |
| Zerg 9 | `z9-start`, `z9-road`, `z9-late` | 8,442 | 2,680 | 5,024 | 5,024 | n/a | 5,024 | `gfx_ellipse_outline_rows` | 296 | 824 | 2,392 |
| Zerg 10 | `z10`, `z10-brief` | 4,147 | 2,680 | 5,632 | 4,960 | 5,632 | 5,632 | `sc64_weapon_source_vfx_draw_image_frame_resolved` | 296 | 216 | 1,784 |
| Protoss 1 | `p1` | 15,522 | 2,720 | 5,760 | n/a | 5,000 | 5,760 | `gfx_blit_sprite_mapped` | 296 | 180 | 1,896 |
| Protoss 2 | `p2` | 35,684 | 2,720 | 5,760 | 5,680 | n/a | 5,760 | `gfx_blit_sprite_mapped` | 296 | 180 | 1,896 |
| Protoss 3 | `p3-start`, `p3-mid`, `p3-brief` | 7,182 | 2,720 | 5,680 | 5,016 | n/a | 5,680 | `sc64_weapon_source_vfx_draw_raster` | 296 | 260 | 1,976 |
| Protoss 5 | `p5-start`, `p5-mid`, `p5-brief` | 7,196 | 2,720 | 5,680 | 5,680 | n/a | 5,680 | `sc64_weapon_source_vfx_draw_raster` | 296 | 260 | 1,976 |
| Protoss 7 | `p7`, `p7-brief` | 6,861 | 2,720 | 5,736 | 5,000 | 5,736 | 5,736 | `gfx_blit_sprite` | 296 | 204 | 1,920 |
| Protoss 9 | `p9-brief` | 568 | 2,720 | 4,968 | 4,968 | n/a | 4,968 | `sc64_source_unit_attack_pose_resolve` | 296 | 972 | 2,688 |

## Limits

- These are mGBA emulator runs. Hardware timing, hardware saves and
  real cartridges remain open.
- The comparison covers what the recorder samples. It does not see
  OAM or the I/O registers other than DISPCNT, and between samples
  it sees only button changes. A difference that appears and
  disappears within 100 updates without changing any later sample,
  save or button would be missed.
- Zerg 9, Protoss 3 and Protoss 5 have pause saves only, and the
  Protoss 2 and 4 tapes do not reach victory. Their late game and
  result transition on the relinked ROMs are unreplayed.
- The recorder gained its bypass and per-process export-count checks
  during this work, after most pairs were recorded. The drivers used
  here send buttons, updates and exports only through the transport's
  methods, and a stray run command would have failed the transport's
  own update-counter check on the next advance.
- Several drivers adapt to game state, but each ran once per ROM.
  Neither ROM was asked to recover from a different state.
- Terran 1 and 3–10 and Zerg 1, 2 and 4–8 ran without stack
  tracking. The placement audit measured pause saves of those
  missions (up to 5,928 bytes on Terran and 5,720 on Zerg), and the
  build's static bound covers them; their whole routes are unmeasured.
- The second pass covers the pairs listed there. The other full routes
  ran only on the `e9d0f6c1` pair; the tip changes engine code, so
  they are evidence for the placement, not for the tip's own bytes.

The routes, saves, drivers and both runners are private local files.
`tools/sc64_physical_record.py` and its unit test are the public part.
Any replay tool that drives `tools/sc64_headless_physical.py` can be
recorded on two ROMs and compared the same way.
