# Reachable firing positions

A visible ranged target can occupy a disconnected walk-grid cell while normal
firing positions beside it remain reachable. The old combat branch repeatedly
planned toward the target center, failed that route and waited, even when the
unit could walk into its existing weapon range. This was observed during Z7:
a Hydralisk at (1140,276) could not reach a visible Hydralisk at (1111,192).

## Behavior and bounds

`SC64_ATTACK_RANGE_ROUTES` adds a fallback only after an ordinary ground combat
center-route plan fails with retry 12. It does not intercept explicit MOVE,
flying movement, melee contact, local collision retries, or fogged last-seen
pursuit. The fallback seeds walkable tile centers inside the existing inclusive
minimum/maximum weapon band and chooses a reachable next waypoint. Actual
movement, collision, visibility, attack eligibility, damage, cooldown and range
checks remain in their existing engine paths. This does not add firing through
a new visibility or geometry rule, or increase weapon reach.

The authored route field reserves the last existing packed field rather than
allocating another map-sized buffer. Its exact target x/y and min/max keys cost
eight BSS bytes. Ordinary point-route capacity drops by one slot (14 to 13 in
the current products). No Unit, Tracer, save row, save generation, actor limit
or stack reserve changes. Topology changes and loads invalidate the derived
fields through the existing route reset.

A first implementation flooded the entire map. A private ARM probe found that
unnecessarily expensive, so it was replaced before release. The range-only ROM
flood now stops once the querying origin has a known gradient. Same-key queries
reuse that discovered prefix. An origin outside the prefix rebuilds until it
is reached or the search is exhausted; an exhausted field caches failure.
Range fields use validity 3 (partial) or 4 (exhausted), separate from point
fields. Formation connected-component hints require validity 1, so a region
that seeds several disconnected components cannot falsely join them.

This draft proposes the option for the four canonical products through
`CANONICAL_ATTACK_RANGE_FLAGS`; setting that variable empty restores ordinary
point-field capacity and center-only combat routing. Original-art, historical
race products and unrelated probes retain their prior flags.

## Reference boundary

The pinned [OpenBW bwgame.h](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h)
was inspected for `unit_target_in_weapon_movement_range`, `weapon_max_range`
and `order_StayInRange` (around lines 1702, 1737 and 2790). Those distinguish
weapon reach from the movement destination. No reference implementation code
was copied. This tile-center BFS is an authored GBA navigation policy, not a
claim to reproduce the native N64 pathfinder, collision-box geometry, halt
distance or full attack-order lifecycle.

## Verification scope

`make test-sc64-attack-range-routes` runs asset-free ASan/UBSan fixtures for the
isolated open target, exact target subtiles and both range endpoints, seed
arrival within the weapon band, blocked starts, unreachable components,
partial-prefix misses, cached exhausted failures, topology resets, point-field
eviction and component-hint isolation. It compares 120 deterministic obstacle
layouts with an independent BFS. Variants cover plain and split point floods,
visit-grid/frequency policies, and combined partial/visit/frequency policies.
The visit-grid cases assert the input grid is unchanged.

`tests/sc64_attack_range_game.c` is an optional actual-game synthetic fixture.
With the feature off, the ranged attacker stalls; with it on, it approaches
without teleporting and applies the same stock damage inside the existing
64-pixel range. Melee, explicit MOVE and direct-order fog pursuit retain their
old behavior. It passed ON and OFF in Terran, Z6 and P7 prepared profiles.
These synthetic actor states are regression fixtures, not campaign wins.

A separate private actual-Z7-terrain fixture reproduces the observed coordinates
on the prepared source map. The baseline dealt no damage in 300 ticks; the
candidate reached a firing position by tick 72 and dealt 7 damage from 63.8
pixels away. All private inputs, harnesses, ROMs, SRAM, traces and captures stay
ignored. Public checks contain authored sources and synthetic values only.

## Current qualification and merge gate

This is a draft. The correct near-target approach is verified, but cold distant
queries still cause material spikes. Do not merge the proposed canonical
activation until those queries are amortized or otherwise bounded within an
acceptable frame-time budget. Main remains unchanged by this draft.

At runtime commit `5995b45`, the four full canonical ARM products link under
the unchanged guards. The prepared vectors have every current Make profile
macro (plus the expected imported-audio switches); no imported Make recipes or
asset generators were run. Single-TU include traces verify current authored
sources and the explicitly reused private generated inputs.

| Product | IWRAM end | BSS end |
| --- | --- | --- |
| Terran | `0x03004df0` | `0x02037084` |
| Zerg Z6 | `0x03004ee8` | `0x020370f0` |
| Protoss P7 | `0x03004b30` | `0x020382b0` |
| Showcase | `0x03004fa8` | `0x0203df30` |

Showcase retains 88 bytes before the code guard and 208 bytes before the BSS
guard; neither reserve is reduced. The no-asset route fixtures and actual-game
ON/OFF fixtures pass. The 72 ordinary menu/skirmish/save/reload cases pass
across both maps, all human races, 1–3 AIs and focused three-AI compositions.
They explicitly warm a reserved range field before loading and assert that the
load invalidates it, then compare continued semantic state.

Separate actual ARM SRAM runs pass in Terran, Z6 and P7: normal-menu start,
economy through frame 9001, save, in-place restore/continuation, process
termination, fresh isolated-process load of the actual SRAM file, and 900 more
ticks matching the current host reference. Their cache markers inspect the
existing VFX tracer-duration cache, not a warm range field. Warm range-field
invalidation is the explicit host check above. All owned emulators terminated
and were waited on.

