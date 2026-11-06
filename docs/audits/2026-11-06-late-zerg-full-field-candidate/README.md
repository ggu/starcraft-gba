# Late-Zerg full-field candidate

This isolated candidate is based on `9668df74`, including the unified custom
starting-bank correction. Its only production change adds the existing
`CANONICAL_ROUTE_INDEX_FLAGS` and `CANONICAL_ROUTE_NEW_CELL_FLAGS` to
`ZERG_Z7_CANONICAL_FLAGS`. Z8 inherits that vector, and Z9 and Z10 inherit it
in turn. At normal defaults this selects word clearing, the visited grid and
the fresh-cell writer; it does not add the separate monotone shortcut. Setting
`CANONICAL_ROUTE_INDEX=0 CANONICAL_ROUTE_NEW_CELL=0` supplies a same-source
control. The existing validation rejects incompatible toggle combinations.

The normal Terran, Protoss, early-Zerg and Showcase vectors already carry the
same full-field capability; this candidate only fills the late-Zerg gap. The
late-Zerg vectors already use packed race fields and split flood, without the
partial-flood option. Existing route-header compile guards enforce the visited
grid and fresh-cell prerequisites. The full-field path still allocates the
complete route field and preserves the existing slot policy; the borrowed
walkability bits are restored from the flood queue. A different monotone
shortcut previously failed a fresh Z7 replay because its omitted allocation
changed a ranged cache lookup, and is intentionally not enabled here.

Prior qualification on source `271db94a` used a different, independently
verified codec2 audio bank: the complete Z7 HOST tape, both-map one/three-AI
custom opening samples, fresh guest continuation, and all eight Z7–Z10 guarded
links passed; one earlier busy window showed about a 6% diagnostic guest-cycle
reduction. Those results do not qualify this candidate source, the delivered
audio bank, every campaign, hardware, or the near-30-fps objective.

Before promotion, run paired same-source controls and candidate builds with
all guard limits intact and the exact delivered advisor header/blob: 73754a61…
and bd0adfd0…, respectively. Verify all frozen input hashes before and after.
At minimum, check Z7–Z10 normal ARM links and reserves; run full Z7 ordinary
input from fresh state and fresh guest save/load continuation with strict state,
input and display comparisons; exercise actual Z8/Z9 identity and representative
custom 1–3-AI scenes and both pages; run an adequately advancing busy guest
window with explicit state/workload/palette scope; and run the repository's
source boundary, default build/tests and applicable prepared-profile checks.
Retain any failures. Compare the latest main source again before integration;
release only after the requested broader state, visual, memory and performance
evidence is satisfied. No tests or builds have run on this candidate yet.

The current Makefile defines but does not include the `split-visit-newcell`
route-policy fixture or the `bounded-newcell`, `full-newcell`, and
`active-newcell` depot fixtures in its aggregate test lists. Qualification must
build and run those four sanitized fixtures explicitly, along with the existing
word-clear checks; an ordinary default test run alone does not cover them.
The vector check should also inspect Z7–Z10 for both toggle settings and assert
that `SC64_ROUTE_MONOTONE_PROBE` remains absent.
