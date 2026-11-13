# Exact bounded facility flood

The normal P10 cartridge can spend tens of millions of guest cycles in one
update building depot and melee perimeter fields. These are multi-source
reverse floods. They share the point route field pool but have a distinct
cache key and stop after discovering the requested walkable origin.

`CANONICAL_ROUTE_FACILITY_FIFO=1` opts into a padded byte grid and compacting
FIFO on maps of 64, 96 or 128 columns and up to 128 rows. The probe shares the
point flood's scratch layout and packed field conversion, with a separate
kernel for the bounded facility stop. It seeds in row-major order, visits
left, right, up and down, and finishes all four neighbors of the pop that
discovers the origin. An origin on a seed stops before the first pop; a
blocked origin requests the full field. The existing publication path owns
cache keys, age, partial validity and the exact node counter. Frontier or
seed overflow falls back to the original flood before publishing any packed
bytes. The probe uses existing scratch and does not change the point flood
option or the order in which workers request fields.

`make test-sc64-facility-fifo` compares all packed bytes against an independent
asset-free oracle on five map shapes with both depot and perimeter seeds. It
also checks cache hits and rebuilds, complete and partial validity, original
grid bits and flood counters. A 32-entry variant forces overflow and must
still match. `make test-sc64-depot-routes test-sc64-route-dense-flood test-sc64-route-linear`
also passes.

The P10 candidate adds only the facility flag to the pinned normal cartridge
compile vector. It verifies at 32 MiB, SHA-256
`291a0da45e4194c992e28339c8a6ed03dff6e205951638b39bd1b2c86d4199fa`.
The BSS end remains `0x0203c0e0`; IWRAM grows by 448 bytes to `0x03004c70`,
below the unchanged `0x03005000` guard. The control is the previously
qualified normal P10 icon ROM (`7a8aa4da8bf156993db753b18a2fc2e02ad6bd880a58f014b6593b983a781310`).

The paired mGBA replay starts at HOME and uses the same physical button
transitions. At all three sampled peaks the scoped clock and instruction trace
agree, and route build and hit counters match between ROMs:

| Call | Whole update, guest cycles | Facility lookup and flood, guest cycles |
| ---: | ---: | ---: |
| 9,674 | 42,470,903 → 19,792,901 (−53.40%) | 34,588,398 → 17,506,384 (−49.39%) |
| 11,045 | 45,253,308 → 19,432,711 (−57.06%) | 40,405,690 → 17,119,499 (−57.63%) |
| 32,644 | 43,187,544 → 24,678,587 (−42.86%) | 30,015,545 → 16,887,491 (−43.74%) |

The padded expansion itself takes 6.78M, 7.18M and 5.89M cycles.
Grid expansion and field packing together now account for more of the
facility fill than walking the FIFO.


Both runs win on call 47,194, frame 46,057. All 10 briefing pages, 14
acknowledged dialogue pages and 20 campaign observations match. All 19
ordered video and state samples match in both indexed pages, palette, fog,
raw unit/building/mineral banks and route metadata, grid and counters.
Mean whole-loop cost falls from 1,907,674 to 1,886,340 cycles (−1.12%).
Through call 47,000, the recorded peak falls from 45,252,677 to 32,223,411
cycles (−28.79%). Both runs reach about 76.5M in the final 194 calls; the
probe does not resolve that late spike. These are emulated guest costs.

Paired P10 replay receipt SHA-256 (control, then candidate):

- `045583e14ce72cea4043f79666c1bd29dadad2eeec261292157576d91b6f0b07`
- `2bed85fb910b39e31fa07ba41858a0d190af1b1002412968924b33b7f383a6da`

## Cross-cartridge checks

All four opt-in 32 MiB cartridge builds verify with unchanged BSS ends and
448 more IWRAM bytes each. Their IWRAM end addresses are `0x03004908` (Terran),
`0x03004c70` (P10), `0x03004da0` (Z10), and `0x03004ec0` (Showcase). Showcase
has 320 bytes before the unchanged guard. The default remains off.

On the normal Z10 cartridge playing Z7, 39 named checkpoints and 67 ordered
raw world and video samples match, including both indexed pages, palette,
fog and exploration. The 63 non-menu samples average 1,318,076 → 1,318,548
cycles (+0.036%); the four menu samples average 2,396,722 → 2,399,501 (+0.116%).

A physical Z10 Blood Bath test reads 64×64 geometry at launch and after
fresh load. Both ROMs match 600 neutral play updates, the pause save,
restored state, and 60 neutral updates in a fresh emulator process. Launch
route builds and hits are 14 and 2; they reach 16 and 519 by the save prefix.
Both ROMs export identical 32 KiB SRAM, which remains unchanged after load
(SHA-256 `d28f76b40d5df666ca5eba94efccc68aefad06e1e59eee5c3fbddffa0a75e6fe`).
