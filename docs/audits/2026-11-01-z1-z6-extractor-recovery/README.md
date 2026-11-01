# Z1 Extractor recovery and canonical Z6 carried suite

At source `79757a7d1e39e1e2f6a83c0f96c2fd9bf1d6f7b6`, the canonical
six-entry Zerg carried target stopped on Z1's final assertion. The controller
won at tick 53735 with 25 Hydralisks and no enemy buildings, but its
Extractor was absent at the result screen. The assertion requires the
Extractor, Spawning Pool and Hydralisk Den to survive and was not relaxed.
The failed target log is `/tmp/sc64-z6-six-qualify-20261101.log`; its retained
trace is in `/tmp/sc64-z6-six-compat-20261101/.local/sc64/` under
`z6-carried-z1-controller.json` before the subsequent successful run. The
original failed trace is also retained at
`/tmp/sc64-z1-z6-recovery-normal-20261101.json`.

A read-only replay of the original recorded inputs matched all 40 retained
1,000-tick checkpoints on resources, worker and Hydralisk counts, and the
three tech buildings. It observed 69 Extractor damage updates, from tick 11576
to destruction at tick 38654; enemy raw-3 Goliath cooldown resets coincided
with those damage updates. This is temporal evidence, not a damage-callback
proof. The Hatchery remained alive and 270 minerals remained at tick 39000.
The controller could spend up to 9000 ticks rebuilding its army in
`regroup_hydras`, during which it did not call its existing Extractor recovery
path before spending the remaining minerals on Hydralisks.

Commits `ca422a3` and `3453f3f` call that existing recovery path when the
Extractor is absent, both between combat orders and during regrouping before
production spending. It selects a worker, chooses the build command, moves
the cursor and places a foundation through ordinary controller inputs. The
controller can inspect live game and map state; it does not write world,
resource or UI state. When the Extractor stays alive, the new branches do not
issue inputs.

At frozen source `3453f3f747e228d464d5421566f4a5b03204d1fc`, the unmodified
`make test-sc64-zerg-carried-missions` target passed under the canonical Z6
production vector and ASan/UBSan. It includes the Z1 cancel and timeout site
checks and all six controller runs. Z1 won at tick 53795 with 24 Hydralisks,
and its existing final Extractor, Pool and Den assertions passed. The target
reused Z2–Z6 binaries built in the preceding clean `79757a7` run; their source
and engine dependencies were unchanged by these Z1-only commits. Z2–Z6 also
passed individually in that preceding run, which preserved the original Z1
failure. The result and log are retained at
`/tmp/sc64-z1-z6-recovery-v2-result-20261101.json` and
`/tmp/sc64-z1-recovery-v2-normal-full-20261101.log`.

The Z7 carried Z1 vector separately passed cancel and timeout checks and won;
its new trace is byte-for-byte identical to the prior successful trace, SHA-256
`54c70ea4509d3907071817375d85a5c71898079e735eb1d0163f1931ea3e46c1`.
The canonical Z6 Z1 trace SHA-256 is
`f5e0675e813f24f496fdee52276bf700322002e120629daf71c5ce50126daf7d`.
The frozen source stayed clean and all 109 pinned private input hashes, the
walk-word header and isolated contact mask matched before and after the runs.
The inputs and generated outputs used by this qualification were copied into
the isolated worktree; no original input was rewritten.

These are bounded native host controller results. They are not a physical-key
replay, a hardware performance measurement, or qualification of other profiles
or complete state equivalence. No ROM or mission is promoted by this audit.
