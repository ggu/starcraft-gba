# Experimental stack release qualification

Source `3cbb7665e3e471410929d65da94175765a445f73` combines the current
experimental stack and released Support Bay gate. The gate hides an unusable
custom-game purchase while keeping the catalog record and campaign paths.
Z7 is the default Zerg product; P8, Z8, Z9 and Z10 remain separately selected
candidates. Unfinished services remain gated, and the Transport1 namespace
does not automatically import predecessor saves.

The independent skirmish menu, four actual runtime fixtures, and Terran,
Zerg and Protoss same-process save/reload continuations passed under
ASAN/UBSAN. Nine normal ARM builds passed unchanged BSS/IWRAM guards. Eight
normal ROMs passed their official verifier; Showcase's normal ROM was fixed
but has no verifier target. ROM SHA-256 and final BSS/IWRAM ends:

- `build/starcraft-terran.elf`: `b1ba9303650ceaf320c9f26c4d95b8c5c2e64e577a1fdbc7de7552f43044d8f0`; `0x20370e0` / `0x3004b40`.
- `build/starcraft-zerg-z6.elf`: `1576aaf2d15a409a887b6b4bc33dca8a7291b1271f8849d8a4fd4b6707f33cdb`; `0x2037134` / `0x3004c10`.
- `build/starcraft-zerg-z7.elf`: `a771844d1b534cc4dee4a17dc1010582beef44789ca66ffc2b65f68962383726`; `0x203d718` / `0x3004f80`.
- `build/starcraft-zerg-z8.elf`: `6960c77c1b95f4264ab4cf9b696ec2400fdf1118ce0c0e60b19d2898cc9af55c`; `0x203dbd8` / `0x3004bf0`.
- `build/starcraft-zerg-z9.elf`: `8cbc085e9448ca85bf6f387b431ffc518aab4de814f9eb9a2cd9ff0bbd5bc459`; `0x203df88` / `0x3004bf0`.
- `build/starcraft-zerg-z10.elf`: `e5864c7b0a246408ffd3d83afcb4dfe7a324d14ee7ea79cf2f56f25e0557b4ce`; `0x203df88` / `0x3004bf0`.
- `build/starcraft-protoss-p7.elf`: `805d1b3d65aea727db1e677a9a5b8f7c9b84aa38b2a19686463ea845bf625b7c`; `0x2038314` / `0x3004a28`.
- `build/starcraft-protoss-p8.elf`: `b211e63a0b1b10094ce47922935389e4e8c3294e3626056252392188f12b7e86`; `0x2038cdc` / `0x3004a28`.
- `build/starcraft-showcase.elf`: `3ba09a5dfa5c5eaf70f2bfbd94eb601ab88e3ed68620ad06753cb5c36e29b95a`; `0x2037008` / `0x3004cc0`.

Z9 and Z10 end BSS at `0203df88`, leaving 120 bytes before the existing
`0203e000` guard. No guard or reserve was relaxed. The candidate programs and
private banks were pinned inputs and are not committed.

A separate clean unprepared checkout passed source-boundary, doctor, default
build and full default test: 951 Python cases with 26 skips, separate five-
and eight-test suites and native fixtures. Source, 109 pinned inputs and
auxiliary inputs were unchanged. The local focused and clean receipts are
`/tmp/sc64-release151-final-focus-20261101/progress.json` and
`/tmp/sc64-release151-final-clean-output-20261101/progress.json`.

Earlier Z7 and P8 physical victories are tied to their previously recorded
ROMs, not the new Z7 `a771844d…` or P8 `b211e63a…` above. Z9/Z10 candidate
builds do not imply ordinary-input victories. This source release does not
claim all 30 missions, complete roster/AI fidelity, hardware performance or
30 fps. No ROMs, extracted assets, saves or captures are committed.
