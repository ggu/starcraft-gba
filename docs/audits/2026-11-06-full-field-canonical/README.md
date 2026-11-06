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

Normal Z7–Z10 link guards, fresh guest continuation and guest cycle measurements
for this exact canonical source are still being checked. No 30-fps or physical
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
that every game event selects the intended sound. Current-bank stream and
header verification is pending.

The current format-2 bank subsequently passed the existing
`sc64_audio_pack.py --verify-bank` check: it decoded all 514 populated SFX and
music streams from the header-defined offsets and lengths, checked stream
boundaries, alignment, padding and guard, and reproduced the manifest PCM
SHA-256 `9979d9b99ca3888eec879904fcc42c2e0742b702634c622136cdb608b185eb10`
for 15,649,120 decoded bytes. No bank problems were reported. This verifies
internal header/blob/manifest consistency; it does not verify the game's choice
of clips for events or establish equivalence to the older advisor bank.
