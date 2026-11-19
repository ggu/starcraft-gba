# World depth regression cases

The USA N64 renderer sorts visible sprites back to front by elevation. For
elevations 0 through 4 it sorts by ascending world Y within each elevation.
Higher elevations ignore Y. A turret flag and a sprite ID break remaining
ties. The target picker calls the same depth helper.

The layer values come from the 228-byte elevation column at offset 4068 in
BOLT section `00C/001`. The section SHA-256 is
`54356d7585c66cd463346996444fd208336eaacf6335d30eb2688aa73c9cb2df`.
The USA ROM SHA-256 is
`e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`.
The retained Wasteland observer capture at step 7121 has RAM SHA-256
`d30d79bcd796c4c535c2667875da309a045bfe482bba9720ba01eb5f92951b9b`.
Its helper at `0x80122750` feeds the render queue at `0x80045f48`, which
drains back to front at `0x80120460`. The depth helper is also present at
ROM offset `0xE6440` (64 bytes, SHA-256
`f9a75923b37bbc55602345e4b1d20c0449c4ac922d8548afd203089c010cfaf1`).
In native gameplay footage the
Overlords cover the Hatchery rim, and a Probe covers the southeast Nexus lip.

The GBA Mode 4 painter uses those elevation and Y rules for units, buildings
and resources. Composite turrets remain with their hull. GBA actor kind and
slot resolve exact ties as a deterministic approximation of the native
sprite-ID tie. Native depth changes for special states still need individual
checks. The change covers world actors in the shared campaign, skirmish and
Showcase renderer. Target picking, free-standing effects, HUD and the
experimental tile/OAM renderer need separate coverage.

## Harness cases

For each case, render five frames with one fixed camera and the same
terrain, palette and animation tick. Draw the empty scene, each actor by
itself and the pair in both creation orders. Restrict the overlap mask to
pixels both isolated actors change to different palette indices. Require
a nonempty mask and compare every masked pixel in both paired frames against
the expected foreground actor. The Terran host frame coverage fixture
executes the first four cases. The expected counts are diagnostic for its
pinned sprites and are allowed to change when the private art is regenerated.

| ID | Setup | Foreground | Measured overlap |
| --- | --- | --- | --- |
| `air-over-ground` | Tank and Wraith at (96,50), layers 4 and 18 | Wraith in both unit slot orders | 192/192 |
| `ground-by-y` | Tank at (96,50) and Goliath at (96,56), both layer 4 | Goliath in both unit slot orders | 115/115 |
| `building-over-unit` | Tank at (96,50), Barracks at (96,58), both layer 4 | Barracks | 459/459 |
| `unit-over-building` | Barracks at (96,43), Tank at (96,50), both layer 4 | Tank | 380/380 |
| `air-tiers-ignore-y` | Move a Wraith north and south across a Carrier or Overlord, layers 18 and 16 | Wraith during overlap | Future replay |
| `camera-and-hud` | Scroll an overlapping pair past screen rows 12 and 119 while flipping framebuffer pages | Foreground stays stable, HUD rows stay clear of actor pixels | Future paired replay |
| `selected-overlap` | Select the Overlord above a Hatchery and the Tank behind a Barracks | Overlord ring over the Hatchery, Overlord body over its own ring, Barracks over the Tank ring | 15, 14 and 29 distinguishable pixels |

The old renderer flipped all 192 Wraith/Tank pixels on unit slot swap and
drew the Tank over all 459 pixels occupied by the southern Barracks. Use
`SC64_DISABLE_WORLD_DEPTH` to replay the former pass order from the same
source. The current canonical Terran framebuffer test is
`build/test-sc64-terran-full-frame`. Add campaign and skirmish replay cases
for picking the topmost actor and for dynamic layers such as workers against
resources or Interceptors around Carriers.

In native Challenger melee, a selected Overlord moves across the Hatchery.
The cursor is away from the overlap. At observer step 3920 the Overlord's
lower ring arc covers two pixels of the Hatchery lip that were opaque at
steps 3612 and 3740. The native capture SHA-256 is
`765ca89ac1f501f021ecde36762ca79af2f0e3953cdae00d54647900b84e5926`.
The GBA depth painter now draws a selected unit's ring immediately before
its body. Foreground actors can cover the ring. The legacy path still draws
the rings in a separate pass. The PC implementation also draws the ring
within its selected sprite's turn in the sorted pass.
[OpenBW sprite renderer](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/ui/ui.h)

The terrain cache now holds background and fog. Profile counters attribute
the sorted actor pass, including buildings, to `sc64_unit_draw_cycles`.
For before/after comparisons sum building and unit draw counters and compare
the combined cost with background and full-update timings.

## Local ARM performance

An ordinary-input Protoss 10 ROM replay starts P7 and runs 9,020 tape
steps. The comparison covers 958 advancing updates in the final 1,000
tape steps. Both builds had the same frame progression, camera and route
counters. Eleven sampled world states, fog masks, and actor arrays
matched. One sample had changed indexed frame pixels.

| Arm cycles per advancing update | Former renderer | Depth painter | Change |
| --- | ---: | ---: | ---: |
| Mean | 1,877,754 | 1,799,786 | -4.15% |
| 95th percentile | 2,176,419 | 2,098,029 | -3.60% |
| Peak | 2,558,598 | 2,464,640 | -3.67% |

The tape SHA-256 is
`44e68fec4bd45ef8e8df86144dcfe877dc99592f2adf99107351cfc1bb0ed3a6`.
The former-renderer receipt SHA-256 is
`b21cc587f2f6fec72aeba1f5eac38e977a2426bfc01c5f10db49c08a8ffd5849`.
The depth-painter receipt SHA-256 is
`0eec46ab8ea1510e12095aa8485ffc4d2dbdd941f4b26354ff4be7b5758de410`.
The result is scoped to the emulator replay. Hardware timing remains open.

