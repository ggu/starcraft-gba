# Indexed target query experiment

The trial source at `6ff5cd0d` uses `CANONICAL_TARGET_QUERY=1` to
compare the indexed target query with the scalar policy. The option
defaults to zero. On the indexed
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

## Dense Z7 ordinary replay

All three packed Z10 builds write an ordinary GBA pause journal at frame 5,641
and load it from fresh HOME. One ordinary A press reselects the
Hatchery. The subsequent replay matches all 228 named world samples
through tape call 155,650, nine paired indexed-page and palette cuts,
and six physical route-cache cuts. The complete 32 KiB pause files,
restored state and all timed route counters agree.

| Timed tape calls | Mean cycles, control → full query | Mean cycles, coordinates only | p95 cycles, control → full query → coordinates only |
| --- | ---: | ---: | ---: |
| 47,758–48,237 | 2,192,613 → 2,217,666 (+1.14%) | 2,192,722 | 3,182,357 → 3,208,684 → 3,181,907 |
| 153,812–154,741 | 2,492,556 → 2,425,483 (−2.69%) | 2,491,830 | 3,764,634 → 3,716,725 → 3,768,476 |
| 154,742–155,650 | 2,418,359 → 2,340,526 (−3.22%) | 2,419,385 | 3,667,801 → 3,588,167 → 3,671,741 |

The full option saves 3.33% and 3.84% on late zero-build updates, almost
entirely in orders. The early zero-build mean rises 1.67%. Coordinates
alone stay within 0.05% of stock on all six means. The full option's Z10
IWRAM endpoint is `0x03004e50`, 240 bytes beyond the control. EWRAM is
unchanged. The three-way comparison SHA-256 is
`8fa1b2ac6eb10f111ba4b277c6e36d5ca65aa91becef4de64de2186883b1d013`.

## Z8 lazy weapon checks

The lazy trial computes an attacker class only when a candidate reaches
the corresponding live weapon predicate. It matches the same 203 timed
Z8 updates, 17 world and display cuts, and three modal boundaries. The
stock mean is 1,694,983 cycles. The lazy mean is 1,698,098 (+0.18%) and
its p95 is 3,234,002. It retains the packed control's RAM endpoints and
uses a 116-byte indexed query stack frame, up from 92. Dense Z7 remains
unmeasured for the lazy variant. The Z8 receipt SHA-256 is
`b616d13448aa7037e50550e4e40045f911d3084574e5d752cecee6bfaa264efb`.

The scalar policy remains the production default for all query-local
variants.

## Immutable combat facts

The scalar-policy trial uses one generated byte per unit type for
immutable target and attacker facts. It retains mutable visibility,
cloak, containment, construction, flight and invincibility checks in
their existing paths. The new normal Z10 bank preserves the stock fact
tables and adds the combat table and its compile-time guard. The
sanitizer fixture checks 2,739 cases. The `race_can_attack` ARM body
shrinks from 1,372 to 1,264 bytes and the new table occupies 50 ROM bytes.
The IWRAM and EWRAM endpoints both match the `9398b13c` control.
[Opt-in source](https://github.com/ggu/starcraft-gba/commit/a1d49513)

The paired normal Z10 Z7 replays match all 228 named world checks, nine
display cuts, six route-field cuts and the full frame-5,641 pause SRAM.
Both runs load through fresh HOME and use ordinary controls, including
one A press to reselect the Hatchery. The Z8 pair matches 17 world and
display cuts, three modal boundaries and all 203 timed frame and route
counters. The timings use advancing gameplay updates.

| Replay window | Mean cycles, stock → combat facts | p95 cycles, stock → combat facts |
| --- | ---: | ---: |
| Z7 early, 477 updates | 2,193,528 → 2,193,360 (−0.008%) | 3,182,692 → 3,182,885 |
| Z7 first late, 926 updates | 2,492,987 → 2,424,115 (−2.76%) | 3,766,802 → 3,712,728 |
| Z7 second late, 905 updates | 2,419,168 → 2,343,482 (−3.13%) | 3,666,292 → 3,595,765 |
| Z8, 203 timed updates | 1,696,432 → 1,694,349 (−0.12%) | 3,236,715 → 3,235,970 |

Zero-build means in the two late Z7 windows drop 3.38% and 3.73%.
The Z7 tape stops at call 155,650. Normal Custom Game and P10 trials
remain open, including P10's V2 fact layout and the live campaign
observer wrappers. Terran and Showcase use separate production vectors
without the necessary actor fact tables.

| Receipt | SHA-256 |
| --- | --- |
| Opt-in Z10 ARM ROM | `18de5ef93b64cee76e89e624cf63763fa90a6eb38feb844c6b309dbcfd0f095f` |
| Pinned ARM link | `8c4badda68205aa8731b0e22e18a678fdd2209afcd86f72c7c6164e7737a3767` |
| Z7 comparison | `ab235cc96acd887faa239cde15f5d23ecbd886ca7bf7940ceaa7924874f449a2` |
| Z8 comparison | `89d3cbef99f22fbc3255d5347b500bfb5c316b1aaed8ccf5aa0b872c90746ae8` |
