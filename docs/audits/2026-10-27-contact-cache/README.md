# Contact cache: cached collision facts and one-pass steering

`SC64_CONTACT_CACHE` (`starcraft/sc64_contact.h`) is enabled in the canonical
Terran, Zerg and Protoss ROMs through `CONTACT_CACHE_FLAGS` and in the new
`protoss-p7-contact-probe`. It changes how the race collision query and the
blocked-mover steering are evaluated, not what they decide.

## Where the cycles went

An instrumented P7 movement probe (private scratch build, timers around each
component, same inputs and controller tape as the public
`tools/sc64_p7_move_workload.py` run, 600 neutral then 1,500 attack-move
updates, about 178 live units) attributed the movement cost per update as
follows. Timer reads add about 30 cycles to each timed segment, so per-call
values are upper bounds.

| Component | Opening cycles/update | Attack cycles/update | Note |
| --- | ---: | ---: | --- |
| `move_toward` inclusive | 482,242 | 787,558 | 136 / 118 calls |
| Blocked steering (eight offset queries) | 137,276 | 409,009 | 18.5 / 40.3 blocked events, 7–10K each |
| First `unit_step_clear` | 115,873 | 181,845 | about 270 cycles per bucket candidate |
| Stock speed lookup (ROM) | 72,749 | 55,594 | 766 cycles per call |
| Route next tile | 32,197 | 61,996 | almost all in rare 2.77M-cycle field builds |
| `direct_path_clear` | 13,029 | 5,734 | |
| Bucket and target index moves | 35,527 | 24,825 | |

The per-candidate cost came from repeating type-derived fact lookups for
every neighbour: collision radius, flight, collidable flags and a ROM call
for burrow eligibility, and the blocked path repeated the whole query for
each of its eight steering offsets.

## What changed

- Each unit type's collision facts are cached once in a 256-entry byte table
  (radius, flying, chrysalis/non-collidable, burrowable-from-type). The facts
  are pure functions of the type within a build, so the table never needs
  invalidation; types or radii the byte cannot describe use the original
  lookups for that actor. The P6 burrow test still reads the live flag and
  the pinned `raw_type` per candidate; `race_p6_active()` is read once per
  query instead of once per candidate.
- A blocked ground mover evaluates all eight steering offsets in one pass
  over the union of their bucket windows (`sc64_contact_blocked_mask`). The
  query is an any-blocker predicate, so candidate order cannot change an
  answer, and the direction loop keeps its cost comparison, traversability
  test, iteration order and first-best tie-breaking.
- The stock movement rate replaces `(n) / 5` by `(n * 52429) >> 18`, which
  is exact for every `n` below 81,920; the profile table's largest value
  gives `n = 68,262`. The step amount, phase and diagonal scaling are
  unchanged.
- The original query stays in ROM as `sc64_unit_bucket_step_clear_reference`
  for the fallback paths and the host differential tests. The late Terran
  profile keeps the steering pass in ROM (`SC64_CONTACT_STEER_ROM`) because
  its IWRAM reserve has no room for it.

Movement speeds, cadence, path timers, route fields, collision geometry,
terrain traversal, target selection, worker trips, placement, topology
invalidation and the save formats are untouched. The option is incompatible
with `SC64_COLLISION_CLASS_CACHE` and `SC64_COLLISION_STEER_LIST`, which it
supersedes; the Z7 probe targets that use those flags are unchanged.

## Equivalence evidence

`make test-sc64-contact-cache` builds `tests/sc64_contact_cache.c` under four
flag sets and runs it with ASan/UBSan: unified race (`RACES_FLAGS`), P7
(`P7_MOVE_BASE_FLAGS` + `P7_CONTACT_FLAGS`, with P6 burrow checks live in a
quarter of the worlds), canonical Terran (`TERRAN_ROM_FLAGS`, persistent
collision) and the Nydus Z7 probe (strict placement mode). Each variant runs
400 randomized worlds for three passes without cache resets, with clusters at
map edges and corners, workers gathering, chrysalises, loaded units, burrow
raw types and flags, stray `raw_type` values, and mid-pass morphs, deaths,
slot reuse, loads, unloads and bucket moves. It compares every alive unit's
cached query against the reference on 13 offsets, the steering mask against
eight `unit_step_clear()` calls at speeds 1 and 2 (also with the bucket index
switched off and after the legacy dirty-list overflow), and the step amount
against `sc64_stock_motion_step()` for raw ids −48..319 and extremes.

