# Stock profile and art runtime follow-up

This audit records the runtime follow-up to the [stock profile and sprite
audit](../2026-10-26-stock-profiles-and-art/README.md). Numeric table
comparisons below use the clean local BWAPI reference at pinned commit
`d727fed68558c506163048ea889131d8cbb33915`. No game assets or extracted
proprietary payloads are included.

## Numeric facts and live profile rules

The independent extractor and comparator checked all 228 addressable raw unit
IDs (95 flagged buildings) and 132 weapon rows. HP, shields, armor, size,
mineral/gas cost, build frames, supply, ground/air weapons and hit counts,
weapon damage/bonus/factor/cooldown/ranges/damage class, and Q10 top speed
match the pinned public numeric defaults. The constructible building census
has 50 entries including the Greater Spire morph. The top-speed property is
documented by BWAPI as approximate; matching its table value is not proof of
native movement physics.

The active race rules were independently compared with those facts. The 13
base race-menu trainables match mineral/gas cost, build frames, supply, HP and
shields: SCV, Marine, Firebat, Medic, Drone, Overlord, Zergling, Hydralisk,
Mutalisk, Probe, Zealot, Dragoon and Scout. The expanded Terran production
rules also match those fields for Goliath, Tank Mode, Siege Mode's source
identity, Vulture, Wraith, Dropship, and (in the T4 profile) Ghost. Siege Mode
is a zero-cost in-place transformation rather than a second factory product.
Late Terran Science Vessel and Battlecruiser facts are present, but their
runtime rules intentionally use producer 255 and are not trainable. The mapped
rules for 48 of the 50 constructible building identities also match those
fields. Comsat Station
(raw 107) and Nydus Canal (raw 134) have source construction-art census rows,
but no runtime building enum/rule or current build-menu route.

Imported source actors initialize health, shields and armor from their raw
identity. The building-capacity fixture passes across 12 profile compilations.
Generic combat selects source ground/air weapon facts and applies per-impact
armor and damage class/size modifiers, weapon upgrade bonus, hit multiplicity,
minimum/maximum range and converted cooldown. Runtime fixtures cover Goliath
and Scout damage factors, two Zealot blade hits, Hydralisk explosive damage
against a small shielded target, Siege minimum range/cooldown, and raw-19
Raynor identity and health.

The former +1 HP heal for SIDE workers every 90 frames was non-stock behavior
in imported missions. It is now compiled only outside
`SC64_IMPORTED_MISSION`. A focused `game_update()` regression verifies that
imported SCV, Drone and Probe health is unchanged on frame 90 and that the
original-art SCV still heals by one HP on that frame.

The current Protoss 3 stock-controller trace wins at host tick 60,197 / frame
59,586, with 2,999 input transitions and 59 periodic checkpoints. The earlier
ARM mission proof and 20.0 fps equivalent performance measurement apply to
commit `9a2e281`, before this worker-heal correction; a current-source ARM
mission replay has not yet been run. Shield regeneration remains implemented,
but this audit does not claim complete stock Drone HP recovery behavior.

Shield regeneration is implemented separately in `race_tick_state()` using
the fractional accumulator for the reference 7/256 shield HP per 24 Hz source
frame, converted to 7/320 per 30 Hz runtime update. The P7 Battery path retains
its separate passive recharge adapter.

## Runtime fidelity limits

Stock nominal movement rates are quantized to Q10 and converted to 0.4 runtime
pixels per source pixel/frame (half spatial scale times 24/30 frame rate).
Fractional movement is distributed by deterministic tick phase without new
actor or save fields. The mover remains an adaptation: it has no native
acceleration, turning, speed upgrades, unit script, collision footprint, or
flight physics. Ground navigation uses the runtime's grid pathing; aircraft
move directly toward their target.

Combat facts do not provide native projectile or script behavior. Generic
attacks resolve immediately at approximated center ranges. Firebat uses the
legacy area adapter, Reaver uses a Scarab fallback, P7 Interceptor attacks
remain disabled pending projectile lifecycle integration, and Archon splash
and mission abilities have separate adapters. Native splash geometry,
projectile travel and launch timing are not represented by the fact table.

The unit/building tables and these focused fixtures do not establish complete
native StarCraft equivalence. They validate the selected stock numeric facts
and bounded runtime paths only.

## Sprite and construction coverage

The race renderer now resolves the nine late-building raw IDs 108, 117, 120,
123, 127, 116, 118, 171 and 172 through their source building frames. The
full Terran roster fixture checks 43 visible unit enum identities, 53 regular
building identities and eight separately rendered T4 props for opaque source
art. Raw 101 is intentionally invisible. Completed pixels and vital-bar
placement are checked for each of the nine corrected building identities. The
source construction-art test checks composite source pixels at every stage for
all 50 census identities (18 Terran, 16 Zerg and 16 Protoss). Photon Cannon
raw 162 retains separately verified idle, firing and construction frames.

Gas-overlay placement and native construction completion, worker animations
and post-timer effects remain incomplete. Source-art coverage does not make
raw 107 or 134 buildable through the runtime rules.

## Local checks

The focused checks completed for this audit were:

- Full independent raw-unit/weapon fact comparison and the source construction
  census.
- `make test-sc64-stock-profiles`, `make test-sc64-building-stock`,
  `make test-sc64-stock-combat-runtime`, `make test-sc64-stock-motion`, and
  `make test-sc64-stock-motion-runtime`.
- `make test-sc64-worker-heal`, including both imported and original-art
  `game_update()` variants.

The independent table comparison and static helper tests are asset-free;
runtime fixtures use prepared ignored generated headers where required.
GitHub Actions were not used.

The final local regression also passed `./dev build`, `./dev test` (467 Python
tests plus host fixtures), `make unified test-unified`, the canonical render
tests, Showcase tests, and `make test-race-campaigns`. Across three canonical
profiles, 54 menu launches and 702 startup/save configurations passed. The
separate `./dev sc64 test --profile terran` run completes all ten actual-win
mission controllers after the worker-heal correction. A fresh input-only P3
host replay also wins at tick 60197 / frame 59586, and the updated strict
replay helper passes its 21 focused tests.

All three canonical ARM ROMs and Showcase were rebuilt. IWRAM/BSS ends are
Terran `0x03004f10/0x0203ddd4`, Zerg `0x03004f38/0x02039c58`, Protoss
`0x03004fc8/0x02039e00`, and Showcase `0x030049e8/0x0203dee8`. Existing RAM
and save guards are unchanged. Current-source physical-GBA and full-mission
ARM timing qualification remain separate work. Private logs and reference
facts are under `.local/build/stock-art-runtime/`.
