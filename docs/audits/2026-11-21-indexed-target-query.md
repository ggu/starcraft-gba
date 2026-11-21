# Indexed target query experiment

Set `CANONICAL_TARGET_QUERY=1` to compare the indexed target query with
the scalar policy. The option defaults to zero. On the indexed
occupancy path, the candidate's position passes directly to the live
visibility check. The scan order, squared distance, ties and existing
fog and source-owner rules retain the same inputs and evaluation order.
The scalar path keeps its own coordinate reads.

The query also computes ground and air weapon eligibility once for a
valid non-Scourge attacker where the existing direct policy already
permits that step. Target-side cloak, detector, invincibility,
transport, lifted-building and hangar checks remain live. Scourge,
wildcard and invalid attackers use scalar weapon predicates. T6, P6
and P7 builds retain their source-specific attack wrappers. Non-indexed
queries use the established scalar path.

Four targeted HOST binaries use the prepared flags and data for normal
Terran, Z10, P10 and Showcase. ASan/UBSan passes 500 real gameplay
updates per profile, 16,000 live target queries, 96,000 synthetic
queries, and 2,000 invalid-index fallback checks. A second comparison
uses the indexed helper and the scalar policy directly for 5,040
eligibility cases. It varies the attacker, viewer, target class and
live fog, invincibility, transport and hangar state.

The final flag-off normal Z10 rebuild reproduces the Carrier control
ROM byte for byte. The default option and its existing build key keep
their prior values.

## Timing and next qualification

Use the exact current normal Z10 ROM and same prepared banks for the
paired run. Start with the [retained Z8 fresh-HOME windows](2026-11-16-resumable-frontier-budget.md#z8-check-on-the-current-mineral-routing-release).
Then replay the [dense Z7 zero-build case](2026-11-21-z7-late-order-budget.md#harness-priorities)
at calls 153,992, 154,202, 154,247, 154,598 and 155,333. Compare sampled
world and camera state, palette, both indexed pages, route counters and
the complete SRAM. Attribute any cycle change to the live eligibility
and indexed query scopes before widening to late P10 and mixed
skirmish. The bounded Z8 ARM comparison follows.

For the reusable sanitizer differential with prepared inputs, run
`make CANONICAL_TARGET_QUERY=1 test-target-row-query`.

## Z8 coordinate-only comparison

The full option and a coordinate-only alternative use the exact packed
Z10 release asset vector from `60dd3d94`. The latter adds
`-DSC64_TARGET_QUERY_COORDINATES_ONLY` to the trial compiler vector.
That isolates position reuse and keeps the existing scalar ground and
air checks under the same direct, cloak and source-owner policy.

All three normal ROMs load ordinary Z8 SRAM at HOME and reach controller
call 33,900. They match 203 timed game frames and logical route counters,
17 world and display cuts, and all three modal boundaries.

| Z8 timed calls | Mean cycles | Mean orders cycles | p95 cycles | Peak cycles |
| --- | ---: | ---: | ---: | ---: |
| Packed release control | 1,694,983 | 771,102 | 3,228,956 | 12,056,654 |
| Full indexed query | 1,707,002 | 783,158 | 3,248,894 | 12,077,038 |
| Coordinate-only query | 1,694,875 | 770,904 | 3,229,552 | 12,062,664 |

The coordinate-only variant brings the mean back within 0.01% of
the control and saves 8 IWRAM bytes. EWRAM remains at `0x02039b30`.
The full trial grows the indexed query from 1,200 to 1,412 bytes
and calls the damage helper before spatial pruning. The coordinate-only
variant removes that eager work and uses 1,176 bytes for the indexed
query. GCC still expands the context pointer into five scalar arguments
for the internal policy call, with one passed on the stack. Keep the
variants opt-in while comparing denser target workloads.

The coordinate-only sanitizer run passes the same 4,000 live queries,
24,000 synthetic queries, 500 fallback checks and 1,272 direct
eligibility cases on the normal packed Z10 compiler flags.

| Local input or receipt | SHA-256 |
| --- | --- |
| Packed Z10 control ROM | `ea3faa53ff31553e91e3888c58746161d9ac9681bffd6a45885df4e14ea32308` |
| Full indexed query ROM | `ca77ef9fbf028accf2133581dd8b42e38d609a9569a3d6cd8309f730bc4a2d9c` |
| Coordinate-only ROM | `d6e77227cba70c536208bbd12a434787d4552162926e2d877c5363c096717489` |
| Three-way Z8 comparison | `704745cbeb2d639e637049242962e32ea06551679b8235735a1c19d513ed7fad` |
