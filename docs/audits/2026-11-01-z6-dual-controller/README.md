# Z6 controller under normal and carried profiles

At controller source commit `187138599b2e9ddaedadad96f06ee8413d6ca43a`,
the same source produced native host wins under both the normal Z6 production
flag vector and the Z7 canonical carried vector. The policy is profile-aware:
normal Z6 targets currently visible Force 2 actors, scouts the explored
frontier when none are visible, and allows a nearby visible structure to
justify Kerrigan's solo finish. Under `SC64_Z7_RUNTIME`, it retains the prior
carried controller's live-actor target selection and solo finish at three or
fewer remaining actors. This is a test-controller policy split, not an engine,
mission, scripted AI, damage, or actor-limit change. The carried policy can
inspect live actors outside current vision; all resulting game actions use the
ordinary controller inputs. The normal policy also reads world and map state.
The isolated synthetic policy fixture changes its own test state to check the
visibility gate; that fixture is not a controller replay.

The normal build used `ZERG_ROM_FLAGS` in the local qualification recipe. The
carried build used `test-sc64-z7-canonical-carried-z6`. Both were compiled and
run under ASan/UBSan, including the Z6 policy fixture, and both recorded traces
passed `tools/sc64_zp_controller_replay.py --profile zerg-z6 --validate-only`.
The source objective was won with Kerrigan alive, zero Force 2 actors in both
required locations, and runtime fault zero in each run.

| Profile | Host ticks | Native frame | Inputs | Trace SHA-256 |
| --- | ---: | ---: | ---: | --- |
| Normal Z6 | 121970 | 120753 | 7315 | `69a23bafe84e165231d5312671fa68197e3ef82eac552f1ec526954629e907aa` |
| Z7 carried Z6 | 119539 | 118289 | 6699 | `4c739a68730aa69d54ff930887044bbb8d0c48be1baea32672941676306b1cae` |

The normal tape is byte-identical to the retained normal win from source hash
`9a5089c07ee5027d946a0d647bafc55e6a08e82a89b298476c59e9a093fba405`.
The carried tape is byte-identical to the prior carried win from `0a9beef`.
The new source and its fixture remained unchanged across both runs. All 109
pinned private input hashes, the separately pinned walk-word header and the
isolated contact mask matched before and after the runs; generated outputs
were isolated in this worktree. The full log and status are retained locally at
`/tmp/sc64-z6-dual-policy-qualify-20261031.log` and
`/tmp/sc64-z6-dual-policy-qualify-result-20261031.json`. The normal trace is
`/tmp/sc64-z6-dual-policy-normal-20261031.json`; the carried trace is
`.local/build/z7-canonical-carried-z6/controller.json` in this worktree.

These are bounded host controller results and offline trace validation, not
physical-button replay, hardware timing, a full-state equivalence proof, or a
general claim about the controller on other maps or profiles. No mission is
promoted by this audit.
