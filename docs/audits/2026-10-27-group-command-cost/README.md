# Z7 group command cost

This audit separates an unconditional command-loop improvement from the
default-off formation destination experiment. The measured command path uses
synthetic in-bounds Hydras and ordinary R plus field-command input; it is not a
normal campaign run or a mission-completion result.

## Command loop improvement

`issue_order` now computes `group_order = selected_count() > 1` once before
iterating selected units. The selection is not mutated while the loop assigns
orders, so this preserves the old group predicate while avoiding a complete
selection scan for every selected actor. The change applies with
`SC64_REACHABLE_FORMATION` absent and does not add state or storage.

The full-render ARM probe measured the direct `issue_order` call separately
from the surrounding `game_update`. At 240 selected Hydras, the warm direct
call fell from about 2.06 million cycles to about 216 thousand cycles for both
the known disconnected-slot click and an open-area click. The direct call is
below the 559,240-cycle 30 Hz budget in this synthetic probe.

| 240 selected Hydras | Before hoist | After hoist |
| --- | ---: | ---: |
| Warm `issue_order`, disconnected-slot point | 2,062,281 | 215,748 |
| Warm `issue_order`, open-area point | 2,062,268 | 215,742 |
| Warm command-consuming `game_update`, disconnected-slot point | 108,660,510 | 106,813,805 |
| Warm command-consuming `game_update`, open-area point | 159,818,670 | 157,972,305 |

The full update still costs roughly 191 to 282 times the nominal frame budget.
The direct-call improvement does not make the full frame fit 30 Hz and does
not establish hardware timing. Both probe runs completed with `fault=0`,
`won=0`, and `lost=0`. They exercise command cost and no mission objective.

## Optional formation destination fallback

`SC64_REACHABLE_FORMATION` remains absent from canonical build flags. When
enabled for complete imported-race route fields, a multi-unit ground command
keeps each unit's existing formation slot if `path_next_tile` can reach it. If
the slot is unreachable but the clicked point is reachable, the unit uses the
clicked point. If both are unreachable, it keeps the formation slot. Single
unit orders use the clicked point, and flying units keep their formation slots
without a ground-route query. Loaded, immovable, and Z7 Egg exclusions remain
in place.

The fast decision reuses complete route fields as connected-component labels.
If no complete field covers the current walkable source component, it builds
one field from that source cell and checks the destination sets before another
allocation can evict it. Four stack-local hints retain recently used fields
during one `issue_order` call; no new persistent fields or save data are added.
Partial flood and visit-grid configurations use the existing `path_next_tile`
pair as the exact fallback.

Blocked endpoints follow existing path query behavior: path connectivity is
four-neighbor; a blocked start can exit through a walkable cardinal neighbor,
and a blocked destination seeds from walkable cells in its eight-neighbor
ring. The independent host graph fixture checks these endpoint rules, clamped
coordinates, randomized walk grids, and topology invalidation against both the
existing path query and an independent flood oracle. Its retained-hint stress
case runs shuffled queries over more disconnected components than the route
cache has slots, then changes topology while keeping stale stack hints and
compares each fallback decision to the oracle. The UI fixture checks ordinary
command input, a reachable common-point fallback, single-unit and flying
controls, an unreachable clicked point, and movement continuation.

The 12,000-tick ARM prefix comparison used the archived authored sources from
commit `5d34d6fca4735a6d93d69cdb8f3fdecc5d2b6cfd` as the reference and the
formation-enabled current source as the candidate. All 34 digest checkpoints
matched exactly; the run ended at frame 11,726 with `fault=0`, `won=0`, and
`lost=0`. The digest covers game and UI state, unit and building fields,
resources, fog, RNG, and Z7 runtime state. It excludes derived path, render,
and audio caches. This replay was captured before the later unconditional
`group_order` hoist; the hoist leaves the feature-enabled endpoint branch
unchanged. A subsequent post-hoist, flag-off ARM replay matched all 34
checkpoints of the immutable pre-hoist input tape, ending at frame 11,726
without a fault, win or loss. This is a bounded gameplay-state comparison,
not whole-world, pixel, or mission-completion parity.

## Evidence and limits

The post-hoist private probe used full-render Z7 flags with the formation
feature off. Its `issue_order` command measurement is distinct from the full
`game_update` measurement above. The input selected 2, 16, 48, 128, and 240
synthetic actors and used no world-memory edits during command dispatch.

Source and measurement provenance is retained privately under
`.local/build/z7-live-20261027/formation-latency/hoisted-off/` and
`.local/build/z7-live-20261027/component-digest/`. No ROM, emulator capture,
private harness source, or save artifact is included here. The feature remains
default off; neither the optimization nor the prefix replay qualifies a Z7
win, 30 fps, or physical hardware behavior.

## Canonical product links after the selection-count hoist

All three race products linked with the formation feature disabled and passed
the unchanged memory guards. Each ROM is 32 MiB. The change adds no actor or
save fields.

| Product | IWRAM end | BSS end | ROM load end |
| --- | --- | --- | --- |
| Terran T1–T10 | `0x03004e08` | `0x0203de94` | `0x09fc8114` |
| Zerg Z1–Z6 | `0x03004e00` | `0x0203de78` | `0x09e4f81c` |
| Protoss P1–P6 | `0x03004990` | `0x0203dc68` | `0x09e535a8` |

`./dev test` passed 554 Python tests and the asset-free host fixtures;
`./dev build` passed the original-art ROM verifier. The optional formation
fixtures passed under ASan/UBSan, including retained hints across cache eviction
and topology invalidation. Canonical link evidence is retained privately in
`.local/build/z7-live-20261027/command-final-links/`. These checks do not
promote either seventh mission or establish full-campaign timing.
