# Custom-game opponents: full-match reliability

Base `0c11f910` (`codex/source-release`). All planner changes are in
`starcraft/race_skirmish_ai.h`. No save format, enum ID, rule value, actor
cap, timing constant or experimental flag changed. The opponents still have
full knowledge of hostile actors, and nothing here is a claim about the stock
StarCraft 64 AI.

## Evidence classes

| Class | What it is here |
| --- | --- |
| HOST fixture | `tests/sc64_skirmish_ai_progress.c` built per canonical race product. `--mirror` lets the planner drive the human owner as well, so a match runs to the engine's own victory or defeat. Scenario fixtures inject losses through `hit_target` and exhaust mineral fields; they never give an opponent anything. |
| HOST ordinary input | `tests/sc64_skirmish_campaign_run.c`: a scripted human pressing normal buttons. |
| Emulator | mGBA core through `tools/sc64_headless_replay.c`, unchanged ARM ROMs, menu buttons only. |
| Hardware | None. Nothing here was run on a GBA. |

## Baseline

A passive human loses to the first attack at about six and a half minutes, so
the existing harness never saw a mid or late game. With `--mirror`, 288
matches were run on unsanitized `-O2` builds of the same sources: three
products × two maps × three human races × 16 opponent mixes (one, two and
three opponents, same-race and mixed), capped at 216,000 updates (two hours),
with a snapshot every minute.

| Failure at base | Evidence |
| --- | --- |
| 17 of 288 matches never ended | An attack objective stayed live while any hostile structure stood within 128 px of it. Attack-move acquires targets within 64 px, so a structure 65 to 127 px away held the army at a dead objective for the rest of the match (12 of the 17: a Missile Turret at 103 px, a Gateway at 122 px, a Pylon at 127 px and so on). |
| A lone unit never reached its target | A group order spread units over a 5×3 block starting at the corner slot. One Dragoon or Tank sent against a structure in a map corner got a slot walled off by that structure and stood still for over an hour. |
| Protoss stopped building | Powered structures were searched for only on rings around the Nexus, which often miss every psi field. In 84% of the planning passes where a Protoss opponent held 1,500 or more minerals, a build step had no placeable site. A Nexus whose Pylons survived only at a lost expansion had enough supply and so never got a new Pylon: 4,787 minerals, nine Probes and no Gateway for 90 minutes. |
| Minerals piled up behind gas | A gas reservation for a later step also refused units that cost no gas. A producer whose pick needed gas stayed idle. 74% of the Terran passes with 1,500 or more minerals had a unit refused for gas. |
| Ground armies pinned by aircraft | Any armed hostile near a depot was a threat, including Scourge (no ground weapon) and aircraft over an owner with no anti-air. Every home unit attack-moved under it each pass and never left. |
| Exhausted owners held unusable money | With no ore left, workers were still trained up to eight, tech and supply reservations held the last minerals back from army (a Terran bank frozen at 85 with three idle Barracks), the pick stayed a unit the bank could not pay for, and home units waited for a 12-supply group that could never form. Workers attacked only below 50 minerals, which a Protoss owner with 62 could neither spend nor fall below. |
| Workers thrown away with ore on the map | The exhausted-map worker attack looked only at the owner's own fields. Both owners of a two-player match lost every worker with 21,552 minerals still on the map. An owner's last worker also stayed on gas, with ore left and nothing to train another. |
| Protoss never rebuilt a lost Nexus | The existing `--recovery 3` fixture (every depot destroyed with 450 or more minerals banked) failed for a Protoss opponent in all four profiles at base. Supply was planned before the depot: the lost Nexus's supply was replaced with Pylons until the Nexus price was gone. |
| An owner could lose its economy for good | After a raid killed its workers, an owner spent its last minerals on army and ended at 19 minerals with a town hall, larvae and 5,560 ore nearby. |

Not defects: idle workers (97 to 99% of the idle passes had no ore left on the
map), supply (blocked in under 1% of passes) and shared capacity. With three
opponents the unit pool peaked at 176 of 192 (Terran product) and 201 of 240;
the building pool at 87 of 96 and 89 of 160. The pools never filled, and the
slots left to the human were never below an equal share.

## Changes

