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
