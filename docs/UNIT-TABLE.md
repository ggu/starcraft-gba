# Large unit tables

The release race ROMs hold 192 (Terran) or 240 (Zerg Z10, Protoss P10) unit
slots. Unit slot numbers used to be stored in bytes (255 = none) in several
tables, so 254 was a hard ceiling. This document describes the layout that
lifts it, the Swarm profile that uses it (408 slots: 400 Zerglings for one
player at 200 supply), and the measurements behind each step.

All cycle counts below are mGBA guest cycles (emulator, not hardware), from
the headless replay runner and the instruction-level profiler kept with the
private measurement inputs. They do not establish physical-GBA performance;
see [Validation](VALIDATION.md).

## Layout

| Storage | Release ROMs (≤ 254 slots) | Larger tables |
|---|---|---|
| Unit slot numbers in lists (`Sc64UnitIx`, `sc64_unit_index.h`) | u8, 0xff = none | u16, 0xffff = none |
| Collision bucket heads (`sc64_collision_legacy.h`) | byte per cell | byte per cell + 1-bit plane (nine bits, +2 KiB) |
| Visibility source counts (`sc64_visibility.h`) | nine bits | ten bits (2-bit high plane) once actors can exceed 511 |
| Impact rows, range-route jobs | nine-bit target | ten-bit target in the same words, once actors exceed 512 |
| Audio actor snapshot (`game_audio.h`) | VRAM arena | units in VRAM, buildings in work RAM, once the arena is full |
| Saved units (`sc64_zp_unit_stream.h`) | fixed 35-byte rows | delta-coded stream under its own save tags |

Each alternative is selected by the table size at compile time, so the
release ROMs are byte-identical to the revision before this work; their code,
RAM and cycle counts are unchanged by construction.

`SC64_ZP_UNIT_SLOTS` sizes the unit table of a full-storage (Zerg/Protoss)
profile; the default is 240.

### Unit save stream

Fixed rows fit 240 units in one 16 KiB SRAM record. Past 240 slots a save
stores up to 15 template units (the first unit of each owner and type) and one
byte-aligned row per non-zero slot: its base (a template or the all-zero
unit), position, a 25-bit mask of the fields that differ from the base, and
those fields at the fixed-row widths. Combined actor references (`target`,
`build`) take one more bit, so references up to 1023 fit. A per-slot offset
table lets every slot decode on its own, so the existing per-slot save
validators work unchanged. The format is canonical (a field is present exactly
when it differs, padding is zero, templates store position 0,0) and the whole
stream is checked before a load. A world that does not fit is refused with
SAVE FAILED, never truncated. Stream saves use different tag bytes, so a
fixed-row save never loads into a large-table ROM or the reverse.

### Crowd target search

Only a table wider than the release one can hold more than 240 live units.
While it does, attack-moving units search for the nearest enemy on one update
in four (staggered by slot) and keep their last target between searches while
it is alive and within range ([`sc64_crowd_slice.h`](../starcraft/sc64_crowd_slice.h)).
Movement, collision and combat run every update. The kept target is stored in
`Unit.target`, so saves resume it. With 240 or fewer units alive, every unit
searches every update, as in the release engine.

## Swarm profile

`make zerg-swarm` builds `build/StarCraft-Zerg-Swarm-Transport1-GBA.gba`: the
Zerg Z10 release (campaign and custom games) with 408 unit slots.

- RAM: `__bss_end` 0x0203d820 (2,016 bytes below the 0x0203e000 guard);
  `__iwram_end` 0x03004e88 (376 bytes below 0x03005000). Z10 has 21.7 KiB
  and 656 bytes free at 240 slots.
- 408 slots are shared by all players: one player reaches 400 Zerglings only
  while the others field at most eight units.
- `make test-sc64-zerg-swarm` (needs the private Zerg inputs) stages 400
  Zerglings, checks the collision index, saves from the pause menu, reloads
  and requires 600 identical updates.

## Measurements

### Baseline (revision 95a435c7)

Busiest 1,000 consecutive play updates of a three-bot custom game on map 1
(human Zerg; bots Protoss, Terran, Zerg), replayed from a HOST controller tape
as physical buttons. Two runs of each tape are byte-identical. `loop` is the
whole main-loop iteration (update, audio, drawing and the wait for the next
video frame); `upd` is `game_update` alone. Means per update:

