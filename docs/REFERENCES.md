# Gameplay references

These pinned repositories were consulted during implementation. The default StarCraft prototype is original code and authored data; it does not bundle their source or Blizzard assets. See [third-party notices](../THIRD_PARTY_NOTICES.md) for license and provenance notes.

| Project | Revision | Use |
| --- | --- | --- |
| [OpenBW](https://github.com/OpenBW/openbw) | `4b046d5f65302b10cb0a745f0fecd37ec85b20a8` | Behavioral and file-format research. The inspected snapshot had no root license file; do not copy implementation code. |
| [BWAPI](https://github.com/bwapi/bwapi) | `d727fed68558c506163048ea889131d8cbb33915` | Unit and weapon identities and default numeric values. Its repository includes LGPL-3.0 with additional permission notices. |
| [sc64-maps](https://github.com/dmang-dev/sc64-maps) | `09b57f787a247107d1de82b8c5eab2734667ea1d` | Optional StarCraft 64 BOLT extraction tools; GPL-3.0-or-later. |

## StarCraft mechanics and values

BWAPI's [`UnitType.h`](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/include/BWAPI/UnitType.h) and `WeaponType.h` define identifiers. `bwapi/bwapi/BWAPILIB/Source/UnitType.cpp:559–919` records default hit points, armor, mineral/gas cost, build time, and doubled supply; `WeaponType.cpp:152–216` records default damage, cooldown, and range. The selected values are transcribed in [`reference-stats.json`](reference-stats.json). Supply is doubled internally by BWAPI; the GBA game stores half-supply values. The 32-pixel original terrain unit is represented on a 16-pixel GBA grid.

OpenBW's [`bwgame.h`](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h), especially its worker gathering and resource credit paths around lines 2120–2156 and 4485–4495, informs the eight-mineral worker trip and selected simulation behaviors. `openbw/data_loading.h:1470–1515` documents data-field interpretation. OpenBW itself reads unit and weapon records from game DAT files; this project does not include those files.

Build times and weapon cooldowns are simulation-frame counts. The default skirmish scales reference times to its 30-update target. The prototype uses simplified combat, collision, route finding, AI, and scenario balance; these references do not imply full original-game fidelity. Numeric facts and identifiers are documented for provenance and are not copied BWAPI implementation code.

## Optional StarCraft 64 missions

The optional unified profile reads map and graphics data from a user-supplied verified USA N64 ROM through `references/sc64-maps/extract_sc64_maps.py` at the pinned `sc64-maps` commit. The local adapter is `tools/sc64_import.py`; terrain, sprite, and race packing is implemented in `tools/sc64_assets.py`, `tools/sc64_sprites.py`, `tools/sc64_races_pack.py`, and `tools/sc64_race_sprites.py`. Imported outputs remain local. The result is an authored GBA engine and mission adaptation, not a port of the N64 engine. See [the unified guide](SC64-UNIFIED.md).