1. An objective is held only by a hostile structure or ground unit within 64 px.
2. The first unit of a group order takes the ordered point; a slot on blocked
   terrain falls back to it.
3. A Protoss structure that needs power gets nine more candidates around a
   completed home Pylon when the Nexus ring has none, within the same budget
   of eight expensive checks. A Nexus with nine workers and no Pylon within
   256 px gets one.
4. Gas reservations no longer block gas-free purchases. A gas-short Terran or
   Zerg producer trains its gas-free line unit once 400 minerals are free.
5. With no ore left on the map: no workers, no tech, town-hall or supply
   reservation, and the line unit whenever the pick cannot be paid for. While ore is left
   and fewer than four workers remain, army production leaves a worker's
   price in the bank.
6. Units with no ground weapon are not base threats. Flying threats count
   only while the home army can shoot air.
7. An owner that is all in sends home units to a deployed army at once. The
   exhausted-map worker attack triggers below the price of the race's
   cheapest army unit.
8. With every depot lost, rebuilding one is planned before supply.
9. The worker attack needs no ore left anywhere, not only at home. No gas
   worker is kept when none is wanted.

Tried and dropped, with the measurements that decided it:

- One more pending supply provider when nearly blocked, and one more per two
  Hatcheries. Zerg army at four minutes on Blood Bath fell from 16 to 6
  supply: Overlords take larvae from the same serial queue.
- An earlier second Hatchery and a looser Drone rule. Zerg opponents alive at
  the end fell from 23–34% to 15–21%.
- Searching around Pylons instead of around the Nexus on alternate passes.
  Protoss production on Outpost at four minutes fell from 3.1 to 2.3
  structures; as an extra attempt after a failed search it rose to 4.2.

## Results on the same 288 mirror matches (HOST, `-O2`)

Base ran with the first version of the mirror fixture. The final fixture also
gives the mirrored human the cargo-resume service the engine gives opponents;
the tables below are about opponent owners.

| | Base | Candidate |
| --- | --- | --- |
| Ended in the engine's victory or defeat | 271 | 281 |
| Open at two hours, Blood Bath (144 matches) | 10 | 0 |
| Open at two hours, Outpost (144 matches) | 7 | 7 |
| Mean match length (updates) | 36,876 | 27,270 |

The seven open candidate matches are all four-owner Outpost games with every
mineral field mined out. Six are dead worlds: no owner has an army, a worker
or the price of an army unit. The engine has no draw rule; see Open items.
The seventh (`zerg-m0-t-zzz`) is the melee-reach defect below: six Zerglings
have acquired a Terran Machine Shop and cannot reach it.

Opponent owners only, averaged over the snapshots in which the owner still
had a building:

| | Terran base → cand. | Zerg | Protoss |
| --- | --- | --- | --- |
| Snapshots with 1,500 or more minerals | 19.9% → 14.3% | 8.2% → 9.7% | 32.0% → 11.7% |
| Peak bank (mean) | 1,624 → 1,472 | 1,807 → 1,585 | 2,470 → 1,506 |
| Peak army supply (mean) | 85 → 93 | 39 → 39 | 77 → 94 |
| Snapshots with no worker | 4.1% → 5.7% | 12.7% → 10.7% | 7.6% → 16.1% |

The no-worker share rises for Protoss because exhausted owners now stop
training Probes and send the ones they have to attack at under 100 minerals.

Opponents with a depot at six minutes, mean army supply and production
structures:

| | Outpost base → cand. | Blood Bath base → cand. |
| --- | --- | --- |
| Terran | 84 → 94, 5.7 → 5.5 | 80 → 83, 5.5 → 5.4 |
| Zerg | 44 → 45, 5.0 → 5.0 | 27 → 26, 3.1 → 3.1 |
| Protoss | 77 → 97, 3.8 → 4.8 | 65 → 86, 3.2 → 4.7 |

Single matches are chaotic: every match ends on a different update after any
planner change, and winners flip. The tables compare aggregates, not
matches.

## HOST checks on the candidate