| Variant | Step checks | Strict checks | Mask checks | Inactive-index mask checks | Stock checks | Failures |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Unified race | 1,961,570 | 0 | 301,780 | 79,698 | 104,048 | 0 |
| P7 | 3,287,076 | 0 | 505,704 | 130,504 | 171,024 | 0 |
| Canonical Terran | 2,674,113 | 0 | 411,402 | 106,460 | 142,320 | 0 |
| Nydus Z7 | 3,291,197 | 3,291,197 | 506,338 | 130,292 | 171,024 | 0 |

An earlier revision of the header used a 9-pixel candidate window for every
mover; this fixture caught that flying Z/P movers need the 18-pixel window
(radii up to 19) before any ARM measurement was taken.

`make test-sc64-p7-contact` reproduces the plain P7 reference trace byte for
byte: a per-tick FNV hash of every unit, building, P7 runtime and wallet
field over the 1,200-update neutral opening and the 1,500-update
controller-only attack-move (`852c1981` / `ee09572b`), then the menu,
trigger and SRAM save/load fixture output.

## ARM measurements

The public P7 movement workload (`tools/sc64_p7_move_workload.py`, 600
neutral then 1,500 attack-move updates, controller input only, about 178
live units on the 128×128 map) was run on the existing movement probe as
the baseline and on the new contact probe, both built from the same shared
private inputs. The candidate matches all 23 state checkpoints (actor, map,
campaign, runtime, wallet, supply, upgrade and fog bank hashes, decoded
actors, per-owner HP and selection counts), the 25 injected inputs and the
terminal state (PLAY, frame 2,605, no win or loss, no fault).

| Workload | Baseline cycles/update | Contact cache cycles/update | Change | Baseline fps | Contact cache fps |
| --- | ---: | ---: | ---: | ---: | ---: |
| Opening, 600 updates | 1,582,381 | 1,426,952 | −9.8% | 10.603 | 11.757 |
| Attack-move, 1,500 updates | 1,890,055 | 1,551,295 | −17.9% | 8.877 | 10.815 |
| Worst 100-update interval (attack+1300) | 2,398,126 | 2,058,592 | −14.2% | | |

The hardware-timer intervals include the VBlank wait, so an update that
finishes early still spends whole 280,896-cycle frames; several candidate
intervals sit exactly at five frames (1,404,480 cycles) and understate the
CPU saving. The uninstrumented diagnostic counters sampled on the last tick
of each of the 23 checkpoints give the direct CPU view:

| Last-tick counter, mean of 23 samples | Baseline | Contact cache | Change |
| --- | ---: | ---: | ---: |
| `demo_update_cycles` (whole simulation update) | 1,228,985 | 966,880 | −21.3% |
| `sc64_orders_cycles` (orders stage, movement included) | 870,163 | 608,211 | −30.1% |
| `gfx_draw_cycles` | 423,506 | 423,140 | −0.1% |

Route counters are identical in both runs (29,822 hits, 40 field builds),
as expected for an unchanged route cache. Against the instrumented
attribution above (movement about 482K / 788K cycles per update), the
orders-stage saving of about 262K cycles per sampled tick is roughly a
third to a half of the movement cost; the arbiter refresh, target queries
and route field builds in that stage are untouched.

ROM SHA-256: baseline movement probe
`850b8bd8777ba790a720758ebd5ac06af345ea052b683e75000ef7ee608971f8`,
contact probe
`885864938e7a35435b36a08e82408c6e2fb83619c699bfa349a812f2ccbcdf1d`.

