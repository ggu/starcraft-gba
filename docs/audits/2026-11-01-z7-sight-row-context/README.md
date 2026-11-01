# Prepared source-sight row contexts

This candidate is based on experimental integration `8e4a333`.
`SC64_Z7_VISIBILITY_ROW_CONTEXT` is absent from normal product flags. It
prepares each old/new center's truncated tile coordinates, clipping extent,
radius and halfwidth-table pointer once per supported source-sight transition.
The row loop uses those temporary values instead of deriving them again for
every old and new row. The phase-footprint rejection still runs first.

The contexts live on the stack and do not add persistent state or BSS.
Their actual ARM stack-frame cost, normal OFF identity, memory guards and
execution cost have not been measured. No speedup is claimed. The existing
source-sight activation and unsupported-radius fallback are unchanged; this
is a shared visibility implementation candidate, not a mission rule change.

Qualification must run the independent bounded-row squared-distance oracle
(including negative centers and map edges) and the existing full-grid
source-count, minimap, lifecycle and fallback oracle with the candidate flag.
Then check normal Z7 and inherited Z8 ARM links against their existing guards,
OFF ROM identity, and a current source-matched ordinary-input ON/OFF window.
Only uninstrumented timing with matching retained state and render evidence
can justify further admission. A bounded window alone is not a full mission
or hardware frame-rate result.
