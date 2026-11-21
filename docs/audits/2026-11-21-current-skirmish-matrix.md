# Ordinary-button skirmish matrix

The three normal race ROMs at `e92caada` launched both Outpost and
Blood Bath through the Custom Game menu in mGBA. Each cartridge tested
all three human races. Each of the three independent CPU rows was
Terran, Zerg, Protoss, or Closed. The all-closed lineup was excluded.
This gives 63 lineups per human race and 378 starts per cartridge.

| Computer opponents | Lineups per human race and map | Launches across all three cartridges |
| --- | ---: | ---: |
| 1 | 9 | 162 |
| 2 | 27 | 486 |
| 3 | 27 | 486 |
| Total | 63 | 1,134 |

The opening checks verify the selected map and race, the occupied seat
mask, the resolved CPU races, and distinct spawn locations. Each owner
has a town hall and four workers of the selected race. A Zerg owner
also starts with an Overlord. Every owner starts with 50 minerals.
All 1,134 games advance 120 in-game updates with the same occupied seats.

For each cartridge, map, human race, and opponent count, one lineup
also saves through START and B. That produces 54 completed 32 KiB
journals. Fresh HOME L restores the saved gameplay fingerprint, races,
and start locations. The following 60 updates match uninterrupted
play in every case, and the re-exported SRAM matches the original.

| Cartridge | ROM SHA-256 |
| --- | --- |
| Terran | `682c223e1b984c88ce362fa93309d006e6dd80416ef111a50d34c6a4eabd2df9` |
| Z10 | `1da8c6a4bc7fe0d9f8989925b1f2fecd094072583db5b1df35b361fbf8eac478` |
| P10 | `aa9960187f398c6c95ac618d4399a0492032bc886a154ba835e4e1308785987f` |

The six local reports keep the exact ROM and ELF hashes, case coverage,
launch and continuation receipts. The aggregate receipt SHA-256 is
`c858dc048f28009069b2046d31777b875924d12b80716dbfdff2f9ad9d933e0d`.
The local runner SHA-256 is
`f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d`.
Keep the prepared images and SRAM files local.

## Repeat a saved matchup

Use the existing [skirmish continuation driver](../../tools/sc64_skirmish_sram_continuation.py)
with the matched local ROM and ELF and the physical-button mGBA backend.
This example opens the first and third CPU rows in a Blood Bath match.
Use a fresh output directory.

```sh
python3 tools/sc64_skirmish_sram_continuation.py --physical \
  --rom "$SC64_ROM" --elf "$SC64_ELF" --runner "$SC64_RUNNER" \
  --map 1 --human t --seats=p-z --play 120 --continue 60 \
  --output "$SC64_OUTPUT"
```

Repeat the opening and fresh continuation checks when a release changes
roster options or the save format. Longer paid-production, skirmish wins,
and physical-device timing have separate gates.

## Current sparse-route and Terran Carrier builds

The normal-title input matrix was repeated after sparse route packing
and with the Terran Carrier candidate composed with it. The exact
tested ROMs were Terran `d9a2bd5f…`, Z10 `ebc0d0f4…`, and P10
`6e0f09d4…`. Z10 and P10 are the ROMs from
[#659](https://github.com/ggu/starcraft-gba/pull/659). The Terran
candidate was linked from `685d4a7d` before the final release-only
Makefile change reproduced the same normal ROM.

All 1,134 ordinary starts match the preceding roster matrix through
the first 120 game updates, including the map, seat mask, races,
building locations and world fingerprints. The comparison includes
162 single-AI, 486 two-AI and 486 three-AI cases. The Z10 comparison
uses the normal Carrier build as its earlier baseline. All 54
sampled pause journals also load from HOME in fresh mGBA processes
and continue for another 60 matching updates. These are startup and
save checks. The [result replay](2026-11-21-blood-bath-live-results.md)
covers a separate full match.

Private aggregate SHA-256
`9a6793cd3df2c41f92d37b1abcd25347dbc863eb1ac2da0c611a34098010cbcd`.

The shipped Terran ROM `c32005c7…` from #659 separately passed both
maps with all 378 named lineups and 18 fresh pause loads. Its launch
and 120-update fingerprints match the Terran candidate above. Combining
those Terran runs with the unchanged exact Z10 and P10 runs gives
1,134 launches and 54 fresh continuations on the three shipped ROMs
from #659. All 54 re-exported journals match their own input SRAM.
The exact-release private aggregate SHA-256 is
`994e4901b1bb8b689f99db93c0df6ae24dfe5f82085100e3df107126d6b07672`.

## Three race ROMs with the optimized Carrier service

The [combined Carrier runtime](2026-11-21-carrier-empty-training-pass.md#shared-cartridge-qualification)
repeats the full matrix against the exact three #659 ROMs. The tested
Terran, Z10 and P10 ROMs are `455b9e1c…`, `86324a99…` and `91af2b84…`.
All 1,134 launch fingerprints and their 120-update world observations
match #659. Each of the 54 sampled pause journals restores its saved
state in a fresh guest and yields the same 60-update continuation as
both uninterrupted play and the matching #659 case. Every immediate
SRAM re-export matches the journal written by its own ROM. All 36 Z10
and P10 journals also match the previous release byte for byte. Terran
writes the new compact Custom Game format.

The private paired matrix SHA-256 is
`58edfd72acbcd0a762fc4155bbffb558488605305b58989fc95301c26f93d207`.

### Independent Random rows

The same three optimized ROMs also launch every lineup that contains a
Random row. This adds 1,098 starts across both maps, with 54, 378 and 666
starts for one, two and three opponents. Each Random row resolves to
a valid race. Named rows keep their selected race, and Closed rows
remain empty. Spawn locations and starting units match the resolved
races through the first 120 game updates. The test observes all three
possible Random resolutions without testing their statistical distribution.

Another 54 pause saves sample each cartridge, map, human race and
opponent count with a Random row. All preserve the resolved races,
seat mask, start positions and saved world after a fresh HOME load.
The next 60 updates match uninterrupted play. All re-exported SRAM
files match their inputs byte for byte. The private Random matrix
SHA-256 is `11158cac20466a56278373858cee7ec6bf8018b5cf73c7d0c67c457e9367c15c`.

## Packed building release

The three race ROMs from the [packed building release](2026-11-21-terran-building-word-draw.md#shared-release-checks)
repeat all 2,232 named, Random and Closed lineups from #663. There are
216 one-opponent, 864 two-opponent and 1,152 three-opponent starts.
Both maps, each human race, resolved Random races, spawn locations and
the next 120 gameplay updates match the corresponding #663 cases.
All 108 sampled journals match #663 byte for byte and restore the same
world and another 60 updates after a fresh HOME load. Each immediate
SRAM re-export also matches. The private comparison SHA-256 is
`44e3b81dae4ca6668e3c08082d0ab794c4e38ace8d1cacf3d0dfc399ee4bb15b`.
