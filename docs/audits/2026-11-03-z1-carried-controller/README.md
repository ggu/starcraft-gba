# Carried Z1 controller under the Z7 vector

At source `63665045`, `build/test-sc64-z7-carried-z1` reported
`controller attack did not complete Z1 (tick=188587, M=250 G=0, D=1 H=0,
buildings=37, won=0 lost=0 phase=1)`. The same controller under the canonical
Z6 vector had won. This is a controller defect that a shared engine change
exposed. It is neither an input artifact nor a demonstrated range-routing
regression.

## Input check

Inputs were regenerated in this worktree's private `.local` with
`./dev sc64 prepare --profile zerg` and `prepare-bc`, from the verified ROM and
a clone of `references/sc64-maps` at its pinned revision. The failure matched
the reported line exactly, so the earlier rsynced copy was not the cause.

## What the failing line meant

The run actually won Z1 at tick 187,076 (`screen=3 won=1`). A scripted press
then dismissed the result screen, and tick 187,078 was already mission 2. The
reported `M=250 G=0 D=1, buildings=37` are mission 2 starting values, and `won`
had been reset. The win was slow because the army was wiped between ticks
33,000 and 34,300 and could not be rebuilt.

## Causes, all in `tests/race_z1_controls.c`

1. **Idle Drones could not be sent back to minerals.** A worker whose patch runs
   out goes idle; `tests/sc64_skirmish_ai_progress.c` documents that the planner
   reassigns its own workers and never the human's. Those Drones park beside the
   next patch, where A selects them instead of mining. The controller tried only
   the nearest patch, so it repeated the same failed command. On this run six
   Drones stayed idle from about tick 26,000 with 3,434 minerals left in five
   home patches. Mining stopped, so any army loss was unrecoverable.
2. **Attack-Move commands became single-target orders.** With
   `SC64_DIRECT_ATTACK`, which the Z7 vector has and the Z6 ROM vector does not,
   A within a visible hostile's pick radius issues `ORDER_ATTACK_TARGET`. On this
   run, A landed on a distant Goliath, so the group walked across the map past
   units that were shooting it. The Z5 and Z6 controllers already aim at open
   ground for this reason.
3. **A win could be dismissed.** Scripted presses continued after the result
   screen appeared.

## Why range routing is not the cause

Removing `SC64_RANGE_WORK` (enabled by `4072d735`) made this one run win at tick
58,145, which looked like a regression. To test that, the controller was rerun
with 0 to 143 idle frames added at mission start, in scratch builds without
sanitizers that reproduced the reported line. Those 144 offsets produced 45
distinct outcomes, so treat the counts as coarse.

| Controller | `SC64_RANGE_WORK` | Wins | Median win tick | Wins under 70k |
| --- | --- | ---: | ---: | ---: |
| Before | on | 90 | 97,684 | 44 |
| Before | off | 88 | 69,497 | 45 |
| After | on | 143 | 51,108 | 143 |
| After | off | 143 | 52,652 | 143 |

Before the fix, the same controller failed about as often with or without range
routing. Wins were counted at the first `won`, so dismissed wins are included.
The flag moved this deterministic run off a trajectory that happened to win.
This does not qualify range-routing latency, which the
[range-work audit](../2026-11-01-range-work/README.md) covers.

Removing each change from the fixed controller gave:

| Variant | Wins | Wins under 70k | Slowest win |
| --- | ---: | ---: | ---: |
| All changes | 143 | 143 | 64,259 |
| Without off-target aiming | 143 | 133 | 78,789 |
| Without attack-phase miner recovery | 144 | 139 | 98,214 |

Trying farther patches accounts for most of the gain; the other two mainly trim
slow wins. One offset (20) still fails in both builds: its last Hydralisk dies
by tick 39,174, and the target picker, which ranks buildings by distance to the
army, then ends the attack loop. Waiting to rebuild instead only delayed that
failure, so that change was not kept. Off-target aiming only compiles under
`SC64_DIRECT_ATTACK`, so the other two vectors keep their existing cursor
behavior.

## Results after the change

These are bounded native host results, not a physical-key replay or hardware
measurement. The engine and generated inputs are unchanged, and no ROM or
mission is promoted.

- Z7 vector: Z1 site checks passed; Z1 won at tick 54,859 with 33 Hydralisks. Z3
  and Z4 won.
- Canonical Z6 vector: `make test-sc64-zerg-carried-missions` passed, with Z1
  winning at tick 54,905 and 24 Hydralisks.
- Races vector: Z1 won at tick 56,398.
- `python3 tools/check_source_tree.py`, `./dev build` and `./dev test` passed.

## Not fixed here

`make test-sc64-z7-carried-missions` and `make test-races-controller` still stop
at Z2, with `Z2 carrier selection failed` from `tests/race_campaign_run.c`. That
file is not changed here. The Z2 failure is separate from this one and still
needs its own diagnosis.

`make -k test-zerg-z7` therefore still fails. Its only two failing prerequisites
are `test-sc64-z7-carried-missions`, on Z2, and
`test-sc64-z7-canonical-controller`, the full Z7 mission controller run.
