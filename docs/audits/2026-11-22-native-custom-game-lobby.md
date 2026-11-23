# Native Custom Game lobby controls

An ordinary USA N64 run in ares reached Setup Custom from reset.
Press Start, select Single-Player with A, return from the initial
tutorial briefing with B, move Right to Scenario and open Setup Custom
with A. Challenger shows Melee, two starting slots, Human Terran and
Computer Zerg. The visible rows cover GameType, Map and the two seats.

On a separate two-slot map, Down twice selected the computer row.
Analog Right changed Computer Zerg to Computer Terran. Further inputs
reached Computer Random and brought the row back to Zerg.

A fresh ares process loaded a retained ordinary Blood Bath FFA lobby.
It shows four starting slots, Human Terran, and three computer rows
set to Zerg, Terran and Protoss. Analog Right on the first computer
row changed it from Zerg to Terran. The sampled lobbies expose no
difficulty field. Other native modes and menus are outside this observation.

The regular GBA race cartridges keep their two-map selection and
independent human and computer races. Their [launch matrix](2026-11-22-command-owner-lookup.md)
exercises one to three CPU choices on each map. The native lobby
observation supplies a reference for the two-slot and four-slot menus.

| Native screenshot | Local SHA-256 |
| --- | --- |
| Fresh Challenger lobby | `0721f9f5cf4d73f6e666ecd15bada3fda734a772bafd504165a193c9ccd004f8` |
| Race changed on a two-slot computer row | `d56695068654e3c07cce37fd942deff5fe9be916a06dfe6c9501b466b0ff1f34` |
| Race changed on the first Blood Bath computer row | `120513ed05f693acffd85ec34a5919c83616d1401347b4fbbbcba400afe14f74` |

## November 23 preview on current cartridge inputs

The released Terran ROM reaches ordinary Custom Game setup without a
Melee selector. With the existing `SC64_NATIVE_MELEE_STARTS` option,
all four profiles on `37e45126` link against the same prepared inputs
with the original memory guards. Zerg leaves 32 bytes below its IWRAM
code limit.

Twenty ordinary launch, pause and fresh HOME cases pass across Terran,
Zerg, Protoss and Showcase. They cover Blood Bath Melee with one, two
and three CPUs, plus three-CPU Free for All on Blood Bath and Outpost.
Each resumes for 120 neutral updates and matches uninterrupted play.

Sixteen cross-version Free for All cases restore the recorded lobby,
game frame, seats and starts. They cover both maps and both directions
between the preview and the regular ROM. The four regular ROMs reject
the preview's Melee saves from HOME. All 20 cross-version cases
preserve the input SRAM when re-exported.

Eight ordinary launches resolve a Random human and a Random CPU with
one Closed seat. Each profile repeats its resolved races and start
order under the same inputs. All 30 campaign openings match the
regular reference after explicitly checking that the added Melee
state is zero. Four saved campaign starts (T10, Z8, P9 and P10)
load to the same state on both cartridges and preserve their SRAM.

Each neutral three-CPU Melee replay reaches RESULT between game
frames 11,523 and 11,704. Across the 11,400 fully sampled updates,
the four profiles average 25.19–27.07 emulated updates per second.
The final full 600-update windows average 16.08–19.70. A full
active win remains a separate check. A pause at update 6,000 and
load from a fresh HOME process produce the same defeat frame,
world state, both video pages, palette and exported SRAM as resuming
the pause in the original process, on all four profiles.

| Local receipt | SHA-256 |
| --- | --- |
| 20 ordinary save continuations | `cdb2c3e16eb6310553329cacc14a57586a63ac0837a213019c82abfeb6f30f28` |
| 20 cross-version load and rejection cases | `299e6290b0fe1d43bca291cbc14b93165e89f499fcfdf4cb66cd4b537d821dad` |
| Random and mixed-seat launches | `de0473b6098fd650339c7e2f27faaea7b00b9b33f877ed8824451dc133e24247` |
| Four legacy campaign save loads | `853448855dc39fdc1f62ad41950075d474dde4f99a6dba9ee7296b6cd7b7bd9e` |
| Four three-CPU Melee runs | `4750d836940d7d906147bd0df5897ca6cf7a7b0c6c2538f989393c13d6041e35` |
| Three-CPU defeats after fresh HOME load | `6838dd54bd2a5bdea0fbc087e3b08b45b8283534fa71aecba0cf71d8cc893340` |

