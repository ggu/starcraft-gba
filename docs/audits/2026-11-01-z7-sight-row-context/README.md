# Prepared source-sight row contexts

The candidate was initially based on experimental integration `8e4a333`.
The normal Z7 profile now selects `SC64_Z7_VISIBILITY_ROW_CONTEXT`, inherited
by Z8. It
prepares each old/new center's truncated tile coordinates, clipping extent,
radius and halfwidth-table pointer once per supported source-sight transition.
The row loop uses those temporary values instead of deriving them again for
every old and new row. The phase-footprint rejection still runs first.

The contexts live on the stack and do not add persistent state or BSS.
The initial bounded result alone did not establish a full-mission speedup. The existing
source-sight activation and unsupported-radius fallback are unchanged; this
is a shared visibility implementation candidate, not a mission rule change.

The candidate-stage qualification plan required the independent bounded-row squared-distance oracle
(including negative centers and map edges) and the existing full-grid
source-count, minimap, lifecycle and fallback oracle with the candidate flag.
Then check normal Z7 and inherited Z8 ARM links against their existing guards,
OFF ROM identity, and a current source-matched ordinary-input ON/OFF window.
The bounded window was not a full mission or hardware frame-rate result.
Later qualification is recorded below.

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
and `/tmp/sc64-z7-row-context-z8-followup-20261101/result.json`. At that initial
stage, full physical replay, clean default suite and activation were pending.

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
maximum is retained as a qualification concern. At that stage, a full ordinary save/fresh continuation and normal-profile
activation checks were still pending.

The local receipt is `/tmp/sc64-contact-row-threeway-20261101/result.json`,
SHA-256 `448e4bc0b590f643e3e16238548694a194b601bf8050b7c4750bf8c0e69ab837`.
Root independently recomputed the three means, p95s, maxima and bucket counts
from all 900 per-variant samples. The contact candidate was slightly slower
and is not adopted.

## Full fresh physical continuation

The gated Z7 ROM `856059b0f5443d0c673380cd8744e70f914089514009cf95223696c57be1e373`
exported its own ordinary physical-button save at cut 12 and resumed in a fresh
process. The prefix observation and all 302 retained continuation observations
matched the current phase-enabled baseline. The continuation reached victory
across 179,753 updates. Mean emulated main-loop cycles including presentation
and UI fell from 2,158,730.324 to 2,099,758.494 (2.731783%); peak since boot
fell from 53,713,860 to 53,689,552 (0.045255%). The earlier 900-update window
maximum increased 0.658%, a separate measure from the peak since boot. These
figures are not physical-device FPS. The ON continuation result SHA-256 is
`05b47ee39f68673df979450f3c6e9351798b7ff10902f00100519b17750987e3`.

The subsequent normal-profile activation qualification is recorded below.


## Normal activation qualification

Activation source `e8fd130` passed the independent phase oracle, Z7 full-grid
sight oracle, and gated Z8 source-sight host fixture. Its normal Z7 ROM was
byte-identical to the full physical candidate
`856059b0f5443d0c673380cd8744e70f914089514009cf95223696c57be1e373`;
the inherited normal Z8 ROM matched the qualified gated Z8 ROM
`b896fbdf6871f6252127d0461c9ec678589a5ffd362c24eeab5ac985001d49ab`.
Their BSS/IWRAM ends were `0203d718`/`03004f80` and
`0203dbd8`/`03004bf0` respectively, within the unchanged guards. ELF debug
file hashes differ between builds; the ROM bytes are identical.

The clean unprepared activation worktree passed source boundary, doctor,
default build and full default test; Python discovery ran 932 tests with
26 skips. The 44-byte increase in the nested static transition/helper frames
is a compiler estimate, not a complete stack high-water measurement.
