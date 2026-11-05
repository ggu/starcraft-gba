# Z9 carried campaign host qualification

The Z9 candidate `0e6d3ba4`, merged with canonical `1ffa7aad` and the
fixture-only changes in this branch, was tested with the complete Z9 feature
vector and generation 48. The promoted-profile fixture selects Z9 before the
Z7 fallback and checks nine normal-menu mission starts and save round trips,
plus 234 skirmish startup/save configurations. It passed.

Existing ordinary-input host controllers were rebuilt with the same Z9 flags.
All eight carried routes completed in wins: Z1 input tick 54,859; Z2 input tick
8,527; Z3 game frame 131,061; Z4 frame 47,601; Z5 frame 48,347; Z6 frame
118,289; Z7 frame 173,807; and Z8 frame 39,617. Z1 used the established
`race_z1_controls.c` controller; the generic early-race controller was also
tried and did not complete, and is not the established Z1 route. The Z1 route
was independently run under canonical Z7 flags and completed at the same
tick. These are host controller wins, not new ARM replays, arbitrary save
qualification, native AI equivalence, or performance measurements.

A private input snapshot matched all 1,958 hashes in the existing candidate
receipt before building. Make regenerated five noncompiled JSON metadata
files: construction and death manifests changed generated-header paths and
source provenance hashes; the order-marker manifest changed its recorded
palette-source hash; the tank-VFX manifest changed source provenance hashes;
and the weapon-VFX audit changed serialization without semantic JSON changes.
All compiler-consumed headers and binaries in that receipt remained identical.
The original inputs were not modified. Logs, trace hashes, the post-build
manifest and the failed generic-controller attempt remain in the ignored local
qualification directory. Native work used the shared lock and free-space
guard; one initial attempt stopped before compilation when the guard failed.