## Instructions in late three-CPU Melee

Ordinary neutral replays on Terran and Protoss match paired
unprofiled runs at 32 cuts per product. Every cut matches game
state, route counters, both video pages, palette and elapsed guest
cycles. The instruction histograms and call stacks balance at
each sampled update.

| Mean guest cycles | Terran updates 9,001–9,016 | Terran updates 11,385–11,400 | Protoss updates 9,001–9,016 | Protoss updates 11,385–11,400 |
| --- | ---: | ---: | ---: | ---: |
| Whole loop | 689,180 | 970,140 | 759,876 | 1,353,772 |
| Orders | 342,700 | 413,254 | 359,536 | 502,125 |
| Uncached contact mask within orders | 111,785 | 91,215 | 47,225 | 53,861 |
| Rendering | 213,032 | 385,028 | 247,108 | 581,950 |
| Depth-ordered actors within rendering | 64,704 | 218,656 | 86,691 | 389,867 |
| Audio IRQ | 14,972 | 43,839 | 17,770 | 119,233 |

The Protoss late window spends 96,720 cycles on tracer drawing and
about 108,071 in the ordinary sprite blitter. Audio's first-order
decoder alone accounts for about 68,018 cycles. The nested values
overlap their enclosing stages. In these late windows both profiles
miss the 30 fps budget. These are emulator measurements.

Three bounded probes preserve the state and display samples. A
per-tile-row visibility cache in the projectile raster changes the
Protoss late mean by about 9 cycles. A zero-width audio decoder
shortcut raises that whole-loop mean by about 36 cycles. A
single-direction contact bucket bound raises the Terran means by
0.58% and 0.34%. Keep all three out of normal builds. Measure actor
and terrain work that can be reused across the two video pages, and
which contact queries recur as units move between cells.

| Local instruction report | SHA-256 |
| --- | --- |
| Terran baseline | `e58feac2b13368c56edcd90ce45590058d733d4a8933495277e13d936a346fa3` |
| Protoss baseline | `c9fb55c51e18157705f615c94e25fce99e56f14c62e60d950202d0d714c2ff97` |
| Projectile fog cache | `63a97f37c0d43edefb5c06a451d47eeab8e257b3824a3501d8ad2ed8c08da88d` |
| Zero-width audio shortcut | `5e5f894297453c646e9365750629f2133439126ede99fd851593ec52a17b4345` |
| Single-direction contact bound | `24c992c419e196db6be74229ac4372a25bea01808439f8a9352bcca9ede9697a` |

## Same-page battlefield changes during Melee

Replaying the same four windows with both Mode 4 page addresses
observed independently gives these mean changes on the work page.
The camera and palette stay fixed throughout. The work page is
identified by the renderer's backbuffer index. The other physical
page stays byte-identical, including at two samples where DISPCNT
still shows the previous displayed page.

| Window | Draws with identical battlefield pixels | Changed rows | Changed 16×16 tiles | Contiguous tile spans | Changed pixels |
| --- | ---: | ---: | ---: | ---: | ---: |
| Terran 9,001–9,016 | 15/16 | 0.06 | 0.06 | 0.06 | 0.06 |
| Protoss 9,001–9,016 | 12/16 | 1.13 | 0.25 | 0.25 | 7.13 |
| Terran 11,385–11,400 | 0/16 | 86.69 | 30.19 | 7.94 | 2,039.94 |
| Protoss 11,385–11,400 | 0/16 | 83.75 | 30.56 | 8.56 | 1,695.19 |

