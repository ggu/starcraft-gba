# Direct firing corridors (2026-10-28)

This is a bounded partial improvement to imported-race combat movement. When
the ordinary route to a target center fails, a qualified product may scan up
to a 33-by-33 tile box for a walkable point in the weapon's exact range band
and retain at most eight nearest candidates. It checks each candidate's direct
corridor first. If those fail, it can test two axis-aligned one-corner paths
for each candidate within 128 pixels, bounded to 32 additional leg checks. A
small derived cache stores the keyed final firing point and rechecks its
walkability and direct or one-corner path from each new origin. The returned
first-leg waypoint may be outside firing range; the cached final point stays
inside it. The change does not use route-field slots, add unit/save fields, or
start a second map-wide flood. The ordinary route-field capacity is unchanged:
all 14 existing fields retain their original roles and capacity.

The candidate helps when a direct corridor or one of the bounded one-corner
paths to a firing point is clear. If none of those paths work, combat keeps
the existing failed-center retry behavior. Paths requiring more turns or
routing around multiple obstructions remain stalled; this change does not
complete general combat approach navigation. Incremental routing through
obstacles remains open follow-up work in PR125.

The opt-in is restricted to the four delivered Terran, Zerg Z6, Protoss P7,
and Showcase products. The default original-art target and legacy/probe
products keep their existing behavior. Verification for this staged change
covers the actual-engine combat hook with the feature on and off, direct and
one-corner firing paths, firing-band and cache-key boundaries, origin
revalidation, topology/load invalidation, melee and explicit-MOVE exclusions,
the blocked-corridor retry, default build and host tests, and dependency-free
Make flag expansion. ARM memory, timing, save-continuation, and release
qualification are separate gates and are not claimed here.
