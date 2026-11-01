# Prepared source-sight row contexts

This candidate is based on experimental integration `8e4a333`.
`SC64_Z7_VISIBILITY_ROW_CONTEXT` is absent from normal product flags. It
prepares each old/new center's truncated tile coordinates, clipping extent,
radius and halfwidth-table pointer once per supported source-sight transition.
The row loop uses those temporary values instead of deriving them again for
every old and new row. The phase-footprint rejection still runs first.

The contexts live on the stack and do not add persistent state or BSS.
The bounded result below does not establish a full-mission speedup. The existing
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
and `/tmp/sc64-z7-row-context-z8-followup-20261101/result.json`. No full-mission
physical replay, full clean default suite or activation is claimed for this
candidate.

## Bounded ordinary-input timing

Three fresh physical-button boots compared the current baseline, a separate
contact candidate, and this row-context candidate. Across updates 50,001 to
50,900, all 135 retained named trace checks matched. At nine checkpoints the
three builds also matched every decoded Unit scalar and Building scalar/queue
field, complete visibility/exploration/source-count/minimap arrays, and the
active framebuffer and palette. This is checkpoint evidence, not continuous
whole-world equivalence.

The current baseline used ROM `8c61048b`; row context used `856059b0`.
The 900 consecutive shipped cumulative-cycle deltas gave:

| Emulated main-loop metric | Baseline | Row context | Change |
| --- | ---: | ---: | ---: |
| Mean cycles | 2,030,413.882 | 1,968,664.239 | -3.0412% |
| p95 cycles | 4,155,222 | 3,961,962 | -4.6510% |
| Window maximum cycles | 8,926,169 | 8,984,906 | +0.6580% |

The window maximum is the largest of the 900 measured per-update deltas,
not the peak-since-boot counter. Timing includes presentation/UI and does not
establish physical-device FPS or near-30-fps performance. The worse window
maximum is retained as a qualification concern. A full ordinary save/fresh
continuation and normal-profile activation checks remain necessary.

The local receipt is `/tmp/sc64-contact-row-threeway-20261101/result.json`,
SHA-256 `448e4bc0b590f643e3e16238548694a194b601bf8050b7c4750bf8c0e69ab837`.
Root independently recomputed the three means, p95s, maxima and bucket counts
from all 900 per-variant samples. The contact candidate was slightly slower
and is not adopted.
