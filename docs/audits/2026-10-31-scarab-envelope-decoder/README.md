# Selected impact envelope decoder

The existing race load path now separates SRAM selection from decoding the selected raw impact envelope. Selection still prefers the current tag then falls back to the pre-BC tag exactly as before. The decoder operates only on `race_impact_record` scratch and retains the existing BC marker, compact Yamato-tail, impact bounds and event checks; version checks outside the decoder remain in their original location. No Scarab tag or migration is registered by this change.

Prepared-input ASan/UBSan save fixtures for Terran, Zerg, Protoss P7 and Showcase passed with byte-identical output to the retained `059dda5` preflight baseline. Separate-process legacy-BC pending-impact and two-active-Yamato write/read tests passed; each produced byte-identical SRAM and continuation output to that baseline. Commands and local logs are under `.local/decoder-proof`. The first Zerg compile failed because the isolated worktree lacked the already-prepared BC-body path; after linking the existing inputs, the serial retry passed. No asset generators ran.

Clean-source qualification at `25a890e` passed boundary checking, doctor,
default build and default tests (793 Python cases with 23 optional skips,
eight ancillary cases and C fixtures). Four prepared-input gated ARM links
also pass without asset generation or changes to the 104 input hashes.
BSS/IWRAM endpoints match the preflight baseline: Terran
`020370b8/03004db8`, Zerg `02037124/03004e38`, P7
`020382e4/03004c80`, Showcase `0203df38/03004fc8`. These are gated Scarab
links, not canonical admission. The first three recipes run their normal ROM
verifier; Showcase's recipe has no verifier command. A separate read-only
Showcase check verifies its title/code/header checksum/SRAM marker, 32-MiB
size, ELF payload outside the fixed boot header, and all-FF padding.

Child `6f08720` adds an unregistered semantic-selection transaction helper.
It rebuilds the selected winner's scratch-only plan after the selector reloads
the winning raw record, then calls an infallible commit callback. An independent
ASan/UBSan rerun of `make test-scarab-save-selection` passes, covering a newer
checksummed but semantically invalid record, destructive candidate scratch use,
and failure during winner re-preflight without changing live state, the
selection token or SRAM. This is a generic callback fixture, not actual
gameplay-envelope migration or a complete-world atomicity proof. The broad
default and gated ARM results above are for `25a890e`, not this child.

This remains a refactor prerequisite, not a namespace, bridge registration,
historical conversion, gameplay cross-tag semantic selection, full-world
atomicity or performance qualification. Reaver admission remains gated.
