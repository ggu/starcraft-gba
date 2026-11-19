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

The old renderer flipped all 192 Wraith/Tank pixels on unit slot swap and
drew the Tank over all 459 pixels occupied by the southern Barracks. Use
`SC64_DISABLE_WORLD_DEPTH` to replay the former pass order from the same
source. The current canonical Terran framebuffer test is
`build/test-sc64-terran-full-frame`. Add campaign and skirmish replay cases
for picking the topmost actor and for dynamic layers such as workers against
resources or Interceptors around Carriers.

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
The result is scoped to the emulator replay. Busy skirmishes and hardware
timing still need this comparison.
