# Stock roster and live sprite recheck

This follow-up checks the canonical Terran, Zerg Z4 and Protoss P3 builds,
rather than assuming that an all-options atlas establishes coverage in each
ROM. Reference facts remain pinned to BWAPI commit
`d727fed68558c506163048ea889131d8cbb33915`.

The independent numeric comparison covers all 228 raw unit identities and
132 weapon records: health, shields, armor, size, costs, build time, supply,
weapon identity/hit count, damage, upgrade increment, damage factor/class,
cooldown, ranges and nominal Q10 speed. The earlier
[runtime audit](../2026-10-26-stock-art-runtime/README.md) describes which
production rules and specialized behaviors consume those facts.

## Defense attack cadence

Photon Cannon, Missile Turret, Spore Colony and Sunken Colony already use
their reference damage and half-scale range. Their reference cooldowns
convert to 28, 19, 19 and 40 runtime updates respectively. The defense loops
decremented the timer and skipped the firing check even when it reached
zero, delaying every subsequent shot by one update. Allowing the ordinary
target check on that zero transition restores those converted intervals.
The same correction applies to the full scan and optimized target-list
paths. Stored timers, attack art sampling and save layouts are unchanged.

## Ground contact

The expanded actor profile could prevent a worker from attacking an
Ultralisk. Its source-derived collision spacing is ten runtime pixels, while
the center-based SCV attack check allowed only six. An ordinary direct attack
order stopped outside weapon reach indefinitely.

The correction keeps source weapon facts unchanged. Ground melee with no
minimum range and at most 32 source pixels of reach can reach the center
spacing already reserved by the mover.
Ranged weapons, minimum-range weapons, aircraft and building targets retain
their existing checks. This is a bounded accommodation for the GBA mover,
not an implementation of native collision-edge range geometry.

## Construction and rendering scope

The new per-canonical-profile census checks the runtime enum-to-sprite
resolver using each profile's actual compile flags and include order. It
found four completed-building gaps hidden by the all-options atlas:
Control Tower (115) and Protoss Beacon (196) had no resolved art; Queen's
Nest (138) and Ultralisk Cavern (140) used generic art instead of the selected
ZP frames. Their frames were present in the ZP building bank, but the
completed-building resolver did not consult it before the generic atlas. The
resolver now prefers the selected building bank in that profile. The new
fixture checks source pixels and vital-bar placement for its building rows.

Prepared canonical builds enable source construction art. The existing
positive-pixel and layer-composition fixture covers all 50 supported source
construction identities: 18 Terran, 16 Zerg and 16 Protoss. Photon Cannon
checks cover idle, firing, return to idle, construction, power loss and
partial terrain-cache redraws.

Construction stages still use build-progress transitions and a converted
source visual clock. Native worker scripts, finish-order signals, later
Protoss warp states and dynamic gas-overlay placement remain incomplete.
Movement still uses adapted navigation, without native acceleration, turning
or movement upgrades; generic combat retains the limitations recorded in
the earlier audit. Numeric equality does not establish complete StarCraft
behavior or physical-GBA performance.

All verification is local. GitHub Actions remains disabled/manual-only;
ROMs, generated art and private evidence are excluded from the repository.

## Verification

Focused checks pass with the prepared private inputs:

```sh
make test-sc64-stock-profiles-facts SC64_STOCK_FACTS=.local/build/stock-art-runtime/reference.json
make test-sc64-stock-motion test-sc64-stock-motion-runtime test-sc64-stock-combat-runtime
make test-sc64-building-stock test-sc64-scv-contact test-sc64-scv-contact-zp
make test-sc64-zp-defense-targets test-sc64-construction-source test-sc64-static-dirty-rows-game
python3 -m unittest tests.test_sc64_photon_cannon_renderer
```

The new melee fixture covers SCV, Drone, Probe and Zealot direct orders against
an Ultralisk, reciprocal contact, the exact 10/11-pixel attack boundary and
Siege Tank minimum range. The defense fixture tests all four defenses through
both the campaign-list and skirmish full-scan dispatchers, including the
absence of early shots and the exact next-shot update.

Fresh ARM links of the canonical race ROMs and Showcase retain the existing
RAM guards and 32 MiB ROM limit:

| ROM | IWRAM end | BSS end |
| --- | --- | --- |
| Terran | `0x03004f10` | `0x0203ddd4` |
| Zerg Z4 | `0x03004e70` | `0x0203c410` |
| Protoss P3 | `0x03004c70` | `0x02039e00` |
| Showcase | `0x030049e8` | `0x0203dee8` |

These builds do not promote a campaign chapter or add a new full-mission
ARM timing or physical-GBA qualification.

The corrected combat cadence changes the outcome of two older test-controller
strategies. Z3's controller now reacts to nearby Chrysalis threats every 120
frames with ordinary local box selection and attack-move orders, instead of
waiting for its 1,800-frame strategic command. It wins both the carried Z4
profile and legacy Z3 profile at frame 51,360. P3's controller begins its
ordinary assault at frame 10,000 with 14 army units, before the old defensive
buildup suffers attrition; its fresh canonical win is at frame 47,626. Neither
controller edits simulation state, resources, caps or mission conditions.

P1's controller now uses a bounded unit drag to select the moving Probe for
its existing Pylon attempt, avoiding the Nexus click hitbox. Two fresh runs
win at frame 9,303 with Fenix alive. This route completes the ordinary Probe
queue; it does not report a completed Pylon/Gateway reinforcement, so no new
combat-force resupply is claimed. P2's carried controller still wins at frame
31,623 with Fenix alive. These controller changes preserve strict native
mission outcome checks.

The default build and suite pass (488 Python tests plus host fixtures), as
does the full ten-mission Terran host-controller target. The renderer module
passes all four private-input tests without skips after its partial-input
guards were added. ROM preparation also now retains packed sprite metadata
when preparing shared/staged banks, preventing another profile's preparation
from invalidating the canonical Zerg bank. Private logs are in
`.local/build/current-roster-*.log`.

The final combined `make test-unified test-canonical-render
test-race-campaigns test-sc64-showcase` run passes. This includes the carried
campaign controllers, all three race frontend matrices (54 launches total),
702 startup/save configurations, and Showcase scene/save-isolation checks.
