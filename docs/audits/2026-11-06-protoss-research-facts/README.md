# Protoss research: authenticated facts and integration boundary

At runtime source `31a5ddf8d3734427ebfef151acfb683d0db54a03`, the WEAPONS and ARMOR rows use SIDE-only generic globals, require a Forge for Protoss, charge 100 minerals and 100 gas, finish after 600 updates, and stop at level two (`starcraft/game.c`, `start_research` and `tick_research`). `RaceSave.state` stores those four generic values. This does not represent Protoss upgrade families, other owners or producer identity. The stock-weapon helper already has a source per-impact `upgrade_bonus`; some other attack and shield paths use separate calculations and cannot be covered by changing the menu alone.

`tools/sc64_protoss_research_facts.py` authenticates the owned USA ROM, the pinned extractor file (`sc64-maps` commit `09b57f787a247107d1de82b8c5eab2734667ea1d`, file SHA-256 `6a84761a9a8074fcc393527a7b17cea194084e8ab49176ff82fa3bef8e1b1dc2`) and the private N64 `upgrades.dat` entry `00C/006` (SHA-256 `5c7a104f37eab018d15719066052c2db25ef046d9330770045d7e4977f98c12b`). It prints only six selected numeric rows. For level n, cost is base + factor × (n−1), separately for minerals and gas; source time is base + factor × (n−1):

| Upgrade | Source id | Mineral and gas base | Each level increment | Time base / increment | Maximum |
| --- | ---: | ---: | ---: | ---: | ---: |
| Ground armor | 5 | 100 / 100 | 75 / 75 | 4000 / 480 | 3 |
| Air armor | 6 | 150 / 150 | 75 / 75 | 4000 / 480 | 3 |
| Ground weapons | 13 | 100 / 100 | 50 / 50 | 4000 / 480 | 3 |
| Air weapons | 14 | 100 / 100 | 75 / 75 | 4000 / 480 | 3 |
| Plasma shields | 15 | 200 / 200 | 100 / 100 | 4000 / 480 | 3 |
| Singularity Charge | 33 | 150 / 150 | 0 / 0 | 2500 / 0 | 1 |

These are source-table units, not verified N64 runtime durations. The project's existing 5:4 frame-conversion policy is an approximation and must be applied explicitly, not described as native timing. [Pinned BWAPI upgrade types](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/BWAPILIB/Source/UpgradeType.cpp) identify Forge for ground weapons, ground armor and shields; Cybernetics Core for air weapons, air armor and Singularity Charge. Its requirement table is a PC-source proxy, not an authenticated N64 prerequisite trace. Producer validity, power interruption, cancel/refund and scenario-specific upgrade limits require independent treatment.

A bounded production implementation needs per-owner level and paid job state, an exact producer binding and family uniqueness checks; real payment, completion, and field UI must all share the same rule source. It must use ground/air *attacker identity*, not target flight, to choose weapon family; target identity chooses armor family, with shield upgrades applied in shield damage. Singularity must change the relevant Dragoon weapon range rather than all ranges. Validate each route against source weapon/armor identities and special-unit handling.

The existing save stores two generic SIDE-only levels and timers; loading an old journal must not silently reinterpret a generic weapon or armor level as one family only. A new authenticated version and explicit legacy policy must preserve or deliberately migrate both ground/air effects and pending generic jobs without awarding shield upgrades. Candidate validation must happen before mutating the live world, reject duplicated or invalid producer bindings, and preserve journal fallback. `RaceSave` has profile-dependent layout and `common/gba.h` limits accepted journal tags; capacity, tag collisions and actual EWRAM/IWRAM/stack guards must be measured for every supported product before selecting a wire layout. No save-schema change or runtime research implementation has been made in this facts-only checkpoint.
