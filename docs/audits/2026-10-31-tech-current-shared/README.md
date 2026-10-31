# AI tech stage on current shared source

Runtime merge `45427e01093d3e3a9edd356e1d6825f868fab1a8` combines
author stage 2 `c5795fdd8ee1af9c1a69d0b87cc1c9d108900bda` and shared
`7a4cc09cde710398de73fcef8663bcba9fdbe0ed`, preserving both histories.
Later commits `02fb1e2` and `d34fa70` change only the ARM verification tool and
its tests. Private inputs, binaries, SRAM and diagnostic output remain ignored.

## Behavior and source review

The authored custom-game planner develops per-race tech and mixed armies.
It uses the same owner-generic paid addon and morph service as the human.
The two previous source findings are fixed: a Creep Colony retains its source
type, maximum and current HP during its Sunken morph, applying the target type
and HP difference on completion; addon parent feasibility is checked before
reserving its price. Independent source review found no concrete blocker to
this scoped shared integration. This is not decoded native AI behavior.

## Local qualification

- Clean unprepared default build and tests passed at runtime `45427e0`:
  829 Python tests with 23 optional skips, additional 5- and 8-test suites and
  the native checks. The verifier-only follow-ups pass their focused 15 tests.
- All four normal 32 MiB ARM products passed the unchanged reserve guards:

| Product | BSS end | IWRAM end | ROM SHA-256 |
| --- | --- | --- | --- |
| Terran | `020370c0` | `03004b70` | `c7f2ce997b2101f49420b6104f56d80cb4a767e15bd14a79afacdb92795bef38` |
| Zerg | `02037134` | `03004c40` | `1e4b7ece8e0842816d10bdc39710ca878562e4086c0ef01a75613cc9b2d7d484` |
| Protoss | `020382ec` | `03004a18` | `882d2847f0df63cf8dad9ada986a2d7d8dd999a993003adfff36820c2ba6877a` |
| Showcase | `0203df40` | `03004e08` | `9ca8f4c3b7edaff99e2672b8017eddbf4b0ff0cdcb5d1de0336ed9f314c9af85` |

- The development matrix reproduces **241/256**, not a complete pass. Its
  24 damaged-morph/HP/save cases, 8 addon-feasibility cases, 12 walking-builder,
  16 worker, 80 recovery and 12 late-save cases pass. Six passive-human matches
  end before opponents reach the requested development thresholds; one match
  ends before its save window; eight end before an opponent Sunken morph.
  These remain the disclosed stage-2 military-policy limits. Stage 3 is separate.
- An additional map-0 Terran-profile/Zerg-human/ttt save at frame 4,000 passes
  1,800 host continuation updates through the actual codec. This adds earlier
  coverage without relabeling the unavailable late save as a pass.
- All four normal ARM products pass fresh-process SRAM continuation against
  three mixed opponents on map 1: save through the pause menu after 9,000 play
  updates; compare 900 resumed updates. Saved/restored fingerprints match;
  uninterrupted and fresh continuations match at frame 9,901; loading leaves
  SRAM unchanged. Active queue contents and selected persisted actor fields
  are compared. This is not a complete save-state or framebuffer comparison.
- The final tool rejects an actual corrupted Terran SRAM record as expected.
  That negative control exits nonzero and is not counted as a continuation pass.
- All 104 prepared input records retain their frozen SHA-256 after these runs.
  No source assets were regenerated. Whitespace/source-boundary checks pass.

## Verification failure retained and corrected

The first Terran run failed strict parity: uninterrupted frame 9,900 versus
fresh frame 9,901. An observation-only rerun found a repeated stop at the same
audio-update boundary, frame 9,881 and scheduler tick 9,919, with no update
progress. The old driver counted breakpoint notifications as updates.

The driver now requires exactly one scheduler-tick advance per measured step,
retries a stationary boundary at most eight times without reinstalling input,
and rejects skipped/backwards ticks or persistent non-advancement. Neutral
title-screen scheduler bootstrap is synchronized and counted separately.
Unit tests exercise these failures and input non-repetition. Independent review
found no concrete blocker to this bounded protocol. Exact frame/fingerprint
parity remains mandatory; no mismatching frames are accepted or normalized.
The failed result and diagnostic trace are retained separately. Final four
product runs pass with two startup stops per process and no later stationary
stops; an earlier counter-only run also exercised a later stationary stop.

## Scope

Ready for stage-2 shared integration, not the release of #167 or military
stage #182. No near-30-fps hardware or complete campaign claim follows from
these checks. Canonical coverage remains T10/Z6/P7 (23/30). No Actions or SD
writes were performed.
