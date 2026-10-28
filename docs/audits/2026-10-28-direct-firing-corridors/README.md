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
revalidation, topology/load invalidation, Zergling-versus-unit and explicit-MOVE exclusions,
the blocked-corridor retry, default build and host tests, and dependency-free
Make flag expansion. The release checks below establish the documented
bounded scope; general combat navigation and full-campaign performance remain
open.

The hook uses the effective combat range after the existing building-target
allowance. It skips ranges below 32 pixels and flying movers. This is not a
blanket melee classification: for example, the Probe's native 32-pixel weapon
becomes 16 world pixels plus the existing 18-pixel building allowance and can
use the helper against a building. The fixture's unchanged melee case is a
Zergling attacking another unit. Damage and firing eligibility still use the
ordinary combat checks.

## Release qualification at runtime `622face`

The preceding direct-only revision `40f8068` did not solve the actual Z7
cliff regression: the attacker still stalled through 300 ticks. The final
one-corner revision solves that case without adding another flood. On the
prepared Z7 map, the baseline's center path and direct corridor both fail,
and the feature-off Hydra remains stationary through 300 ticks. With this
feature it fires at tick 48, within its 64-world-pixel weapon range, applying
the expected seven damage. Every tick preserves ordinary movement limits.
This targeted fixture does not establish a complete mission win.

Four full normal-product ARM links pass the unchanged memory guards:

| Product | IWRAM end | BSS end |
| --- | --- | --- |
| Terran | `0x03004df0` | `0x0203708c` |
| Zerg Z6 | `0x03004f40` | `0x020370f8` |
| Protoss P7 | `0x03004b70` | `0x020382b8` |
| Showcase | `0x03004fd0` | `0x0203df38` |

The derived cache is 14 bytes, rounded to 16 bytes of linked BSS growth.
Showcase retains 48 bytes before the existing IWRAM guard and 200 bytes
before its BSS guard. No reserve or guard was reduced, and all 14 ordinary
route fields retain their capacity. Helpers remain in ROM; the small IWRAM
delta results from GCC outlining the existing HOT corridor routine.

Actual-game ON and OFF host fixtures pass in all three race products. The
72 ordinary menu/skirmish/save continuations cover both maps, all human
races, 1–3 AIs and focused three-AI compositions. The focused game fixture
explicitly warms the new cache and verifies load invalidation. Three actual
ARM save tests pass in-place and in a fresh isolated process, using the same
SRAM bytes and matching the host continuation from frame 9001 to 9901.
The SRAM harness's `CACHE_*` markers concern VFX duration caching, not this
new cache. Every owned emulator was terminated and waited on.

Menu-started Blood Bath replays against three mixed AIs match all 14 prior
semantic state windows. Overall cycle-equivalent rates are 28.73 fps for
Terran, 28.66 for Z6 and 28.72 for P7; the late P7 window remains 24.66 fps.
These are fixed-camera unattended defeats, not campaign wins, hardware
measurements or steady 30-fps coverage.

Actual ARM microprobes explicitly invalidate the new cache for cold queries:

| Probe (cycles) | Showcase | Z6 |
| --- | ---: | ---: |
| Cold 480-pixel origin / maximum range 210 | 80,589 | 80,589 |
| 1,000 cached queries, mean / maximum | 2,541 / 4,318 | 2,539 / 4,316 |
| Local one-corner case, cold | 35,615 | 35,287 |
| Obstructed-wall miss | 93,911 | 93,052 |
| First near-target combat tick | 109,574 | 109,452 |

Pixels above are GBA/world coordinates, at half the native source scale.
Eight alternating target/range keys average 76,625 cycles per miss, with a
maximum around 82,014. Their sequential miss work totals about 613,000
cycles (36.5 ms), before any other frame work. The single-entry cache and
cold-search bursts therefore remain a frame-time limitation; a bounded
individual query does not establish a frame-budget guarantee. General
incremental routing and scheduling remain follow-up work rather than a
claim that all requested near-30-fps coverage is finished.

An additional strict ordinary-input Z7 controller run at this runtime ends
at its 20,000-host-tick limit without a win: the last snapshot has one worker,
seven combat units and 54 enemy buildings. It does not qualify or enable Z7.
The delivered mission scope remains T10/Z6/P7.

Local default build and tests pass (649 Python tests, 20 optional skips, plus
the native checks), as do the source-boundary and whitespace checks. All
prepared-profile compiles use captured direct compiler arguments with current
authored paths and existing ignored prepared inputs; no imported Make
recipes, asset extraction, Actions, GDB or SD writes were used. Source hashes
were frozen for the runtime checks. The SRAM runner's final whole-tree
cleanliness assertion noticed a concurrent audit edit and a zero-byte
compiler `.su` artifact; its independent postcheck confirms unchanged runtime
hashes, matching continuations, valid guards and terminal owned processes.

Private evidence is under `.local/build/direct-corridors-release-v2/` and
`.local/build/direct-corridors-fresh-sram-v2/`. Older `40f8068` evidence remains
separate. No assets, generated game data, ROMs, saves, captures, native
decompilation or private harnesses are included in the source repository.
