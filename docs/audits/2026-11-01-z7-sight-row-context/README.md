# Prepared source-sight row contexts

This candidate is based on experimental integration `8e4a333`.
`SC64_Z7_VISIBILITY_ROW_CONTEXT` is absent from normal product flags. It
prepares each old/new center's truncated tile coordinates, clipping extent,
radius and halfwidth-table pointer once per supported source-sight transition.
The row loop uses those temporary values instead of deriving them again for
every old and new row. The phase-footprint rejection still runs first.

The contexts live on the stack and do not add persistent state or BSS.
No speedup is claimed. The existing
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

## Initial qualification

At frozen source `c30b5e6`, the five-variant ASan/UBSan full-grid test passed,
including the prepared-row squared-distance checks. Normal OFF Z7 and Z8 ROMs
matched the merged integration hashes `8c61048b` and `5696e42d` respectively.
The ON ROMs were `856059b0` (Z7) and `b896fbdf` (Z8). BSS/IWRAM ends were
unchanged in both pairs: `0203d718`/`03004f80` for Z7 and
`0203dbd8`/`03004bf0` for Z8, within the existing guards.

GCC's static stack report puts the transition at 96 bytes OFF and 128 bytes
ON; the old row helper uses 16 bytes and the prepared row helper 28 bytes.
The nested transition/row-helper frames therefore increase from 112 to 156
bytes. This is not a measurement of the complete dynamic stack high-water
mark. The contexts are not zero-RAM work, despite requiring no new BSS.

The first Z8 dry run stopped because this new worktree lacked its private
sprite header. A separate Z8-only follow-up copied the already-prepared,
hash-verified Nydus-complete Z8 bank and program, without regeneration, and
passed both normal ARM builds. Earlier Z7 and oracle results were retained.
Local receipts are `/tmp/sc64-z7-row-context-preflight-20261101/result.json`
and `/tmp/sc64-z7-row-context-z8-followup-20261101/result.json`. No physical
replay, timing benefit, full clean default suite or activation is claimed for
this candidate.
