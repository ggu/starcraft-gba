# Current roster and construction recheck

The numeric and sprite recheck uses main merge `2d9d6f8` (PR 35), which
includes the stock-combat and art fixes from PR 34. All checks run locally;
Actions remains disabled and manual-only.

The independent pinned BWAPI comparison passes for all 228 raw unit IDs and
132 weapon records, plus the 50-entry constructible-building census. The
current compiled helper comparison covers 275 profile rows, including 271
canonical HP/shield/armor rows and their available weapon and nominal-motion
helpers. See the [stock audit](../2026-10-26-stock-profiles-and-art/README.md)
for fields, scaling and exceptions. These are PC reference defaults, not a
claim that every N64 scenario override or native engine behavior is reproduced.

All four private sprite/Cannon tests pass without skips. They exercise the
actual canonical Terran, Zerg Z4 and Protoss P4 Storm flags and sprite banks,
including each mapped visible unit/building enum and selected imported raws.
The intentionally invisible map revealer is excluded. Cannon checks include
completed idle, firing, return to idle, power loss, construction and partial
terrain-cache redraws. The source-art fixture also checks composite pixels for
all 50 construction identities (18 Terran, 16 Zerg and 16 Protoss).

Focused stock motion/combat, building capacity and contact checks pass too.
The three campaign ROMs and Showcase rebuild and pass header, checksum,
32-MiB size and unchanged RAM reserve checks. The local release manifest
records their exact hashes; no ROM, generated header or source-game pixels
are committed.

Source construction art is enabled in each prepared canonical race build.
The live worker-order check exposed a Protoss phase mismatch: the old importer
used each completed structure's `WarpIn` script during the entire build timer.
For a Pylon, that script and its linked child select one static frame.

Pinned OpenBW revision `4b046d5f65302b10cb0a745f0fecd37ec85b20a8`, in
`order_ProtossBuild`, instead replaces the initial structure image with
`IMAGEID_Warp_Anchor` (211). Creating the replacement runs its `Init` script.
`order_IncompleteWarping` retains this anchor during the build timer; the
normal structure's `WarpIn` and texture materialization belong to later order
states. The importer now selects the authenticated anchor's `Init` program
and spawned layers for all 16 Protoss construction identities. No synthetic
animation frames or altered build times are needed.

The ordinary builder-order regression now advances only through `game_update`
after starting construction. It verifies fixed camera coordinates, building
identity, monotonic progress, source-composite pixels, builder disposition and
the completed source sprite through the art and overlay passes. A Depot takes
750 updates with 3 sampled art changes; a Hatchery takes 2,250 with 233; a
Pylon takes 563 with 70. The pixel hash excludes health/shield/progress bars,
so a changing bar cannot stand in for an animated building. The all-50 source
stage census, pause and save/load checks remain alongside this regression.

Stage transitions still use adapted build progress and a converted visual
clock. Native worker animation, finish signals, dynamic gas overlays and
later Protoss warp states remain incomplete. The native Assimilator's
separately attached Vespene Geyser underlay is also unimplemented; it is an
order-side attachment, not an opcode-56 request in the anchor's script.
Generic combat and movement
retain the earlier audit's documented limits; numeric equality does not
establish complete native gameplay or physical-GBA performance.
