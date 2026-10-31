# Military AI on the current shared engine

Runtime `3228f21719e43f74d6d2459e0ee9840ef74d82f9` retains public
military history `aabb730`, the current shared engine through `d44891d`,
and two defense corrections. It does not activate experimental Scarab,
Defiler or shared-projectile gates. Terran, Zerg, Protoss and Showcase keep
both custom maps, all three human races and one to three mixed-race opponents.
Canonical campaign coverage remains T10/Z6/P7, or 23/30; further missions and
near-30-fps performance remain active work.

## Corrections and negative evidence

The public military candidate counted a hostile actor once for every owned
depot within threat range. One Marine near two depots could therefore trigger
miner pulls and army recall as if there were two attackers. A closer armed
worker could also replace the chosen non-worker threat. The corrected census
excludes workers before both decisions, counts each eligible actor once and
retains the nearest-depot distance and strict 320-pixel boundary.

The actual-engine negative fixture failed on the old census with an expected
count of one and an observed count of two. Four scenarios cover overlapping
depots with one/two Marines, a closer armed SCV, and the exact range boundary;
they also inspect actual miner orders. All 32 combinations pass on the final
source (four products, two maps, four scenarios) under ASan/UBSan. The old
failure, compile commands and later results remain in ignored local receipts.

The census correction exposed an ordinary developed-base placement failure:
the first four Zerg defense rings could all be occupied even though legal
creep sites remained on the existing fifth/sixth rings. An observer of the
unmodified match found eligible outer-ring sites; no resources or actors were
injected. Zerg defense placement now searches all six existing radii while
retaining the single-ring pass and existing expensive-candidate budget.
Collision, creep, lane and path predicates are unchanged. The earlier combined
matrix's eight missing natural-morph save windows and 264/272 result are
preserved; after this correction all 272 cases pass, including those eight
ordinary morph/save continuations. Synthetic forced-morph checks are separate
and are not substituted for those ordinary cases.

## Current qualification

All checks ran locally; no Actions workflow was enabled or dispatched.

- Clean unprepared `python3 tools/check_source_tree.py`, `./dev doctor`,
  `./dev build` and `./dev test` pass on `3228f2`. Python discovery runs 853
  cases with 23 optional skips; the additional 5/8 groups and remaining native
  fixtures pass. This is a separate clean worktree without prepared inputs.
- With 104 already-prepared inputs authenticated and frozen, the actual
  four-vector AI progression/recovery/save matrix passes 272/272 on this
  source. It includes both maps, all human races, one to three mixed bots,
  mirrors, small-army forced commits, builder/worker recovery and Sunken saves.
- All four normal Make ARM links pass the unchanged guards. Independent
  checks verify title/code/checksum/SRAM marker, ELF loadable payload outside
  the cartridge header, and FF padding. The four ROM byte streams equal
  those at preceding runtime `99cf6c4`; the later shared merge adds disabled
  controls. ELF file hashes differ because their build/debug provenance differs.
- Four actual normal-ROM fresh-process SRAM continuations pass on map 1,
  with three mixed bots, an ordinary pause-menu save after 9,000 updates and
  900 continued updates. Saved/restored and uninterrupted/resumed bounded
  semantic fingerprints include active queues; all checks pass and the input
  SRAM remains unchanged. This is not arbitrary-world or hardware coverage.
- All 104 prepared input hashes match afterwards. No capacities, RAM guards,
  save fields or simulation cadence were weakened.

| Product | BSS end | IWRAM end |
| --- | --- | --- |
| Terran | `020370c0` | `03004b40` |
| Zerg | `02037134` | `03004c10` |
| Protoss | `020382ec` | `030049e0` |
| Showcase | `0203b0e0` | `03004a60` |

Receipts are ignored under `.local/qualification/` in the
`ai-military-shared-final-20261031` worktree: `arm-results.json`,
`rom-verification.json`, `threat-positive.json`, `full-matrix/report.json`,
`fresh-sram/summary.json`, and the executed commands and individual logs.
The clean default worktree retains its separate `qualification-default`
logs. Failed disk-full attempts from preceding candidates are preserved and
are not counted as successful checks.

## Performance scope

The unchanged normal Showcase binaries were compared from the same retained
saved world for each cohort, with the same port-free mGBA profiling backend.
Each window contains 900 active advancing updates, frames 9,002 through 9,901.
AI2 uses shared `0a314c4`'s qualified Showcase bytes; AI3 uses `3228f2`.
The policies intentionally diverge after loading. These are actual workload
comparisons, not an isolated planner-cost or state-equivalence experiment.

| Bots | Mean full cycles, AI2 → AI3 | p99 full cycles | Maximum full cycles |
| --- | --- | --- | --- |
| 1 | 836,909 → 722,345 | 1,382,900 → 842,804 | 1,685,099 → 1,351,408 |
| 2 | 1,149,537 → 862,411 | 3,370,037 → 1,123,574 | 5,008,258 → 1,453,981 |
| 3 | 761,046 → 745,753 | 1,404,021 → 1,172,653 | 3,141,922 → 3,141,932 |

The three-bot maximum is effectively unchanged, rather than improved. These
full-loop counters include presentation waits. In these bounded windows the
new means still exceed the roughly 562k-cycle two-video-frame budget: they do
not establish near 30 fps. Older full-terminal 15.6–20.5-fps measurements remain
historical evidence for their earlier runtime, not current full-match timings.
Current full-match, campaign-heavy and physical-GBA performance remain to be
qualified and improved; no smaller actor cap is used to hide their cost.

Per-window distributions, totals, starting/final populations, periodic state
and pixel samples, ROM/ELF/SRAM/runner hashes are retained in
`guest-compare/summary.json` and its per-run receipts. The profiling runner's
SHA-256 is `506fb4aecaf74b820f77c43da4ac2251868c074b8a3526e98891e21aea2b06ae`.
The separate fresh-SRAM backend is
`1f38e8da8ff4aafa4c9056e38f8fbe267592a22d7978d8cfb6fb70604cedfe4c`.

This remains authored opponent policy with full hostile-actor knowledge,
free-for-all hostility and no resource/production cheats. It is not a decoded
N64 melee AI. Ordinary producer-exit search, nearest-depot cargo return,
per-owner upgrades/fog and remaining unit services are separate documented work.