- `tools/sc64_skirmish_ai_matrix.py --profiles terran,zerg,protoss` on the
  sanitizer builds: 330 of 330 cases pass, with no sanitizer report. That
  is 110 per product: the earlier progression, recovery, builder, worker,
  addon, commit, morph and save/resume cases, 57 mirrored full-match cases
  in all, the spending-audit controls and the re-power, air-threat and
  no-ore fixtures. Of the 57 match cases, 55 end in victory or defeat and
  two are the same Zerg-product lobby ending as a dead world, once as an
  ordinary case and once as the case that expects the draw. The Showcase
  profile's inputs were not available.
- At base, 14 of the 272 earlier cases failed: Protoss never rebuilt a lost
  Nexus (four profiles), four 36,000-update runs failed their checks, and
  six Sunken-morph save cases lost the passive human before or just after
  the save. The morph cases now run mirrored so the human owner is still
  standing through the save and the continuation.
- The re-power, ground-army and no-ore fixtures fail on the base planner and
  pass on the candidate. The anti-air scenario of the air-threat fixture is
  a control against over-correction and passes on both. Of the chosen match
  lobbies, base fails 12 (never ending, 73 to 118 idle snapshots, or 100 and
  more floating snapshots).
- Spending audit: zero violations over all 288 lobbies for both planners. It
  flags the one unpaid structure or worker the control hands the human.
- Save and fresh load through the pause menu, reloaded into a reinitialised
  game, with the continuation compared against the uninterrupted run: the
  matrix's `resume`, `resume-late` and `resume-morph` cases pass on the
  final planner in all three products (passive-human saves from update
  9,000, moved earlier when the match is shorter, and mirrored saves while
  a Sunken Colony morphs at about update 18,500). On an earlier candidate
  (before changes 8 and 9 and the supply part of 5), mirrored
  three-opponent saves at 12,000 and 15,000 also matched in all three
  products, and at 20,000 in the Terran product; the other two matches had
  ended by then.
- Ordinary-input controller (`tools/sc64_skirmish_controller_matrix.py`,
  unsanitized build, the same earlier candidate): all 54 matches end in a
  result at base and with the candidate. The scripted human loses 53 of 54
  either way; two lobbies changed outcome.

## ARM cost (mGBA core, not hardware)

The unchanged Terran, Zerg Z9 and Protoss P10 ROM targets were built at base
and with this change's sources (`starcraft/race_skirmish_ai.h` blob
`6d7354edfc18`), using the same private inputs. Both passed ROM verification and the linker RAM-reserve
asserts. Each ROM ran four lobbies from the HOME menu with button presses
only and a passive Terran human: three opponents on each map, one Zerg
opponent on Outpost and one Protoss opponent on Blood Bath, for 12,000
updates or until the result screen. Cycles are the ROM's own
`demo_update_cycles` (the game update, not rendering), read after every
update together with `frame`.

- **Memory:** `__iwram_end`, `__bss_end`, `.data` and `.bss` are identical
  to base in all three ROMs. No IWRAM or EWRAM symbol was added, removed or
  resized, and every `race_ai_*` function is in ROM. ROM text grows by
  1,472 bytes (Terran), 1,552 (Zerg) and 1,976 (Protoss).
- **Planner cost** is estimated as a planning update minus the mean of the
  updates two before and two after. Only the first 3,000 updates are close
  to like for like; afterwards the two worlds differ.