## Three-AI skirmish follow-up

The Protoss 10 ROM launches Blood Bath through the Custom Game menu with
a Zerg player and Zerg, Terran and Protoss CPUs. The last 1,000 updates
end at frame 17,301. Twenty sampled world states, actor arrays, resource
rows, and fog masks match in all four builds.

| P10 renderer and steering combination | Mean cycles | p95 | Peak |
| --- | ---: | ---: | ---: |
| Release `f924b9df` | 1,001,597 | 1,407,402 | 2,193,638 |
| Early steering only | 999,806 | 1,403,992 | 2,195,675 |
| World depth only | 1,028,478 | 1,427,845 | 2,211,542 |
| Early steering and world depth | 1,026,648 | 1,426,164 | 2,209,088 |

World depth increases mean cycles by 2.68% on this skirmish. The
combined build takes 25,051 more cycles on average than the release.
The combined build still saves 6.28% mean in the P7 replay with the
same sampled world and one changed frame pixel hash. Keep the paired
skirmish run in the release gate. The main cost increase is the actor
pass, which now paints resources and buildings on every frame. A
trial that rejects hidden actors before sorting adds about 520 mean
cycles to this skirmish, so it stays out of the candidate.

The stock, depth-only and combined skirmish receipt SHA-256 values are
`7e18c69916088d3a9c07fe3abfef68606d9e95323a6090fe55105fc2c1431385`,
`af4bf37336699599bea1da3be7f05c69c6cc0ba7c056ad558951b9255565ab13`, and
`51ef3c3342ba4af6b2a30d89ded095f88b070f64da7aba8ac09fc11525feb76e`.
The combined P7 receipt SHA-256 is
`381c66228c11c7e30c296ee5a4956f5cc278e5a0c2c8975b122a895ea5a0cbe9`.

## Packed Hatchery draw

On P10, the three-AI camera repeatedly draws a visible completed Hatchery
after the terrain cache restore. The P10 build now packs just this frame
using the existing masked-word blitter. Other frame pointers retain the
byte-sprite path. The normal build selects the packed frame when its
existing sprite compositor flags are present. The asset is generated
locally from the prepared sprite bank.

The composed steering and depth builds use the same frozen inputs in
both arms. Twenty Blood Bath cuts and eleven P7 cuts match the complete
indexed framebuffer pages, palette and sampled gameplay. The P10
framebuffer and packed geometry sanitizer checks also pass. The linked
ARM builds use the same IWRAM and EWRAM. The frame adds about 23 KiB of
linked ROM payload within the existing cartridge size.

| ARM cycles | Byte Hatchery | Packed Hatchery |
| --- | ---: | ---: |
| Blood Bath mean | 1,026,648 | 1,003,303 |
| Blood Bath p95 | 1,426,164 | 1,404,322 |
| Blood Bath peak | 2,209,088 | 2,182,928 |
| P7 mean | 1,766,436 | 1,766,498 |
| P7 p95 | 2,066,260 | 2,066,456 |
| P7 peak | 2,433,880 | 2,433,880 |

Compared with the original release in the preceding table, the combined
Blood Bath build is now 0.17% slower on mean cycles and 0.22% faster at p95.
P7 stays within 62 mean cycles of the faster depth-and-steering build.
Use `P10_HATCHERY_WORDS=0` to repeat the paired comparison.

The integrated Blood Bath receipt SHA-256 is
`c4a8090ef2854b13110591fc44a11c6ec541560fcfba797ab1fd10a6fd70cb0c`.
The integrated P7 receipt SHA-256 is
`de9aa79d7b953b32accf6d88b1ac14d0d1b7f6b4585a8c644b279ab1cd9ad350`.

## Release composition

Composed source `063c54fb` applies the renderer and packed Hatchery to
release `2dec1556`, including the P10 steering and standalone P6 save
changes. All four ARM builds pass ROM, header, SRAM signature and memory
checks. Each image is byte-identical to its previously qualified renderer
image, including the packed P10 run above.

Thirteen local renderer checks pass. They cover framebuffer and HUD
rendering for the three race products, dirty-row transitions, packed
Hatchery clipping, and Showcase with its normal, packed and indexed
sprite paths.

The exact three race images complete 702 ordinary-input custom-game
launches. The matrix covers both maps, each human race, and every ordered
one-, two- and three-CPU race lineup. Each case checks the starting roster
and plays 120 updates. Across 54 sampled lineups the player saves through
the pause menu, reloads in a fresh emulator, and reproduces the following
60 updates. This is early skirmish and save coverage. Longer campaigns,
endgame skirmishes and hardware timing have their own checks.

The renderer receipt SHA-256 is
`e68e0fae1c284e6a44d8694e0944636823fb281050080d39e032c83b63b19098`.
The skirmish receipt SHA-256 is
`99571add436acc888849844cbbb50d04f2a4c6078d3866aa19c2dadc40082e0c`.

The selected-ring follow-up also passes all four normal ARM links and
header checks. Against the exact `9dfba082` P10 ROM, the 958-advancing-
update P7 window matches all 1,000 timeline rows and 11 sampled world,
palette and framebuffer states. The release mean of 1,766,498 cycles
falls to 1,760,602, p95 from 2,066,456 to 2,059,279, and peak from
2,433,880 to 2,427,848. The isolated overlap fixture exercises the
changed ring pixels. The ordinary P7 sample checks the wider scene.

The selected-ring P7 receipt SHA-256 is
`c909430793b05122a7ccd6e7c5180e2dac03e22d70d0eb2a2cfdf110b8cd4745`.
