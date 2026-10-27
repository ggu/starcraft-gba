# Z7 normal-input controller win audit

Date: 2026-10-27  
Status: host campaign win reproduced; qualification is **22/30** pending the
optimized ARM replay and fresh-process SRAM continuation.

Follow-up: the [full-render ARM replay](../2026-10-27-z7-full-win/README.md)
has now completed all 365 checkpoints through victory. That separate build
still fails the performance target; ARM save continuation and broader
profile qualification remain separate gates. The indexed host evidence
below is retained as its original reference.

## Result

The Z7 campaign controller won through ordinary input handling. The frozen
private controller and the public `tests/sc64_z7_campaign_run.c` candidate
produced byte-identical full traces: 20,690 events, ending at host tick 209,485
and game frame 205,764, with `enemy_buildings=0`. The result was reached before
the 300,000 host-tick bound.

The public candidate was compiled from the public source using the same
indexed `Z7_CONTROLLER_FLAGS` recipe as the private winner, plus
`-DSC64_REACHABLE_FORMATION`. The only source differences from the frozen
private winner were explanatory comments. Trace paths differ, but trace bytes
do not.

This winning recipe is **not** the full canonical-rendering profile. It does
not include unit facing, source weapon effects, source death art, the packed
canonical sprite bank, `SC64_Z7_SIGHT_SPANS`, or `SC64_ZP_DEFENSE_TARGETS`.
Those flags belong to the separate full optimized ARM qualification work.
This report makes no 30 fps or physical-hardware claim.

## Reproduce

The Makefile defines the indexed profile through `Z7_CONTROLLER_FLAGS` (built
from `Z7_CONTROLLER_REFERENCE_FLAGS`) and its host controller recipe adds
`-DSC64_REACHABLE_FORMATION`. The exact expanded flag vector used for the
public candidate is preserved at
`.local/build/z7-public-integrate-20261027/public-candidate.flags`; the exact
compile command is in
`.local/build/z7-public-integrate-20261027/compile-public-candidate.sh`.
The profile includes the indexed Z7 mission/runtime, egg production, source
sight, Scourge combat, nearest-depot worker policy, dense schedule,
prevalidated tick, and count snapshot. It is not the separate full-render flag
set.

With the existing private Z7 inputs prepared as described in
`docs/DEVELOPMENT.md`, reproduce the indexed host controller from the public
source and Makefile:

```sh
make test-sc64-z7-controller
```

The target runs with `--limit 300000` and writes
`.local/build/z7-controller/controller-trace.json`. The private expanded recipe
paths above are retained as measurement provenance; they are not required by
the public build interface.

The public candidate was also run with `--limit 600000` for direct full-trace
comparison. The documented `--limit 300000` command was run as well; it
terminated at the same campaign victory before reaching its bound.

## Controller behavior observed

- The opening keeps the home defense and worker economy operating while the
  ground force develops. Late ground orders use reachable formation slots and
  accept mixed owned mobile ground groups when at least one Hydra is selected;
  workers, Overlords, eggs, and noncombat units are excluded.
- A remembered visible guard can otherwise retain control after its target is
  no longer making progress. The controller retires that assignment after
  1,800 game frames with no observed HP change, then resumes reachable
  building attacks and reconnaissance.
- At the late ground-clear stage, the remaining structures were ground
  inaccessible. The controller uses normal paid production UI to morph a Lair,
  construct a Spire, and queue Mutalisks when ordinary actor-cap availability
  permits. No units, resources, orders, visibility, or campaign state were
  injected. The authenticated raw identity `132` is a **Lair** (not a
  Hatchery); the remaining structures also included raw `149` Extractor and
  raw `144` Spore Colony.
- The air force uses ordinary Attack-Move reconnaissance and visible-building
  target commands. When a Muta scout assignment became stale, the controller
  reissued the ordinary Attack-Move order. The final air stage cleared the
  remaining enemy structures and ended the mission.

All gameplay mutations in these runs came through the controller's ordinary
`game_update` input path. No source assets were generated or added.

## Frozen evidence

Private winner, preserved unchanged:

- Source: `.local/build/z7-live-20261027/controller/home16-north-localbox-front-reachable-stale-fix-globalr-mixed-stallretire-lateair-refresh.c`
- Binary: `.local/build/z7-live-20261027/controller/controller-localbox-front-reachable-stale-fix-globalr-mixed-stallretire-lateair-refresh`
- Compile recipe: `.local/build/z7-live-20261027/controller/compile-localbox-front-reachable-stale-fix-globalr-mixed-stallretire-lateair-refresh.sh`
- Trace: `.local/build/z7-live-20261027/controller/home16-north-localbox-front-reachable-stale-fix-globalr-mixed-stallretire-lateair-refresh-600k.json`
- Log: `.local/build/z7-live-20261027/controller/home16-north-localbox-front-reachable-stale-fix-globalr-mixed-stallretire-lateair-refresh-600k.log`

Public candidate:

- Source: `tests/sc64_z7_campaign_run.c`
- Binary: `.local/build/z7-public-integrate-20261027/sc64_z7_campaign_run-public-candidate`
- Compile recipe and expanded flags: `.local/build/z7-public-integrate-20261027/compile-public-candidate.sh` and `public-candidate.flags`
- 600,000-bound trace: `.local/build/z7-public-integrate-20261027/sc64_z7_campaign_run-public-candidate-600k.json`
- 300,000-bound replay outputs: `.local/build/z7-public-integrate-20261027/z7-public-300k.json` and `z7-public-300k.log`

SHA-256 values for the frozen winner and the completed public 600,000-bound
replay:

| Artifact | SHA-256 |
| --- | --- |
| Private source | `92cf3483d45a9a1d8d5b5b30a2cb240fba6a8b435e141e8052f044562865be0d` |
| Private binary | `bd6c70241632454bc8494978a347ad10fe038830e4e684c9fa7e243a66935055` |
| Private compile recipe | `7beca8aab2de6cd22876dcdec28a3071a784b814287db2a900ae19082812dc58` |
| Expanded public flags | `7a0dba9e8f437dbb3c6c8f6b26af05f5392e358088f4e0ecb9410ca0130f4e9e` |
| Public compile recipe | `2a8df4daf9942bd538fc3622fba4c14d7066a178c82f3a3a8ee9b821a50c76a4` |
| Private 600k trace | `252891e51ce82c44442acd098f58c63ad7616c79bd895c22c1bebc26e279ec97` |
| Public 600k trace | `252891e51ce82c44442acd098f58c63ad7616c79bd895c22c1bebc26e279ec97` |
| Public 300k trace | `252891e51ce82c44442acd098f58c63ad7616c79bd895c22c1bebc26e279ec97` |

The private and public 600k traces are byte-identical. Compared with the earlier stalled policy, the winning controller retained
the input/checkpoint prefix through host tick 172,000; the new late-air policy
then diverged after activation. The public and private winning traces match
through the entire victory.

## Qualification boundary

This proves the normal-input host controller can win this campaign and that
the public test source reproduces the frozen private trace. The campaign
remains at **22/30** until the exact winning policy is independently replayed
with the optimized ARM profile and a fresh process continues through SRAM
save/load. No ARM replay or fresh-process SRAM result is asserted here.
