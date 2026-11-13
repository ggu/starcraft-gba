# Independent custom game seats

The [owned N64 Blood Bath lobby](2026-11-12-n64-blood-bath-opening.md#individual-computer-slot-options)
showed separate named, Random and Closed options. Right moved from
Protoss to Random, then Closed, then Zerg. With the first and third rows
closed, the original started a match with only middle-row Terran. The
field captures did not establish which physical start that computer used.

The original GBA seat implementation kept all three seats visible and
skipped the map start associated with a closed row. [Later Blood Bath
checks](2026-11-12-blood-bath-resolved-starts.md) found two specific lobby
layouts that use different physical starts. Random resolves once per
seat on Launch; restart and load
reuse the resolved race. Both maps and all human races remain available.
The Opponents shortcut opens the first N seats, and a lobby cannot start
with no opponents. Sparse setups use the corresponding existing GBA map
starts for both initial towns and AI planning. This does not establish
the N64's physical slot-to-start mapping or random seed algorithm.

The runtime keeps the existing dense sequence of internal AI owner IDs
and maps each occupied seat separately to the existing start locations.
The SRAM payload length and layout stay unchanged. Dense games write the
old three named race fields unchanged in form. A sparse game can have at
most two opponents, so its inactive third race word carries a canonical
seat marker. Current readers validate the mode, occupied-seat count,
reserved bits and race value before loading a sparse game. Predecessor
imported-profile readers only accept named values in that field and will
reject a sparse record; the usual two-slot validation can still fall
back to an earlier dense record if it exists.

The frontend sanitizer fixture covers the observed option cycle, a
middle-only match, Random resolution, the empty lobby and invalid wire
values. `make test-skirmish-openings` covers 234 dense starts and eight
sparse starts per profile, using normal menu inputs. Each sparse match
checks live AI starting positions and restores its exact seat mask and
resolved races after a game reset and SRAM load.

A Terran fixture used the same Blood Bath lobby against named Zerg and
Protoss on the predecessor (the tree at `20469c2`) and this implementation.
The two 32 KiB SRAM images were byte-for-byte identical (SHA-256
`414eefb40135637b5809e5f3e48aa942a0905559f0eb9811f8606a031c037b2a`),
and each build loaded the other's image. Three paired normal-controller
matches (Terran against two on Blood Bath, Zerg against one on Outpost,
and Protoss against three on Blood Bath) all reached the same terminal
outcome on both builds: a human defeat. The controller schedules actions
by host tick, including time spent in the lobby, so its terminal frames
can differ when the menu input path changes.
A loader-level negative test rejected four correctly checksummed SRAM
records with malformed seat markers and left the title state untouched.

The four canonical ARM products built and passed the 32 MiB cartridge
verification: Terran, Zerg Z10, Protoss P10, and Showcase.
Physical button playback on all four at runtime source `60474982` also
refused an all-Closed lobby, resolved named and Random opponents, and
checked live town coordinates for seat masks 2, 6, 1, and 5 respectively.
Each 32 KiB cartridge SRAM image loaded unchanged in a fresh emulator
process and matched 60 subsequent ordinary updates against the
uninterrupted match. This is a bounded continuation, not a full-match
ARM or N64 placement-parity check.

## Repeatable physical seat checks

Normal Terran T10, Zerg Z10, Protoss P10, and Showcase ROMs rebuilt at
`6e843d8` after the seeded-start preview merged. All four ROM verifiers
passed. On each ROM one sparse and one three-CPU lobby passed ordinary
physical launch, save, fresh-process load, and 60-update continuation.
Across the eight cases both maps, all human races and one, two, and
three opponents were exercised. All-Closed launch was refused in each
case. Fresh normal lobbies kept legacy start order; no SRAM bytes changed
on readback. The later Z5 qualification changes test code only.

For an ordinary middle-only Random match on Blood Bath:

```sh
python3 tools/sc64_skirmish_sram_continuation.py --physical \
  --rom build/StarCraft-Terran-Transport1-GBA.gba --elf build/starcraft-terran.elf \
  --runner /path/to/sc64-headless-replay --output /tmp/sc64-middle-random \
  --map 1 --human t --seats=-r- --play 60 --continue 60
```

`--seats` supplies three visible rows: `t`, `z`, `p`, `r` for Random and
`-` for Closed. It checks the actual opening halls, the resolved races
and base IDs, and their persistence in addition to the normal world
fingerprint. It also accepts a ROM carrying the opt-in seeded starts.
The original `--bots` argument still selects contiguous named opponents.

## Ordinary four-profile Blood Bath preview

Build the ordinary Terran T10, Zerg Z10, Protoss P10 and Showcase profiles
with `BB_OPENING_FLAGS=-DSC64_NATIVE_MELEE_STARTS`. With the variable unset,
the existing custom menu and match selection remain the default. On the
preview, editing the last open Blood Bath CPU skips Closed; all four ROMs
match the observed native Right transition from Random to Zerg in both
modes. The reverse transition works on GBA; the native witness tested
Right only. A second CPU can still be opened on a separate row.

All four ARM products pass the existing ROM/RAM verifier. Relative to
KODAK, the preview uses 16 more EWRAM bytes per profile and 48 more IWRAM
bytes except Z10, which uses 64. All reserves remain above their guards.
All eight frontend sanitizer configurations (four products, flag off/on)
pass, as do the 16 continuation tool tests.

The physical runner used the normal HOME lobby, then played 240 updates,
saved through Pause, loaded in a new emulator, and compared a further
120 updates against uninterrupted play. Four cases passed per preview:
Outpost with three named CPUs, Blood Bath Free for All with one middle
Random CPU, Blood Bath Melee with one third-row Zerg CPU, and Blood Bath
Melee with three mixed CPUs. The eight legacy control runs for the first
two cases also passed. Separate physical edit checks launch two opponents
from nonadjacent rows in both modes and on all four preview products.

Fresh-process cross-loads matched in both directions for Outpost and
Blood Bath Free for All on each profile (16 cases); loaded state and the
120-update continuation matched the same-profile control. All four old
readers rejected the new allied Melee save without entering play. This is
intentional: old ROMs cannot safely play that save as Free for All. All
readers left the SRAM seed unchanged. These are opening and save checks,
not full matches, elimination fidelity, or performance on hardware.

| Receipt | SHA-256 |
| --- | --- |
| 24 same-build continuation cases | `131fc4f600c6d074f5ed3566ab7ddd3490f9d8d03a2ef9f2bdc785fd769a1bb3` |
| Eight two-seat edit/launch checks | `8a1561c765a3e1686e4aa588b8fa60ece3114863d14e496a1b4c1bf28b88493e` |
| Twenty cross-build checks | `3f9d628456f0a4c48c5af908bd4c2e7d5d2379a52f2aa22b2c659cbbe0bc3a49` |