The battlefield has 108 rows and 105 tiles, including the shorter
bottom row. Identical final pixels are an upper bound on safe reuse.
An exact damage tracker must retain per-page contributor coverage,
invalidate both old and new bounds when a contributor changes, and
redraw every intersecting contributor in painter order. The late
windows leave approximately 585,000 Terran cycles and 772,000 Protoss
cycles after subtracting all renderer time. The 30 fps goal in these
windows also requires cheaper orders and other simulation work.

An alternate sprite loop reads aligned source pairs as halfwords.
Both host pixel and 32 ARM state/display checks pass. The first
version exceeds the IWRAM guard. A smaller version fits and reduces
Protoss IWRAM by 24 bytes, while increasing the earlier and late
update costs by 0.02% and 0.07%. Keep the existing loop.

| Local receipt | SHA-256 |
| --- | --- |
| Same-page battlefield census | `c186965ee7f40727d40d5e6c016459cfb2268694a009c22d6f4340f9994912df` |
| Halfword source trial | `ebba58f187ebbc9f59072b4b9e04bbadaaf4f09f2d683b0935bc03ce4c82a955` |

### Sprite coverage on the current physical pages

The `1f0ad66e` Protoss 10 cartridge reaches the same neutral
three-CPU Melee windows on ordinary inputs. Breakpoints at the
sprite compositors record the screen rectangle, source identity,
clip and call order. Treat non-ROM pixel or lookup addresses as
changed. The 24 cuts match the control frames, route counters,
world summaries, both display pages and palette.

The model follows each physical work page. It marks a region when
its ordered sprite contributions change or its final pixels differ.
The first two cuts in each window initialize the two pages. Values
below cover the remaining ten draws per window. Sprite area
counts overlapping rectangles independently. The whole-call count
lists rectangles that avoid every marked region. The area column
also counts portions of calls that intersect a marked region.

| Window and region | Mean regions marked | Whole calls outside marked regions | Sprite area outside marked regions |
| --- | ---: | ---: | ---: |
| Earlier, 16×16 | 0.3 | 60 of 61 | 99.98% |
| Late, 8×16 | 65.2 | 50 of 402 | 24.42% |
| Late, 16×16 | 39.6 | 50 of 402 | 21.35% |
| Late, 32×16 | 24.0 | 50 of 402 | 19.87% |

All 50 whole-call skips in the late window are byte-sprite
compositions. All 195 packed-word calls intersect a marked region.
This measures rectangle area on the sampled workload. A shipping
tracker also needs terrain, shapes, text and overlay coverage.
Measure a fixed-camera whole-page fast path in the quiet window
and keep late battle optimization focused on paint and simulation
costs that still apply in changed regions.

The local coverage receipt SHA-256 is
`6e12d781e41aaf1a035851eaf6cb83104f81677043e102dd702811a916471fa2`.

### Measured cost of skipping a certified quiet battlefield draw

A replay-only build skips the battlefield pass for calls
9,008–9,016, where the stock replay already shows that both
physical pages are unchanged. The trial keeps simulation, audio,
the HUD, overlays and page presentation active. All 32 sampled
states, route counters, both display pages and palette match stock.
The selected nine updates establish an optimistic timing measurement
before runtime comparison or invalidation cost. The frame-number
gate is local to the trial.

| Mean guest cycles | Stock | Reused page |
| --- | ---: | ---: |
| Nine certified quiet updates | 742,158 | 594,804 |
| All 16 earlier updates | 757,329 | 674,445 |

The nine-update mean improves 19.85%, from 22.61 to 28.21
emulated updates per second. It remains 33,012 cycles above
the two-VBlank budget before adding a runtime guard. A safe
implementation needs an exact contributor snapshot and checks for
camera, fog, page clear, mode and overlay changes. Keep measuring
orders and shared simulation costs alongside that implementation.

The local attribution SHA-256 is
`49a33950e9f362fb84529d9fe801811b58e287f66fbcbb4394078fd4d7ab2698`.

