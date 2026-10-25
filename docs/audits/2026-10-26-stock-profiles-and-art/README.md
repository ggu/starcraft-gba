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

Reproduce the independent fact comparison using a clean local reference:

```sh
python3 tools/sc64_unit_spec_reference.py --reference /path/to/bwapi --output .local/unit-specs/reference.json
make test-sc64-stock-profiles-facts SC64_STOCK_FACTS=.local/unit-specs/reference.json
make test-sc64-stock-motion test-sc64-stock-motion-runtime test-sc64-stock-combat-runtime
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

Runtime/importer coverage checks include all 53 mapped actor raw IDs and 65
building/prop IDs. Raw 101 is an intentionally invisible map revealer. The
all-profile atlas fixture requires opaque pixels for every visible mapping;
the game renderer fixture also checks Photon Cannon idle, firing, return to
idle and construction pixels through the production draw passes.

Canonical prepared race builds now enable source construction art. The census
covers all 50 stock constructible buildings plus the Greater Spire morph;
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