| First 3,000 updates | Planner cost, mean (base → cand.) | Median | Other updates, mean |
| --- | --- | --- | --- |
| Terran, 3 AIs, Outpost | 76.1k → 78.3k | 58.2k → 58.2k | 129.2k → 131.2k |
| Terran, 3 AIs, Blood Bath | 62.9k → 63.6k | 57.1k → 57.7k | 131.8k → 131.5k |
| Terran, 1 Zerg AI, Outpost | 51.0k → 51.7k | 47.8k → 48.3k | 91.8k → 91.8k |
| Terran, 1 Protoss AI, Blood Bath | 52.6k → 58.5k | 51.5k → 52.1k | 100.9k → 100.7k |
| Zerg, 3 AIs, Outpost | 96.8k → 105.5k | 80.6k → 80.8k | 145.5k → 146.4k |
| Zerg, 3 AIs, Blood Bath | 81.6k → 83.8k | 77.6k → 79.1k | 147.3k → 146.4k |
| Zerg, 1 Zerg AI, Outpost | 80.8k → 81.0k | 70.1k → 69.8k | 103.6k → 103.6k |
| Zerg, 1 Protoss AI, Blood Bath | 78.9k → 92.2k | 73.2k → 74.0k | 112.9k → 112.9k |
| Protoss, 3 AIs, Outpost | 93.5k → 102.3k | 98.0k → 99.4k | 252.3k → 254.0k |
| Protoss, 3 AIs, Blood Bath | 81.5k → 83.3k | 87.5k → 83.8k | 254.2k → 253.7k |
| Protoss, 1 Zerg AI, Outpost | 80.8k → 81.0k | 69.1k → 68.8k | 209.2k → 209.2k |
| Protoss, 1 Protoss AI, Blood Bath | 79.8k → 85.7k | 74.8k → 77.4k | 217.8k → 217.2k |

  Medians move by under 5%. Means rise by up to 17%, most where a Protoss
  opponent plays and barely at all with one Zerg opponent. That is
  consistent with the extra nine site candidates around a Pylon, which run
  only on passes whose first search failed and so show in the tail (largest
  planning update in the Zerg ROM's one-Protoss-AI case: 222k → 660k), but
  the per-function cost was not measured. A planning update is one in 48
  per opponent.

| Whole 12,000-update run, 3 AIs | Game update, mean | Updates over 559,000 cycles |
| --- | --- | --- |
| Terran, Outpost | 360k → 371k | 21.8% → 23.3% |
| Terran, Blood Bath | 365k → 372k | 11.1% → 10.7% |
| Zerg, Outpost | 481k → 492k | 28.0% → 30.5% |
| Zerg, Blood Bath | 440k → 438k | 20.6% → 18.5% |
| Protoss, Outpost | 480k → 495k | 33.8% → 34.4% |
| Protoss, Blood Bath | 503k → 502k | 27.2% → 27.6% |

  The whole-run rows mix planner cost with different scenes: opponents that
  spend their bank field more units. They are not a planner-cost
  comparison, and none of this is a frame rate. Three-opponent games were
  over the 30 fps update budget in 11 to 34% of updates before this change
  and still are.

## Open items found here and not changed

- **Melee reach against Armory, Machine Shop and Control Tower in the Zerg
  and Protoss products (shared mechanics).** `tick_combat` measures reach as
  the weapon range plus 18 px from the structure's center. For the
  structure types that carry source bounds (`sc64_zp_building_fact`: Control
  Tower, Machine Shop, Armory, Queen's Nest, Ultralisk Cavern and others), the
  path grid blocks every tile the bounds touch, which keeps a ground unit 35
  to 48 px from the center on the long side. A Zealot (reach 26 px) can hit
  an Armory only from a narrow band on a short side, depending on how the
  structure sits on the 16 px grid; in open ground a HOST probe shows six
  Zealots destroying one. At base, 40 Zealots stood around an Armory for
  100 minutes with the target acquired and its hit points full
  (`protoss-m0-p-tz`). An intermediate candidate showed five Zerglings doing
  the same at an Armory in a map corner (`protoss-m0-z-pt`). Both matches
  end with the final candidate, but the defect itself is untouched. The fix
  belongs in `tick_combat` for every product. It changes campaign combat, and the
  Zerg and Protoss campaign suites could not be built from the inputs
  available for this work, so it was left for a change that can run them.
- **New workers go to the nearest mineral field anywhere.**
  `race_tick_buildings` and the cargo-resume loop in `race_tick_ai` use
  `nearest_mineral` without regard to the owner. 183 of 18,454 worker spawns
  at base went to a field at another owner's base while the owner's own
  second base still had ore. Shared mechanics, small effect, not changed.
- **No draw rule.** A custom game ends only when the human or every opponent
  has no structure. A world in which no owner can ever act again stays open.
- **Zerg opponents are the weakest.** One serial larva queue per Hatchery
  trains Drones and army, and four-minute banks stay near 1,300 minerals.
  The planner changes tried for it made things worse (above). Parallel eggs
  are a separate, staged engine feature.
- **Outpost is a short map.** Each start has four fields of 1,500 minerals;
  home fields are mined out by about minute seven.