### Sequential transfers for packed sprite pairs

An ARM trial fetches two color and opacity pairs per sequential
ROM transfer. Odd rows begin with one pair. Both host pixel
fixtures pass, and 32 paired Protoss 10 Melee samples match
frame, world, routes, both physical pages and palette.
In the late window, packed raster calls fall from 69,352 to
64,832 cycles per update. The whole loop falls from 1,350,466
to 1,346,508, a 0.29% reduction. The earlier mean falls 0.46%.
The trial increases Protoss 10 static IWRAM by 48 bytes and
fails the Z10 linker guard for its 11 KiB stack reserve.
Keep the current loop in the shared builds.

The local attribution SHA-256 is
`7bb64e17d60efc670423c48b3ca76b01bedb94f6ce1c00eb1aecf401786cbda5`.

### Restoring old actor pixels to terrain

The same 64 advancing updates also compare the old work page with
the bare cached terrain. The old and new cache bytes agree at
every cut, and the camera, fog and palette match. The nonwork page
also stays byte-identical before and after each draw.

| Window | Mean affected pixels | Mean affected 16×16 tiles | Mean aligned halfword runs | Mean bytes in exact runs |
| --- | ---: | ---: | ---: | ---: |
| Terran 9,001–9,016 | 3,696 | 57 | 341 | 4,162 |
| Protoss 9,001–9,016 | 4,246 | 63 | 356 | 4,653 |
| Terran 11,385–11,400 | 5,603 | 73.5 | 498 | 6,273 |
| Protoss 11,385–11,400 | 6,496 | 80.125 | 531 | 7,143 |

All 108 rows have at least one difference. One enclosing span
per row copies the full 25,920 bytes in every sample because
the edge pixels differ. Omitting the two outermost columns for
a diagnostic reduces the span counts, but the late windows
still average 286 Terran and 396 Protoss exact runs. These
are pixel coverage counts. They exclude discovery and transfer
setup cost and the work to redraw each changed contributor. The
simultaneous final-page and bare-terrain measurements point toward
retaining unchanged overlays instead of redrawing them after
hundreds of small transfers.

The terrain restore receipt SHA-256 is
`705d71ee45bdc0468e7edeefb5df97f70f79af28daefee0c1755f7239cb5ad51`.

### Terrain filtering in the contact scan

Four Terran cuts on the same replay sample all 80 uncached contact
calls. Terrain accepts all eight candidate directions for 75 calls
and five directions for the other five. In two calls, the returned
mask blocks every terrain-valid direction while leaving at least
one invalid direction unblocked. Two calls is the upper bound for
newly available early exits in these cuts. The census does not show how many additional bucket
visits could be avoided or whether the terrain precheck saves cycles.
All four cuts match the pinned frame, world, both video pages,
palette and route counters. Keep the current traversal.

The contact census receipt SHA-256 is
`732ece93d68f41e886454d97e5ac5520442758a92fe92b60f03576190c3b1d4a`.

### First-order audio on ARM

The packaged Protoss 10 ROM and a trial using ARM instructions
for the first-order decoder run the same neutral Melee inputs.
The trial retains the same decoder source and layout of its
persistent state. Disassembly confirms the changed instruction
set. Both RAM endpoints match the stock cartridge. All 32 sampled
world states, route counters, frames, video pages and palettes
agree. Codec, mixer and Python/C stream tests pass on the host.

| Mean guest cycles | Earlier, Thumb | Earlier, ARM | Late, Thumb | Late, ARM |
| --- | ---: | ---: | ---: | ---: |
| Whole loop | 757,329 | 762,780 | 1,350,466 | 1,381,937 |
| First-order decoder | 12,433 | 17,753 | 68,048 | 98,303 |

The trial raises whole-loop cost by 0.72% and 2.33% in the two
windows. These are emulator measurements with the ordinary audio
interrupts active. Keep Thumb decoding in the shared builds.