### Canonical Terran skirmish

The canonical Terran ROM (`make terran`, SHA-256
`5c92b134e42a537c22c4c7a472b1f96a4678279d09543eab5102d9859b637deb`) was
measured with `tools/sc64_split_profile.py --bots 3 --ticks 10000` for
map 0 / Terran human and map 1 / Protoss human against the base ROM
(`184bef4597611a21ca208662fcee9c4561063d002de8a5ab5372eac3a79e9124`).
Every checkpoint's gameplay state (resources, supply, every unit and
building record) matches in both configurations: 10 and 11 checkpoints.
These skirmishes stay near the frame cap (27.8 / 28.4 mean fps with at most
53 live units), so their interval cycles change by less than 0.4%; they
serve as equivalence evidence, not as a crowded benchmark. The profiler
needed a small bank-size fix (`tools/sc64_races_profile.py`) to read the
canonical 192-unit / 96-building late Terran layout.

The Terran host suite's late controllers reproduce the base tree output
exactly with the cache compiled in: T7 and T8 win, T10 wins at tick 3,511,
and T9 reaches its 500,000-tick limit with byte-identical trace lines on
both the base tree and the candidate (a pre-existing limit outcome with the
local inputs on this machine, not a behaviour change).

### Reproducing

```sh
make protoss-p7-move-probe protoss-p7-contact-probe
python3 tools/sc64_p7_move_workload.py --elf build/starcraft-protoss-p7-move-probe.elf \
    --rom build/StarCraft-Protoss-P7-Move-Probe-GBA.gba --out build/p7-move-base \
    --neutral-ticks 600 --attack-ticks 1500
python3 tools/sc64_p7_move_workload.py --elf build/starcraft-protoss-p7-contact-probe.elf \
    --rom build/StarCraft-Protoss-P7-Contact-Probe-GBA.gba --out build/p7-move-contact \
    --neutral-ticks 600 --attack-ticks 1500
python3 tools/sc64_p7_move_workload.py --compare build/p7-move-base/result.json \
    build/p7-move-contact/result.json
make terran
python3 tools/sc64_split_profile.py --profile terran --map 0 --race 0 --bots 3 --ticks 10000
```

Build the base and candidate back to back from the same prepared inputs
(the importers rerun when their scripts are newer than the generated
headers), keep matching ROM/ELF pairs, and compare only runs with the same
inputs and emulator setup.

## Memory and stack

| Build | IWRAM end (base → candidate) | BSS end (base → candidate) |
| --- | --- | --- |
| P7 move probe → P7 contact probe | `0x03004c28` → `0x03004c18` | `0x0203ded4` → `0x0203dd60` |
| Canonical Terran | `0x03004e08` → `0x03004eb0` | `0x0203de94` → `0x0203df94` |

Both guards (`0x03005000` IWRAM, `0x0203e000` BSS) hold unchanged. The P7
probe drops the 240-byte collision-class cache and the 388-byte static
steering list in favour of the 256-byte type table, and inlines the
division-free stock step into the IWRAM mover (not in the late Terran
profile). `-fstack-usage` frames:
`sc64_contact_blocked_mask` 168 bytes (Terran ROM placement), `move_toward`
152 (P7) / 64 (Terran), `unit_step_clear` 112 / 88, reference query 128 / 88,
cold helpers 16–56 bytes; the blocked path no longer carries the 388-byte Z7
steering context on the stack.

## Limits

- The Zerg Z6 and Protoss P6 canonical sprite banks are not prepared on the
  measuring machine, so those two ROMs were compiled only through their host
  flag sets (unified race and P7 variants above); their ARM link and timing
  are not measured here.
- Route field builds (about 2.77M cycles each on the 128×128 P7 map) are
  untouched; they remain a per-new-goal hitch.
- These are emulator cycle measurements on bounded workloads, not physical
  hardware results, and no near-30-fps claim is made for the P7 workload.
