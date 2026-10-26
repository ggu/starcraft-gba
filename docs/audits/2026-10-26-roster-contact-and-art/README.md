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
