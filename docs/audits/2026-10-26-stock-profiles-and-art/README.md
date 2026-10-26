# Stock profile and sprite follow-up

This implements corrections identified by the [earlier unit audit](../2026-10-25-unit-specs/README.md).
Imported race profiles now read ordinary weapon facts and nominal movement
rates from a raw-ID stock table. The original-art demo remains independent of
ROM-derived inputs. This is not a claim of complete native StarCraft behavior.

## Numeric facts and runtime use

`starcraft/sc64_stock_profiles.h` contains independently extracted numeric
defaults from the clean pinned BWAPI revision
`d727fed68558c506163048ea889131d8cbb33915`. The comparison covers all 228
addressable raw unit IDs (including 95 building IDs) and all 132 weapon records.
The fields include HP, shields, armor, size, costs, build time, supply, weapon
IDs, hit counts, damage factor, damage/upgrade bonus, cooldown, minimum/maximum
range and Q10 nominal top speed. These are public PC reference defaults;
they do not establish that every Nintendo 64 value is identical.

The imported runtime now uses weapon-specific upgrade increments, Explosive
and Concussive size modifiers after armor, per-impact armor for multi-impact
attacks, and the Siege Tank's minimum range. Raw-19 Raynor retains the legacy
objective identity while using the Vulture's 300 HP and actual weapon/movement
facts. Generic spawns use the exact building HP helper and independent shield
capacity. Stock movement uses half spatial scale and 24/30 simulation timing,
fractional rates without new actor/save fields, and normalized diagonal axes.

The rule audit also fixes Siege damage (50 to 70), SCV attack range, Drone and
Probe range/cooldown, Hunter Killer range, Fenix cooldown and Raynor variants.
Production-capable ordinary units retain reference costs/time/supply. Mission
actors, untrainable heroes, mode conversions and powerups keep their explicit
nonproduction/UI policies; they are not purchasable stock units.

The comparison probe was refreshed on 2026-10-26 to match the current helpers.
It reads nominal movement rates and their 1024-phase cardinal/diagonal totals
from `sc64_stock_motion_step`, and reports raw-ID stock attack helper fields
separately from legacy race-rule fallbacks. It compares weapon ID, base damage,
upgrade bonus, damage factor, damage class, unit hit count, per-impact count,
cooldown and both range bounds. Its boundary checks reject below minimum and
above maximum range. This corrected earlier report labels that treated Goliath
and Scout's `1 hit × 2 weapon factor` as missing damage, represented motion by
hardcoded one/two-pixel steps, and described Siege minimum range as absent.

The probe now passes over 275 compiled unit/profile rows (55 distinct raw IDs
across Terran Full, Z3, P3, P6, P7, Z6 and Z7 helper profiles): all 271
canonical rows match source HP, shields and armor; all available stock attack
helper rows match their pinned weapon records; and every mobile row's integrated
nominal fixed-point rate matches the documented scaling assumption. Twenty-five
source-immobile entries and two special lifecycle entries are classified
separately. The 228-ID/132-weapon fact extraction checks the full public numeric
table, including uncompiled IDs. These results qualify table/helper values, not
all actor movement or the complete combat system.

Movement uses the source top-speed number with the assumed half-size spatial
scale and 24-source-frame/30-runtime-update conversion, then normalizes
diagonal steps. The runtime still lacks native acceleration, turn rate, speed
research and full movement IScript behavior; terrain, collisions and order
gating also alter travel. The Siege helper reports 70 damage, +5 upgrade bonus,
75-frame cooldown and 64/384 source-pixel bounds; its actual tick dead zone is
covered by the engine combat fixture. The comparison is not a native N64
equivalence test.

Building capacities are also checked exhaustively across 12 compilation
variants, including mapped scenario props that occupy Building slots. This
caught the Robotics Facility's missing 500-shield capacity and extended
Protoss buildings bypassing the generic shield/armor helpers; regeneration,
construction and save validation now receive their source capacities too.

Important remaining behavior limits: navigation is still an eight-direction
GBA adaptation without native acceleration/turning or movement upgrades;
generic range uses centers/approximated building edges; HP remains integer
with a one-point minimum rather than native fractional HP. Generic hit counts
resolve maximum unit hits and weapon factors as separate impacts; native
IScript timing, Firebat flame geometry, splash, projectile travel, upgrade
families and specialist adapters require separate qualification. This change
does not establish full campaign completion or a new 30-fps result.

Firebat retains its legacy area adapter: the source maximum of three spatial
flame hits is not applied as three guaranteed hits on one target. Its native
flame geometry and damage distribution remain to be implemented. Save codecs
retain their layouts while supporting exact 100000-HP Beacon values and old
10000-HP records; derived codecs reconstruct unit HP from source identity.
Legacy saves can retain their historical stored spawn capacities. Start a new
mission to apply the revised spawn HP and armor defaults consistently.
Ground contact reach has a six-runtime-pixel floor because the mover reserves
that center separation; otherwise the stock SCV's ten-source-pixel range could
never reach a unit. The stock range itself remains ten in the fact table.

Reproduce the independent fact comparison using a clean local reference:

```sh
python3 tools/sc64_unit_spec_reference.py --reference /path/to/bwapi --output .local/unit-specs/reference.json
make test-sc64-stock-profiles-facts SC64_STOCK_FACTS=.local/unit-specs/reference.json
make test-sc64-stock-motion test-sc64-stock-motion-runtime test-sc64-stock-combat-runtime
make test-sc64-building-stock test-sc64-scv-contact
```