A neutral Blood Bath replay against three mixed AIs matches its host reference
at all 14 state windows, plus starts and terminal verdicts. Overall ARM cycle
equivalents remain approximately 28.73/28.66/28.72 fps in Terran/Z6/P7. The late
P7 window remains about 24.66 fps. These fixed-camera unattended runs terminate
in defeat and do not cover a completed campaign, human play, steady 30 fps or
real hardware; they also do not expose every cold range-query case.

A separate synthetic 128x128 ARM probe exercises the fallback directly:

| Probe | Showcase | Z6 |
| --- | ---: | ---: |
| Near Hydra first fallback tick | 351,469 cycles | 348,028 cycles |
| 480px distant origin, max range 210 | 1,619,485 cycles | 1,619,485 cycles |
| Cached query, 1,000-query mean | 921 cycles | 923 cycles |

The near case drops from the rejected full-flood implementation's 4,713,504 /
1,071,756 cycles, and both profiles fire correctly. However, the distant query
still takes about 96.5 ms. Sixteen cold target-subtile changes average about
101 ms (maximum about 104 ms); a near-seed to distant-origin prefix rebuild
costs similarly. The distant Z6 case is slower than its old hot full-flood
implementation. These measurements block activation despite the ordinary
replay's unchanged average rate. Probe-only links discard unused code, so the
full-product table, not probe memory ends, establishes production bounds.

Private evidence is under `.local/build/ranged-approach/partial/`,
`partial-arm-probe/` and `partial-fresh-sram/`; the rejected full-field results
are retained separately. No assets, generated headers, ROMs, saves, captures,
private harnesses or native reference implementation are tracked. All checks
run locally, with no GitHub Actions or SD writes. No mission is newly enabled
or qualified by this work.


## Direct-corridor update at `03d96e2`

The current candidate first checks up to eight nearby firing tiles with the
same clear-corridor predicate used by ordinary movement. The scan is bounded
to a 34x34 tile box (range at most 256 GBA pixels); larger bands retain the
existing general path. A clear waypoint is stored in the reserved slot's
existing metadata with validity 5 and the exact target/range key. Every new
origin rechecks the corridor; a blocked cached tile triggers a new candidate
scan, then the existing BFS if needed. No additional BSS or saved state is
added. Formation and point fields ignore marker 5, including stale gradient
bytes left beneath it. A separate direct-only change is being prepared for
main so these useful clear paths can ship without the synchronous BFS cost or
reserving an ordinary route field.

The updated hybrid ARM probe reduces the clear 480px-origin/max210 query from
1,619,485 cycles to about 82,505 cycles (4.9 ms). Sixteen target-subtile rescans
average about 81,539 cycles; cached corridors average about 2,480 cycles.
The near Hydra's first fallback tick is about 107,968 cycles. Both Showcase
and Z6 apply the expected damage and pass the movement/range assertions.

This does not solve obstructed searches. A long wall with a distant gap makes
the direct shortlist fail, and the correct BFS fallback takes 5,397,201 cycles
in Showcase and 2,690,747 in Z6 (about 322/160 ms). The candidate remains a
draft because that work must be incremental. A two-plane frontier bitset can
fit in one additional existing route field; scheduling, target-motion policy,
fairness, and explicit pending status need implementation and qualification.

Full-product ends for this update are:

| Product | IWRAM end | BSS end |
| --- | --- | --- |
| Terran | `0x03004df0` | `0x02037084` |
| Zerg Z6 | `0x03004f40` | `0x020370f0` |
| Protoss P7 | `0x03004b70` | `0x020382b0` |
| Showcase | `0x03004fd0` | `0x0203df30` |

The helper itself stays in ROM. GCC now emits the existing `direct_path_clear`
HOT function out of line and shrinks its inline copy in `move_toward`, causing
the small IWRAM delta. Showcase retains 48 bytes before the unchanged code
guard. GCC reports static frames of 192 bytes for the candidate finder, 48
for its combat wrapper and 40 for the existing corridor routine; this is not
a whole-program peak-stack claim, and no stack reserve is reduced.

Current-source ON/OFF game fixtures pass in Terran/Z6/P7, including exact band
edges, cache-key changes, alternate-origin rescans, general BFS fallback and
marker-5 isolation. A separate formation-enabled host build passes. The
72 menu/save continuations pass with 18 explicitly warmed direct-point caches
and 54 warmed region caches invalidated at load. Fresh actual ARM SRAM
continuation passes for all three products. The bounded long ARM replay still
matches all prior state checkpoints, with average rates around 28.7 fps and
the same late-P7 limit. Full default build/tests pass (649 Python checks,
20 optional skips, plus C/ancillary checks); source-boundary and whitespace
checks pass. These do not qualify another mission or all obstructed frame
budgets.

Evidence for the update stays under `.local/build/ranged-approach/direct/`,
`direct-arm-probe/`, `direct-obstructed-probe/`, `direct-stack/` and
`direct-fresh-sram/`. A benchmark script initially reused a historical output
folder; the overwritten ELF/map artifacts are explicitly marked as superseded.
An isolated `git 5995b45` source reconstruction under
`reproduced-partial-arm-probe/` reproduced both historical ROM hashes and all
cycle records exactly. Historical logs and that reproduction, rather than the
overwritten files, support the earlier comparison.