| Local timing report | SHA-256 |
| --- | --- |
| Packaged Thumb control | `45a99a47d032f7004a61d8af6a9b44d0d6dc84de7bb70b967d778d775786b302` |
| First-order ARM trial | `c87a93d27a4b4888b61a6295a79d168fa5747cbf3854569cdc801023d1526084` |

### Packed saturation in the shared mixer

A second trial replaces the scalar clipping fallback in the packed
PCM mixer. A sanitized scalar comparison checks 1,890,318 pairs of
reachable lane sums from two, three and four voices. The mixer
fixture passes and the Protoss 10 memory endpoints match stock.
Both paired ARM observers agree at all 32 samples for frame, world
state, route counters, both video pages and palette.

| Mean guest cycles | Earlier, stock | Earlier, trial | Late, stock | Late, trial |
| --- | ---: | ---: | ---: | ---: |
| Whole loop | 757,329 | 758,107 | 1,350,466 | 1,347,383 |
| Audio IRQ | 17,924 | 18,704 | 119,261 | 116,179 |

The late window saves 0.23% across the whole loop. The earlier
window rises 0.10% and includes one more audio interrupt across
16 updates. Keep the trial local and concentrate on the shared
orders and actor composition costs.

The local trial attribution SHA-256 is
`a7bec653dfdcdf0e1053c92b4e04680803b33d2363a34ee45c7f1505fbbbd49f`.

### Shorter cloak-cache scans

The trial cloak tick checks the few relevant runtime types
directly when an actor lacks an explicit source type. Its type
predicates and general type-to-source mapping use the same rows.
Explicit source IDs still take precedence. The GBA Thumb path also
checks unused unit rows with grouped word loads. Every byte,
including padding and the trailing words, still participates.

An isolated ARM fixture sets each bit separately across 48-, 52-,
56- and 60-byte rows, and checks fully zero rows at each slot.
All 13,856 checks pass. The host cloak differential matches its
scalar reference across 24 worlds and 8,640 updates. All four
normal cartridges retain their static RAM endpoints. The Terran
and Zerg ROM hashes match the prior package.

| Three-CPU Melee window | Mean guest cycles, stock → shared scan | p95, stock → shared scan |
| --- | ---: | ---: |
| Protoss 10, 8,401–9,000 | 766,169 → 760,863 | 904,103 → 899,148 |
| Protoss 10, 10,801–11,400 | 1,040,170 → 1,034,282 | 1,350,192 → 1,355,686 |
| Showcase, 8,401–9,000 | 756,315 → 751,544 | 891,401 → 890,774 |
| Showcase, 10,801–11,400 | 913,889 → 909,270 | 1,145,718 → 1,143,908 |

The paired replays match complete unit banks, world summaries,
route counters, both display pages and palette at 84 checkpoints.
The late Protoss mean improves 0.57% and its p95 rises 0.41%.
The late Showcase mean improves 0.51% and its p95 falls 0.16%.
These windows still miss the two-VBlank budget. All 20 ordinary
skirmish save continuations and 30 campaign openings pass.
Four historical campaign saves load and re-export with the same
SRAM bytes. The full ordinary Protoss 10 HOME replay wins at
frame 93,377 with all 37 checkpoints and three SRAM exports
matching. Across all 94,852 calls the mean rises from 1,095,036
to 1,098,010 guest cycles, a 0.27% increase. Keep the trial local
and retain the larger orders and renderer work as priorities.

| Local receipt | SHA-256 |
| --- | --- |
| Protoss 10 sustained candidate | `19b61e33dc75ee2177b18d01d7dd0e8c878392ce7e865d562f593946f4ad3be7` |
| Showcase sustained candidate | `5311f302d2d1f715278e113f374f901ad21a7aba121592b2c6962f08c1daa11b` |
| Protoss 10 full victory and fresh readback | `e8a2fbb49fc04657b3ed237f04cc1dada5495b63f9384c173b2572ab440bce72` |

