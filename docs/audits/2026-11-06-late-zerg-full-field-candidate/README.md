# Late-Zerg full-field candidate

This isolated candidate is based on `e3919ec0`, including the unified custom starting-bank,
worker, audio and source-shaped power changes and the controller-only cleanup. Its only production change adds the existing
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
evidence is satisfied.

The base Makefile defined but omitted the `split-visit-newcell` route-policy
fixture and `bounded-newcell`, `full-newcell`, and `active-newcell` depot
fixtures from aggregate tests. This candidate adds all four asset-free fixtures
to those lists. Qualification must explicitly run those four sanitized fixtures
and the existing word-clear checks before promotion.
The vector check should also inspect Z7–Z10 for both toggle settings and assert
that `SC64_ROUTE_MONOTONE_PROBE` remains absent.

The existing route-policy fixture definitions are moved verbatim into a
literal included Makefile to preserve the unchanged top-level source-size
limit. The source-only vector test checks Z7–Z10 on, partial and off settings
and keeps the separate monotone shortcut absent. The focused combined-source checks below cover these changes.


## Combined-source checkpoint (2026-11-06)

On candidate `54243fbb`, based on main `e3919ec0`, the source boundary,
late-vector test, all four listed sanitized new-cell fixtures, and word-clear
checks passed. All eight normal Z7–Z10 ARM control/candidate links passed their
existing guards using the delivered advisor header and blob. The 564 private
input hashes and relevant source hashes matched before and after each run.

A normal-menu HOST matrix covered two maps, all three human races, and one,
two, or three AI opponents. Each of the 18 openings had 41 matching sampled
selected-state, indexed framebuffer and palette fingerprints between control
and candidate. These are opening/warm simulation samples, not complete games
or hardware measurements.

The original fresh Z7 recorded tape did not win on this combined source:
both control and candidate lost at call 19,505, and the strict original
validator rejected each. The emitted replay logs and 19,504 per-call selected
state/display fingerprints were byte-identical, as were final save journals.
The fingerprint trace SHA-256 was
`77816805ac7597448035e9e6bbf64b5f13096011bb4caae8ebe099c93e5469b9`; the
replay-log SHA-256 was
`d99971b872ede04b484fb78d963502489a74ada842f959766c404d33f5d3f54b`.
The full-tape comparison correctly reports truncation; matching nonwin parity
does not satisfy the fresh campaign-win or fresh guest-continuation gates.
No promotion is claimed.


## Reachable custom-game guest window

On the same e391 combined source and delivered advisor bank, ordinary physical
button input started map 1 with Zerg human and Terran, Zerg and Protoss AI.
At frame 4,001 the human remained passive and the world had 46 units and 17
buildings. START/B saved a 32 KiB journal; a fresh process loaded it with L and
reproduced the uninterrupted 500-update continuation, including selected
persisted-world fingerprint and exact advancement. The control and candidate
journals matched (SHA-256
`7ffbfb8f5ae36fb73ab57f3f5b8fd5039bb6df43e82e3b93d08c7edeca3ac0c4`).
By frame 4,501 there were 48 units and 21 buildings.

A separate same-seed paired guest-cycle probe advanced all 500 updates in play;
screen and mode matched each update, and six sampled selected-state/workload
fingerprints matched. The control averaged 627,566.596 emulated cycles per
update and the candidate 611,691.662, a 2.53% reduction in this window; peaks
were 3,984,361 and 3,102,721 respectively. These are mGBA cycle observations
on this specific passive-human custom opening, not a hardware FPS result. The
physical backend limits reads to RAM and ROM, so this guest probe did not
compare the indexed framebuffer or palette. An initial attempt to read video
registers was rejected before timing and retained as a diagnostic failure; the
separate HOST custom matrix above supplies sampled display equality only.
The Z7 winning and wider gameplay/pixel/performance gates remain open.
