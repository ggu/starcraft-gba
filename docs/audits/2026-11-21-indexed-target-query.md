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
skirmish. Normal opt-in ARM timing and memory remain to be measured.

For the reusable sanitizer differential with prepared inputs, run
`make CANONICAL_TARGET_QUERY=1 test-target-row-query`.