### Occupancy checks for steering rows

The address census at the actual steering bucket-head load covers
calls 11,385–11,387 on the packaged Terran cartridge. Of 104 row
spans, nine are empty, and dropping them would avoid 15 of the
184 bucket-head byte loads. A separate row bitmap would require
104 additional word loads and 2,048 bytes of EWRAM for the
128-by-128 grid. World state, route counters, both display pages
and palette match the stock run in all three updates.

Keep the existing steering traversal. The census covers steering
queries. Measure direct queries separately before changing their
traversal.

### Coalesced visible runs in the effect raster

The trial groups consecutive visible tiles into runs shared by a
whole tile row. It clips the runs at the screen and map edges and
reverses source rows only when they have visible pixels. The
sanitized raster fixture checks the bound on visibility reads and
matches a pixel oracle with a 256-pixel crop across alternating
visible tiles, both edges and a reversed frame. The existing mapped
and unmodified raster checks pass.

The current Protoss 10 Melee trial matches frame, state, route
counters, both Mode 4 pages and palette at all 32 samples. Both
static RAM endpoints match. The earlier mean stays at 757,329
guest cycles. The late mean falls from 1,350,466 to 1,350,404,
and the raster itself falls from 44,619 to 44,544 cycles. The
visible portion of each effect still requires the same composition.
Keep the trial local.

The local attribution SHA-256 is
`61438e1f78b36c7b72c503917ad4b0ca7ef5ef5cdeb8daf15c18eaff08b94aa2`.

### Reusing actor admission during the draw

The trial passes the depth queue's body or charge-only admission
to the unit draw. The ordinary draw entry points still compute
their own admission. The depth keys, attack-image order and
mining-effect order use the existing code. The trial fits the same
Protoss 10 static RAM endpoints. The sanitizer unit-edge fixture
passes on both physical pages, all four battlefield edges and
the hidden-enemy cut.

The control and candidate match frames, world state, route
counters, both video pages and palettes at all 42 cuts in two
600-update neutral Melee windows. All 32 separate attribution
samples also match. At updates 11,385–11,400, the average
body-crop calls fall from 4.31 to 2.31 and the depth painter's
mean cost falls from 389,870 to 384,575 guest cycles.

| Window | Mean guest cycles, control → trial | p95, control → trial |
| --- | ---: | ---: |
| 8,401–9,000 | 766,169 → 766,521 | 904,103 → 903,218 |
| 10,801–11,400 | 1,040,170 → 1,038,072 | 1,350,192 → 1,356,396 |

The late mean falls by 0.20%, and the late p95 rises by 0.46%.
There is little gain at the full-loop level. Keep admission reuse
as a local trial. The 30 fps target still requires savings in
larger shared costs.

| Local timing receipt | SHA-256 |
| --- | --- |
| 1,200-update control | `c26cc62fc6375f3de65fe4afa1bd26ba84a2f7f1a15dfeeb32bc6d156e9cf5c0` |
| 1,200-update admission trial | `443bcd3b5ef715b1d481c7b8a5d42b002ad8b725029ddb5f149264c8569ee30e` |
| Admission-trial attribution | `5c6ec19df684bd7e884bcedd263382ecadfed56d5f8f5803680b16af2fc123e0` |

## Normal cartridge integration candidate

The four normal build recipes enable the same existing lobby mode
tested in the preview. The proposed build combines it with the
batched target index. All four memory checks pass, with the same
IWRAM endpoints as the standalone preview. The current campaign
opening checks, ordinary skirmish save continuations, cross-version
load and rejection cases, and Random seat checks reproduce the earlier
receipts. The normal Terran candidate reproduces the active Melee
victory through a fresh pause load and a third-process result readback.

The current opening fixture passes all 234 configurations under
ASan and UBSan on each of the four profile vectors. It also checks
CPU hostility and target choice in both match types, Random seats,
match-word rejection, and preservation of diplomacy after SRAM load
and match restart.