The extractor checks the pinned reference and hashes its inputs; nothing is
downloaded by these commands. Numeric fact tables are source; ROMs and imported
images remain private and ignored.

## Sprite and construction changes

The importer follows source Init frame selection and composites source image
underlays/overlays. It rejects empty/invalid selected frames. Missing Medic,
Siege Mode tank, Spider Mine, Kerrigan, Refinery and Factory mappings are now
included. Hero rendering uses the actual imported hero frame instead of a
forced Ghost/Vulture substitution.

Photon Cannon raw ID 162 uses image 183 plus underlay image 184. Its completed
idle sprite is nonempty; it is now rendered dynamically so the static terrain
cache cannot freeze its firing frames. The bounded attack sampler follows the
source initial-volley frame/wait timeline and returns to idle. It does not
retain the native repeated-attack IScript state machine.

Both Raynor sprite variants (raw 19/20) and hero Tassadar (raw 79) now belong
to the base race bank. Their enum identities exist outside the later campaign
gates, so an all-gates atlas test alone could miss their absence in base builds.
The compiled base-profile fixture now checks every unit and building enum too.

Runtime/importer coverage checks include all 53 mapped actor raw IDs and 65
building/prop IDs. Raw 101 is an intentionally invisible map revealer. The
all-profile atlas fixture requires opaque pixels for every visible mapping;
the game renderer fixture also checks Photon Cannon idle, firing, return to
idle and construction pixels through the production draw passes.

Canonical prepared race builds now enable source construction art. The census
covers all 50 stock constructible buildings, including the Greater Spire morph;
the private pack contains 135 image banks, 685 frames and 136 stage descriptors.
It draws underlays before the primary image and overlays afterward, retaining
spawn times and follow-main frame links. The all-branch audit found no
construction `setflipstate` instructions; unsupported flip scripts fail closed.

Stage transitions remain progress-based adaptations, with source visual waits
sampled at 24 Hz. Native finish/order signals, worker animations, post-timer
Protoss warp states and dynamic gas-smoke placement remain incomplete. The
importer records Refinery/Assimilator gas requests and imports their image
variants, but the renderer does not yet position those resource overlays.

With prepared inputs:

```sh
python3 tools/sc64_construction_sprites.py
make test-sc64-construction-source
python3 -m unittest tests.test_sc64_race_sprites tests.test_sc64_photon_cannon_renderer
```

For exhaustive optional atlas coverage, generate a private superset bank:

```sh
python3 tools/sc64_race_sprites.py --include-t6 --include-late-terran --include-zp-campaign-actors --include-z5-actors --include-z6-actors --include-z5-doodads --include-p6-actors --include-z6-buildings --include-p7-actors --out .local/build/stock-art/all-profile/sprites --header .local/build/stock-art/all-profile/sc64_race_sprites_generated.h
python3 -m unittest tests.test_sc64_photon_cannon_renderer
```

The source-only clone skips private pixel/atlas fixtures when inputs are absent.
All verification runs locally; GitHub Actions remain disabled/manual-only.

## Campaign regression qualification

Controller timing and force composition were adjusted through normal input,
without changing source mission outcomes or granting test-only resources or
combat advantages. T5 now requires an actual win instead of returning success
unconditionally. T6's strict two-Dropship check uses source actor-footprint
overlap rather than centers. T7 uses source-space SCV/emitter bounds and source
air-weapon reach; its bounded controller budget is 60000 ticks to allow escort
preparation. T9 gathers a full Wraith force and keeps workers mining. The
neutral skirmish economy fixture records production and gas milestones before
later combat can wipe a player.

The normal-input host runs complete all ten Terran missions. This is separate
from normal-menu ARM replay, physical hardware, or mission-wide 30-fps proof.
The individual source stock tables, available sprites and these controller
routes do not establish complete native engine equivalence.

## Final local verification

The final runtime source is commit `ebdb058`; the follow-up render fixture
keeps its mineral patch outside the restored Command Center underlay so the
depletion assertion observes independent visible pixels. All checks passed:

- `./dev build` and `./dev test`: 463 Python tests plus the asset-free host
  fixtures, including the 12 building-capacity variants.
- `make unified test-unified`: stock combat/contact/movement, renderer, cache,
  shields, construction and save checks.
- `./dev sc64 test --profile terran`: all ten actual-win mission controllers,
  startup/save, skirmish, spatial and cache checks.
- Independent stock facts and construction census: 228 raw IDs, 132 weapon
  records and 50 constructible IDs, including morphs.
- Extended P7 projectile, Carrier, Archon, Showcase and Battery fixtures.
- Fresh ARM links and ROM verification for the unified and three race builds,
  plus Showcase. Imported ROMs pad to 32 MiB; no reserve assertion was relaxed.

| ROM | IWRAM end | BSS end |
| --- | --- | --- |
| Terran | `0x03004e08` | `0x0203ddd4` |
| Zerg | `0x03004eb0` | `0x02039c50` |
| Protoss | `0x03004eb8` | `0x02039df8` |
| Showcase | `0x030049c0` | `0x0203dee8` |

The unchanged guards require IWRAM below `0x03005000`, BSS below
`0x0203e000`, and save payloads at most 16368 bytes. No new physical-GBA or
mission-wide frame-rate result is claimed. Private evidence remains under
`.local/build/stock-art/`; no Actions run was enabled or dispatched.
