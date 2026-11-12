# Independent custom game seats

The [owned N64 Blood Bath lobby](2026-11-12-n64-blood-bath-opening.md#individual-computer-slot-options)
showed separate named, Random and Closed options. Right moved from
Protoss to Random, then Closed, then Zerg. With the first and third rows
closed, the original started a match with only middle-row Terran. The
field captures did not establish which physical start that computer used.

The GBA custom menu now keeps all three seats visible. Closed skips that
map start. Random resolves once per seat on Launch; restart and load
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
