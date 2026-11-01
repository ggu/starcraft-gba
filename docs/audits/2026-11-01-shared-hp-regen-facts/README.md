# Source regeneration eligibility

The current shared race tick has a narrow HP-regeneration implementation:
`race_z5_active()` enables it for raw 37 and raw 51 only. It is absent from
ordinary Z8 play and the other gameplay modes. This audit and metadata change
do not enable a new runtime service or claim that this behavior is fixed.

The pinned [BWAPI UnitType table](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/BWAPILIB/Source/UnitType.cpp)
provides an explicit `RegeneratesHP` property. The original UTF-8 source file
has SHA-256 `7f16832dac82b8a739a07616ec0e61bc2c68a34b873f779cca8742de4832c76b`.
Its 234 rows contain 50 regenerators, all below the addressable raw-ID limit
of 228. Eligibility includes units, heroes and buildings; it is not an owner
race test. Egg (36), Cocoon (59) and Lurker Egg (97) lack the property, while
Larva (35) has it. These are PC default-table properties, not verified N64
unit-property equivalence. The tracked compact profile adds mask `0x02` in
its existing flags byte; mask `0x01` remains the building bit. No table dimensions or actor storage
change. Existing profile consumers mask the building bit.

The extractor records both properties as booleans. The generator refuses old
facts without `regenerates_hp`, and the complete-table checker now expects
both bits. Previously extracted private reference JSON must be refreshed from
the same pinned reference; missing metadata must not silently clear the bit.
Three asset-free Python tests passed for exact token/comment handling, row
count, all four property combinations and rejection of outdated facts. An
independent source review matched all 234 compact flags to the pinned table.
No full clean default suite, ARM link or runtime admission is claimed here.

The pinned [OpenBW update](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L1232)
checks completion and the regeneration property before adding four Q8 HP per
source tick. The existing Z5 integer adaptation retains one HP per 80 live
30-Hz updates in `shield_remainder`; exact damage/regen subframe equivalence
is not established. A common service must preserve this existing meaning,
respect pauses, and handle valid carried, burrowed, construction and morph
states according to reference evidence. The field also holds Protoss shield
fractions, Terran burn/last-attacker state and special actor timers. Egg and
colony morph validators and every supported save codec need explicit review.
Do not replace those meanings or reset fractions solely to satisfy a validator.
