# Shared target-acquisition source audit

Scope: source inspection at experimental `443a905`, with no runtime change or native test. The pinned PC references are [BWAPI UnitType.cpp at d727fed](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/BWAPILIB/Source/UnitType.cpp#L1293-L1301), [OpenBW data loading at 4b046d5](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/data_loading.h#L1404-L1413), and [OpenBW bwgame.h at 4b046d5](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L19899-L19923). These sources do not establish N64 equivalence.

OpenBW loads a source acquisition field, then initializes each unit type to the maximum of that field and its available ground/air weapon maximum ranges divided by 32, using its turret type where applicable. Its [runtime acquisition](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L12232-L12247) adds applicable player upgrades. The following are initial, zero-upgrade nominal ranges in game-scale pixels (source pixel coordinates are scaled by two); they are not directly equivalent geometric envelopes:

| Raw unit | BWAPI seek tiles | OpenBW effective tiles | Nominal game pixels | Current imported idle query | Current attack-move query |
| --- | ---: | ---: | ---: | ---: | ---: |
| 37 Zergling | 3 | 3 | 48 | 27 | 64 |
| 38 Hydralisk | 0 | 4 | 64 | 83 | 64 |
| 51 Infested Kerrigan | 3 | 3 | 48 | 27 | 64 |
| 65 Zealot | 3 | 3 | 48 | 27 | 64 |
| 66 Dragoon | 0 | 4 | 64 | 83 | 64 |

Zero raw seek does not mean zero acquisition: ranged weapon maxima supply the effective four-tile floor for the Hydralisk and Dragoon. Current values follow `starcraft/race_rules.h` `sc64_race_unit_range`, `starcraft/sc64_zp_actor_rules.h` raw 51, and `starcraft/game.c` `unit_range` and imported idle/attack-move paths. The idle query is `unit_range + 19`; attack-move uses `unit_range + 18` only above 64 and otherwise 64. `STOP / HOLD` calls `stop_selected`, setting `ORDER_IDLE`; the idle combat path can call `tick_combat` and move toward a target, so a shared idle state does not encode a distinct hold-position policy. Ordinary MOVE does not acquire while moving.

The reference comparison has material qualifications. [OpenBW targeting](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L1625-L1630) measures sprite bounding-box separation using an approximate length, with an inclusive range. Its [acquisition selector](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L1841-L1915) applies owner-specific visibility/detection, attackability, weapon minimum range and firing-angle constraints; it prioritizes target classes before nearest choice and caps candidates per class. Current `sc64_target_nearest`/`nearest_enemy_scan` use center distance, a strict-radius bound, live-owner hostility, eligible-target checks and lower entity index for ties, with Egg fallback; SIDE requires current optical grid visibility and enabled P7 detection uses source-owner provenance. Non-SIDE optical handling is not the same per-source visibility rule. Explicit attacks and projectiles have their own paths.

A future shared service should first represent source identity, player upgrades and independent idle/guard, hold, attack-move and explicit-order semantics; then qualify geometry, source-owner detection, threat priority, target filters, minimum range and deterministic tie/fallback behavior across the production profiles. A seek-only radius replacement or global attack buff would not implement that contract. Preserve existing behavior until isolated fixtures, ordinary controls, save/load, performance and mode evidence are available.
