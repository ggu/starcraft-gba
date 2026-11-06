# Full-field routing: canonical qualification

Base runtime: `31a5ddf8`, including the merged command-menu alignment. This
source-only branch adds guarded replay instrumentation and tests; it does not
change production runtime files or enable any route flags. The candidate adds
`SC64_ROUTE_WORD_CLEAR`, `SC64_ROUTE_VISIT_GRID`, and `SC64_ROUTE_NEW_CELL`
only to explicit test compile vectors. The separate monotone shortcut remains
disabled: its skipped field allocations caused a genuine fresh-game divergence.

With hash-verified private inputs, ordinary retained Z7 input started from
fresh HOST state reached the recorded win for both control and candidate. Both
passed all 307 recorded observations; 175,455 per-call selected actor/world and
displayed indexed-frame/full-palette fingerprints matched, as did final journal
and replay records. Fingerprints do not establish complete-state identity.

Four HOST custom games launched through normal menus with a Zerg human, both
maps and one or three mixed AI opponents. All advanced 4,000 calls and matched
41 selected actor/world/route-field and indexed-frame/palette fingerprints per
game. The human remained idle; these opening/warm games are not completed-match
or busy-battle proof. Frozen input and source manifests matched before/after.
Private logs remain under `build/z7-host-sram-contract/host-full-flood-traces/`
and `custom-full-flood-v2/`; no private inputs or captures are committed.

Fresh guest continuation and bounded guest cycle measurements for this exact
canonical source are recorded below. No 30-fps or physical
hardware claim follows from the HOST results.

All eight normal release-vector control/candidate links for Z7–Z10 passed the
existing guards on the canonical runtime. BSS ends were unchanged. IWRAM ends
were `03004f88`→`03004eb0` for Z7, `03004bf8`→`03004b28` for Z8,
`03004bf8`→`03004b20` for Z9 and `03004bf8`→`03004b28` for Z10. Missing
Make prerequisites in the isolated copy were filled only from hash-matched
retained receipts; earlier failed preflights remain in the local logs. All 563
files in the expanded copied-input receipt rehashed unchanged after linking.

The links used the current format-2 audio header/blob pair with SHA-256
`11bb06f8006f8e6e9bdad8bd730fd3022fa2afc0c341b917ca5a25f8c0f04a21` /
`3243919d7292e3db3dc27b8f47dbc948e521b3c49bb7378f2b451c99b518b163`.
An independent stack-qualification receipt pins that same pair. It is different
from the older advisor-bank header/blob in the Nov 5 Z9 prepared-input receipt;
these results do not establish that the two sound banks are interchangeable or
that every game event selects the intended sound. Current-bank stream and header verification is recorded below.

The current format-2 bank subsequently passed the existing
`sc64_audio_pack.py --verify-bank` check: it decoded all 514 populated SFX and
music streams from the header-defined offsets and lengths, checked stream
boundaries, alignment, padding and guard, and reproduced the manifest PCM
SHA-256 `9979d9b99ca3888eec879904fcc42c2e0742b702634c622136cdb608b185eb10`
for 15,649,120 decoded bytes. No bank problems were reported. This verifies
internal header/blob/manifest consistency; it does not verify the game's choice
of clips for events or establish equivalence to the older advisor bank.

The earlier delivered Protoss 10 ROM is a different audio configuration. Its
ROM and ELF were read-only checked against the November 6 card-copy receipt:
SHA-256 `206847219fc46ea92526e950e5b1f5623acf6a66360fb232eb08a9b8d956e1d6`
and `852c6bb2966be97696d1bee6ab7621e035b2526b3cc4d49628ce78165cb49336`.
The ROM contains the entire advisor-bank blob with SHA-256
`bd0adfd0ce83c0bb054d40c8f6bc69d91a55ca0f846f7ada56d48d98d7f72427`
at byte offset 458,092, matching its ELF blob symbol. That ELF also contains
an advisor lookup table. Thus the absence of advisor entries from the separate
codec2 test bank does not explain missing advisor sounds in the delivered ROM.
Correct cue triggering, clip selection and audible hardware playback remain
separate questions.
The codec2 bank's 1,368 unit/kind first-and-count entries and 1,163 voice
references also passed bounds checks against its 501 sample streams; 13 of 16
music slots are populated. This check does not establish semantic clip mapping.

A fresh guest process then loaded the genuine ordinary-button Z7 journal via
HOME-L and followed the original input from cut 136,058 to the recorded win.
Control and full-field ROMs both passed strict resume validation and matched all
50 HOST/guest cargo checkpoints; their final guest SRAM SHA-256 values were both
`de1d039ed52756d98114ea31a644693ed73e394ab212200f8f8b41b8674cb5ba`.
The custom replay ROM hashes were
`5810967c7619d541c85f911477ac0c5abfff93bf6efbae6371110f0dc0556373` and
`1eabdb89c2cfaced44c3f953ef5ebd11e32de6fd470df64ce4ddbfcddd377613`.
Source and input manifests matched before and after. This is a sampled emulator
continuation, not full-state or physical-hardware proof; canonical busy timing
remains pending.

The delivered advisor bank was also checked against the verified ROM and ELF.
All eight header-defined tables (sample and music offsets/lengths, unit voice
index tables and advisor indices) matched ROM bytes at their ELF symbol
addresses. The bank verifier decoded all 523 streams with no errors and matched
15,860,008 decoded PCM bytes to SHA-256
`95cd32bed0f45677d176e9afb32a1e8e9dfad0301e45314208f52559e490eb57`.
This establishes header/blob consistency, not whether live events trigger the
right sound or whether physical playback is audible.

The bounded guest busy pair was repeated on the exact `31a5ddf8` runtime with
only test-harness observation, without production attribution hooks. Both
variants passed seed/tape checks and all sampled workload, selected-state,
indexed-frame and palette comparisons. Of 1,000 calls, 975 advanced gameplay
and 25 did not; no text modal was observed. Mean advancing guest cycles changed
from 2,512,181 to 2,364,928 (5.86% lower), and advancing peaks from 10,106,704
to 8,650,188. The diagnostic ROM hashes were
`642fbc113c54419ab4677d01560a2f2b6bfada86d25de467a3f4b95d651d001f` and
`fa62c957305975ea94c7e8e14039d93218a844057873ebcfd044fb39f7121d88`.
This sample remains far above the nominal 30-fps budget; it is not a physical
hardware or general workload result. The subsequent `271db94a` queue change
requires its own qualification before any current-source promotion.
