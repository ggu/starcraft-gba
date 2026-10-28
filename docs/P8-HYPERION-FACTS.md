# P8 Hero Hyperion source facts

This note records numeric facts needed for the P8-only raw28 actor binding.
It includes no ROM bytes, extracted graphics, generated asset data or game
artwork.

## Native N64 tables

Read-only inspection of the locally verified USA StarCraft 64 ROM (SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`) with the pinned `sc64-maps`
extractor checkout at `09b57f787a247107d1de82b8c5eab2734667ea1d` established
the following BOLT chain:

| Unit | Flingy | Sprite | Image | GRP entry |
| --- | ---: | ---: | ---: | ---: |
| raw12 Battlecruiser | 70 | 225 | 218 | 222 (`00D/0DD`) |
| raw28 Hero Hyperion | 70 | 225 | 218 | 222 (`00D/0DD`) |

Both resolve to the same 20,877-byte compressed GRP, with 17 frames on a
60-by-60 canvas and SHA-256
`2a1f6a67e2ce1b0278f08ab9a391729a731744b39c3571f0f13c81afda281acb`.
The binding therefore reuses the already available raw12 sprite descriptor
while preserving raw28 as a separate logical unit.

The native raw28 unit row gives 850 HP (217600 in 8.8 fixed point), armor 3,
zero shields, Large size, ground/air weapon IDs 23/24 and one hit per attack.
Native weapon rows 23/24 each give damage 30, bonus 3, cooldown 22, range
0..192 source pixels, one projectile/hit and damage type 3. The shared native
flingy70 row gives speed 640, acceleration 27, halt distance 7585 and turn
rate 20. The existing stock combat table has a separate raw28 row selecting
weapons 23/24, and the fixture verifies those selectors resolve. This proves
lookup identity, not full mission combat behavior or P8 readiness.

## Pinned BWAPI geometry

The gameplay pixel box and sight value use the independently pinned PC table,
not an inferred N64 collision box. `starcraft/sc64_siege_source_bounds.h`
documents BWAPI `UnitType.cpp` `unitDimensions` at revision
`d727fed68558c506163048ea889131d8cbb33915`; row 28 is
`{tileWidth=2,tileHeight=2,left=37,up=29,right=37,down=29}`. These are the
current engine's inclusive edge offsets. The same pinned revision's
`sightRangeTiles[28]` value is 11 tiles, or 352 pixels at 32 pixels per tile.
Neither figure is claimed as a native N64 pixel bound.

## Scope

All new logical identity and fact overrides are under `SC64_P8_DATA_ENABLED`.
The canonical non-P8 enum values and mappings are unchanged. This source-only
binding allows the prepared P8 census to resolve raw28, but does not activate
the mission. Renderer qualification, full mission capacity, native AI,
runtime loops, campaign catalog and save behavior remain unqualified.