| ROM | window (updates) | units / buildings at start | loop | upd | upd p95 |
|---|---|---|---:|---:|---:|
| Terran | 10,803–11,802 | 135 / 51 | 1,105,069 | 633,894 | 873,883 |
| Zerg Z10 | 10,793–11,792 | 142 / 54 | 1,304,159 | 766,867 | 1,063,846 |
| Protoss P10 | 10,825–11,824 | 146 / 52 | 1,435,656 | 902,904 | 1,189,026 |

Crowd scene (`SC64_CROWD_PROBE=200`, [`sc64_crowd_probe.h`](../starcraft/sc64_crowd_probe.h);
`make build/StarCraft-Crowd-200-t240.gba`): 200 Zerglings attack-move into
eight Marines guarding a Command Center on the Zerg Z10 engine, 1,500 updates
from boot. Mean `upd` 1,642,703 and `loop` 2,043,660 cycles over all updates;
2,003,945 / 2,475,664 over the march and fight (updates 2–500).

### Release ROMs after every step

Every commit of this work leaves the Terran, Zerg Z10 and Protoss P10 release
ROMs byte-identical to the baseline, and the replayed windows give exactly the
baseline numbers above (+0.00% loop and update cycles, same workload). The
other verify ROMs that build here (Showcase, Zerg Z3/Z4/Z6/Z8, Protoss
P3/P7/P8/P9, Backwater, skirmish, unified and race targets) are byte-identical
too. Zerg Z7 (IWRAM reserve) and Protoss P6, campaign and the original
StarCraft 64 target (missing private inputs) fail the same way on the
baseline.

### Swarm ROM at today's counts

The same Zerg window on the Swarm ROM plays the identical game (all sampled
state matches) at **+8.30% loop cycles** (1,412,377 vs 1,304,159) and +8.43%
update cycles. The extra cost is loops that visit all 408 slots: drawing
(+26k cycles per update), the custom-game defense scan (+17k), the death-effect
scan (+9k), selection helpers (+12k) and collision bucket setup (+17k). Bounding
those loops by the highest live slot would recover most of it.

### Crowds

Crowd scene, 1,500 updates from boot (Zerglings attack-move into eight
Marines, then stand). Means per update:

| ROM | Zerglings | upd, all | upd, updates 2–500 | loop, all | loop, updates 2–500 |
|---|---:|---:|---:|---:|---:|
| Zerg Z10 (240 slots) | 200 | 1,642,703 | 2,003,945 | 2,043,660 | 2,475,664 |
| Swarm (408 slots) | 200 | 1,723,862 | 2,091,473 | 2,170,613 | 2,608,707 |
| Swarm, search every update | 400 | 3,492,134 | 3,984,210 | 4,113,513 | 4,677,717 |
| Swarm | 400 | 3,176,580 | 3,859,590 | 3,787,874 | 4,553,083 |

- 400 Zerglings cost 1.84 times the update cycles of 200 on the same ROM
  (1.93 times the release ROM's 200). The loop takes 13.5 video frames
  (about 4.4 updates per second) against 7.3 (8.2 per second) for 200 on the
  release ROM; the game's nominal pace is one update per two frames.
- Quarter-rate target search saves 9.0% of update cycles and 7.9% of loop
  cycles at 400. It is inactive at 200, which plays the same game on both
  ROMs.

Where 400 Zerglings spend an update (Swarm, updates 101–700, inclusive
cycles, 4,285,348 per update):

| Work | Share |
|---|---:|
| Movement (`move_toward`), of which collision masks 23.6% and bucket steps 8.8% | 44.8% |
| Fog and sight (`sc64_update_exploration`) | 26.6% |
| Drawing (`game_render`) | 14.1% |
| Target index setup | 2.3% |
| Audio interrupt | 2.2% |
| Target search (after slicing) | 1.6% |

Target search was about 6% before slicing. The remaining cost is per moving
unit every update (collision and sight), which the slicing deliberately leaves
alone because it changes movement and fog.

A lighter record for small ground units was considered and not built: the
Swarm table fits RAM with the existing 56-byte `Unit`, and the cost above is
CPU work per moving unit, not record size.

### Save

An ordinary pause-menu save with 395 Zerglings and the defenders uses
4,207–4,257 of the 7,500 unit-stream bytes. The record keeps room for 12
pending impacts after the units (the save code requires at least eight).
