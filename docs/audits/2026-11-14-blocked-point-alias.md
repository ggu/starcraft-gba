# Reuse point fields for equal blocked-goal seeds

The optional `SC64_ROUTE_BLOCKED_ALIAS` probe runs on 128×128 maps. A
blocked point goal seeds its reverse flood with each walkable cell in its
clipped 3×3 neighborhood, in row order. If a fresh complete point field
has the same absolute seeds, the two floods visit the same cells in the
same order and write identical directions. The probe copies the field
into the normal victim and installs the requested goal with the stock
cache metadata and counters. It does not change blocked-start lookup,
because the caller still passes the original requested goal to that code.

The seed bounding box proves that every requested seed is inside the
candidate's neighborhood. Checking the candidate neighborhood outside
the request proves that there are no extra seeds. Empty seed sets, stale
fields, depot keys and the currently replaced slot are excluded. The
probe adds no storage and falls through to the regular scalar or wide
FIFO kernel when it cannot prove equality.

`make test-sc64-route-blocked-alias` runs an asset-free ASan/UBSan check.
The fixture compares every packed cell against an independent reverse
FIFO for two distinct blocked goals, verifies their separate logical
insertions, and repeats with an extra donor seed that forces a real fill.
The probe remains off pending a density estimate from a physical trace,
guarded links and paired Z7 timing and route-state checks.
