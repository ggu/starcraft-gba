# Z7 canonical controller after range routing

At `d60e75eb`, `make test-sc64-z7-canonical-controller` ended `result=lost
host_ticks=19266 frame=18716`. This is a controller defect. The engine is not
shown to be at fault, and this change touches only
`tests/sc64_z7_campaign_run.c` and the Make policy line. `make -k test-zerg-z7`
now passes.

## What changed the outcome

The last wins and the first loss, each built with the same private inputs:

| Commit | Controller result |
| --- | --- |
| `6d5db14` | won, tick 179,764 |
| `c210775e` | won, tick 179,764 |
| `7181c093` | still playing at the 30,000-tick limit used for this check |
| `63665045` (merge of #134, range routing) | lost, tick 19,266 |

The first different input comes at tick 3,799. The old win was already thin:
at frame 15,660 it had one Drone left and was mining with almost nothing in the
bank.

## Causes, all in the controller

1. **A distant opening Hatchery starved the economy.** The controller placed it
   at (1128,1768), about 175 pixels from the home mineral line, and every Drone
   walked that far on each trip. Moving it to (1120,1680) has the largest effect
   of anything tried here.
2. **One parked enemy could stop mining for tens of thousands of frames.** A
   visible enemy within 180 pixels of a patch makes it unsafe. A single
   Hydralisk parked at (1035,1504) covered every home patch, so in one run 12
   Drones stayed idle from about frame 30,000 to frame 100,536. After 1,800
   frames with fewer than two miners and at least two idle Drones, only that
   enemy's weapon reach plus 16 pixels now counts.
3. **Mined-out bases had no next step.** Once home is exhausted and no field is
   in view, idle Drones now walk to the nearest explored field from frame 60,000
   on. This is the smallest of the three changes.

`--start-delay` adds neutral frames before the first order, which samples nearby
timing phases without changing the policy. The Make target uses 33, a phase that
wins. `--hatch-site` overrides the opening site for experiments.

## Range routing is not shown to be the cause

Each row runs 144 start delays (0 to 143) to the 200,000-tick limit, in scratch
builds without sanitizers. The unsanitized build reproduced the sanitized loss
tick. Many delays give the same outcome, so treat these as coarse counts.

| Controller | `SC64_RANGE_WORK` | Won | Lost | Reached limit | Best win tick | Distinct winning runs |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Before | on | 4 | 75 | 65 | 171,988 | 2 |
| Before | off | 5 | 44 | 95 | 178,973 | 4 |
| After | on | 20 | 16 | 108 | 141,913 | 17 |
| After | off | 19 | 30 | 95 | 145,013 | 13 |

Range routing does not change the win count much either way. Its effect on
losses changes direction between the two controllers, so this does not establish
that range routing helps or hurts. Its known first-hit latency for some reachable
targets is recorded in the [range routing audit](../2026-11-01-range-work/README.md)
and was not changed here.

## Limits

- **The mission is still chaotic for this controller.** About one delay in seven
  wins within the limit. The Make target pins one phase, so a later shared engine
  change can flip this result again. Most other runs reach the limit: many with a
  few buildings left on the northern plateau and no minerals to buy the Mutalisks
  that reach them, others defeated in all but name (next item).
- **Some "limit" runs are really lost.** They hold one or two buildings and no
  units, and the enemy does not finish them, so the mission never ends. The same
  happens with range routing off. This was not investigated further.
- The controller reads internal state to choose inputs, as before. It is not a
  pixel-only or general player.
- This is host evidence. The new tape has no ARM replay, fresh-process SRAM
  continuation or timing; the earlier ARM results belong to their own tapes and
  builds.
