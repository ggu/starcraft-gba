# Selected impact envelope decoder

The existing race load path now separates SRAM selection from decoding the selected raw impact envelope. Selection still prefers the current tag then falls back to the pre-BC tag exactly as before. The decoder operates only on `race_impact_record` scratch and retains the existing BC marker, compact Yamato-tail, impact bounds and event checks; version checks outside the decoder remain in their original location. No Scarab tag or migration is registered by this change.

Prepared-input ASan/UBSan save fixtures for Terran, Zerg, Protoss P7 and Showcase passed with byte-identical output to the retained `059dda5` preflight baseline. Separate-process legacy-BC pending-impact and two-active-Yamato write/read tests passed; each produced byte-identical SRAM and continuation output to that baseline. Commands and local logs are under `.local/decoder-proof`. The first Zerg compile failed because the isolated worktree lacked the already-prepared BC-body path; after linking the existing inputs, the serial retry passed. No asset generators ran.

This is an active-path refactor prerequisite, not a namespace, bridge registration, historical conversion, cross-tag semantic selection, full-world atomicity, ARM or performance qualification. Reaver admission remains gated.
