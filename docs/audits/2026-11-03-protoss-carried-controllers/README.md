# Carried Protoss controllers on the current integration

`make test-sc64-protoss-carried-missions` and `make test-races-controller` were
red on `codex/source-release`. Each failure was a controller defect. This change
touches three test controllers and no engine code.

## Findings

| Mission | Symptom | Cause | Engine bug? |
| --- | --- | --- | --- |
| P1 | Error 22, and about half of start offsets lost | One-shot reinforcement and a thin assault. The route also returned before the campaign tick saw the last building die, and failed if Fenix's parking walk timed out. | No |
| P2 | `lost=1` at tick 45,892 | Idle for 15 minutes, then one attack-move into about 100 units and 14 Sunken Colonies. | No |
| P3 | 180,000-frame limit | Idle Probes never mined. The route also wrote `menu_cursor` directly, so its inputs did not describe its actions. | No |
| P5 | `outcome=2` at frame 5,464 | The approach stayed in Spore Colony range long enough that the Shuttle arrived one hit from death. | No |

**P2** bisects to #133 (first bad `1f875e1d`, last good `4b371730`), which made
human units acquire new targets only where they can currently see. That audit
states that earlier traces relying on the defect cannot qualify a new win, and
this route was one of them. The fog rule stays.

**P5** bisects to #134 (first bad `63665045`, last good `7181c093`), which
shared bounded ranged routing. The Shuttle's flight is unchanged. It arrived at
6 HP on the old route, and the changed timing was enough to lose it. I did not
isolate which hit changed, because the route had no margin either way.

**P3** was already recorded as a "current controller limitation" in
[the P7 candidate qualification](../../research/p7-canonical-candidate-qualification.md).

## What changed

All orders still go through `game_update()` as ordinary key input. Live state
only chooses when and where to act.

- **P1 and P2 base building (`tests/race_campaign_run.c`).** Probes spread across
  patches, Pylons, a Gateway, an Assimilator, a Cybernetics Core, more Gateways,
  then Dragoons. It also powers P1's rescued Gateway, which has no Pylon in range.
- **P1.** The rescue leaves the force over its supply cap (36 of 34), so nothing
  can be trained until the assault has spent some of it. The force attacks first,
  then Fenix walks behind the Nexus. If the army and funds run out, Fenix finishes.
- **P2.** Builds until tick 24,000, defends the mineral line, then presses the
  assault with repeated orders. A box selection leaves Fenix out once he joins.
- **P3 (`tests/sc64_p3_campaign_run.c`).** Sends idle Probes to patches and clears
  Probes parked on a patch. Replaces the `menu_cursor` write with real menu input.
  Gives up on unreachable build orders, which reserve their cost until arrival.
  Drops the Forge, Cannons and spare Gateways for army, and strikes together at
  32 units, regrouping when spent.
- **P5 (`tests/sc64_p5_campaign_run.c`).** Descends the x1632 channel between the
  x1520 and x1760 Spore Colonies. It arrives with more health and wins earlier.
- `--start-delay N` adds N idle frames at mission start to the race controller.

## Results

Run at commit `caa22126` on base `d60e75eb`, under ASan and UBSan, with
inputs cloned from the 2026-10-31 pinned set into a private `.local`:

| Check | Result |
| --- | --- |
| `make test-sc64-protoss-carried-missions` | Pass |
| Carried P1 | 25,167 input frames |
| Carried P2 | 34,375 input frames |
| Carried P3 | Win at frame 48,961 |
| Carried P5 | Win at frame 5,056 |
| `make test-races-controller` (Z1, Z2, P1, P2) | Pass |
| `build/test-unified-campaign --controller 1/2/3` | 8,550 / 16,944 / 35,226 frames |
| `build/test-sc64-p3-controller` | Win at frame 45,535 |
| `build/test-sc64-p7-campaign-early --controller 2/3` | 16,938 / 34,050 frames |

Wins out of 144 start offsets (0 to 143 idle frames), in scratch builds without
sanitizers. Outcomes cluster, so treat the counts as coarse.

| Controller | Races build | Carried build |
| --- | ---: | ---: |
| P1 | 139 | 136 |
| P2 | 137 | 130 |

Before this change P1 won 19 of the first 36 offsets and P2 won none of the 12
offsets tried. P3 and P5 have no offset option, so each has one run per profile.

## Still failing

- `make test-sc64-p7-campaign-composition` still stops at its P3 step. That step
  runs `tests/sc64_p3_expansion_run.c`, a separate larger-bank expansion route
  this change does not touch, and it reaches the 180,000-frame limit.
- P1 and P2 still lose some start offsets. Those I inspected were ordinary
  attrition: the army runs out with a few buildings left and no minerals.

## Engine limits found, not changed

These shaped the routes and affect real players. They are follow-ups.

- **Idle units do not step aside.** Fenix stays blocked behind an idle Dragoon at
  a narrow point for as long as it stands there.
- **Spent patches leave Probes idle.** Workers do not move to the next patch.
- **An idle Probe holding cargo cannot be sent to mine.** The mineral command skips
  loaded workers, and nothing returns its cargo. Open draft #111 changes this area,
  so it is not duplicated here; these routes build only with empty Probes.
- **Canonical Protoss delivers to the first Nexus.** `SC64_NEAREST_RESOURCE_DEPOT`
  is not in the Protoss profile, so Probes mining at an expansion carry cargo across
  the map. These routes therefore stay on their home patches.

## Not claimed

These are host controller results only. No input-only ARM replay, fresh-process
save or timing was run for these missions at this commit, and nothing is promoted.
The earlier P1-to-P5 ARM evidence belongs to the older traces, not to these routes.
Native AI behavior and physical-GBA performance are not addressed.
