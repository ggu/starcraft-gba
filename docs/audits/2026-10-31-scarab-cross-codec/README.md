# Experimental Scarab hidden-child save across codecs

This private, default-OFF stage extends the authenticated Scarab journal
namespace to the Zerg/Protoss-family compact row codecs, including the P7
typed codec. Existing BC energy, Yamato, impact and imported-skirmish gates
remain prerequisites. It reuses the existing hidden-child wire mapping and
selects a semantically valid predecessor before serialization. A winning raw
record is decoded and preflighted again before the existing load commit.
Outside and self-destruct child rows remain rejected. This does not claim
whole-load atomicity or admission of flight, weapon or gameplay services.

The generic journal selector now accepts a scratch capacity larger than a
journal payload: checksum validation still limits the actual record to a
slot. This is needed because a Zerg scratch union is 16404 bytes while the
largest payload is 16368. The selector regression checks a larger buffer and
its untouched suffix. The ZP live commit must use the same authenticated
wire decode as candidate preflight; a direct raw decode would leave the child
in its serialized representation.

Sanitized fresh-process fixtures use synthetic skirmish setups on both maps.
For Zerg, P7 and Showcase they exercised hidden training save, resumed
completion, fresh inside-child reload, outside-child write refusal with
unchanged SRAM, and fallback from a checksum-valid invalid newer child to a
valid older one. Separately a current-code gate-off writer uses a normal spawn
and save to produce an empty Reaver; the gated reader migrates it, writes and
reloads it, and falls back to it when a newer Scarab-tagged child record is
invalid. These are not saves from a historical release or ordinary paid play.
The original Terran hidden training/inside fresh-process sequence also passed
on both maps. Detailed argv, process exits and SRAM hashes stay in the
ignored `.local/codec-receipts` directory.

All three non-Terran full probe ARM links passed unchanged reserve guards:
Zerg `02037164/03004f68`, P7 `0203831c/03004c38`, and Showcase
`0203df70/03004ff0`. P7 uses a newly generated, ignored fact table with
SHA-256 `c720227debe1ca0bd96e7f0687df6f5d6890258155a99778eca2f49c8a425be1`;
source inputs were reused, not committed. Showcase has only 16 bytes before
`03005000`; more runtime work requires a new fit check. These links do not
prove playable flight, all map/race combinations, old experimental packed
parent compatibility, hardware timing, or full save-state equivalence.
The same full probe link for Terran passed at `020370f0/03004e68`. The 104
frozen input hashes matched again after these checks.

At source `00917a0`, a separate unprepared checkout passed `./dev build` and
`./dev test`: 842 Python tests, 23 skips, the separate 5- and 8-test suites,
and all native checks. The default ROM SHA-256 was
`73c0deeeb36cca4cb8614248a7e60cadac11a0ec542f2ece31c37b171e2c56b8`,
matching the preceding port. Four normal gate-OFF ARM builds passed unchanged
reserve guards: Terran `020370c0/03004b78`, Zerg `02037134/03004c48`, P7
`020382ec/03004a18`, Showcase `0203df40/03004e08`. The oversized-scratch
selector regression passed. A schema-derived pre-BC impact-envelope migration
and old experimental BC-marker rejection fixture passed under all four
profiles; it is not a historical-release save. All 104 frozen input hashes
matched after the checks. Detailed outputs remain in the ignored qualification
and codec-receipts directories.
