# Direct firing corridors (2026-10-28)

This is a bounded partial improvement to imported-race combat movement. When
the ordinary route to a target center fails, a qualified product may scan up
to a 33-by-33 tile box for a walkable point in the weapon's exact range band,
retain at most eight nearest candidates, and check at most eight normal
movement corridors. A small derived cache stores one keyed point and rechecks
its walkability and corridor from each new origin. It does not use route-field
slots, add unit/save fields, or start a second map-wide flood.

The candidate only helps when a direct corridor to a firing point is clear.
If all retained corridors are blocked, combat keeps the existing failed-center
retry behavior. Obstacle-only firing routes remain stalled; this change does
not complete general combat approach navigation. Incremental routing through
obstacles remains open follow-up work in PR125.

The opt-in is restricted to the four delivered Terran, Zerg Z6, Protoss P7,
and Showcase products. The default original-art target and legacy/probe
products keep their existing behavior. Verification for this staged change
covers the actual-engine combat hook with the feature on and off, firing-band
and cache-key boundaries, origin revalidation, topology/load invalidation,
melee and explicit-MOVE exclusions, the blocked-corridor retry, default build
and host tests, and dependency-free Make flag expansion. ARM memory, timing,
save-continuation, and release qualification are separate gates and are not
claimed here.
