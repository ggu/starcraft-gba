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
Larva (35) has it. The tracked compact profile adds mask `0x02` in
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

## Authenticated N64 eligibility comparison

A separate read-only audit decoded the already-staged 19,876-byte N64
`units.dat`, SHA-256
`54356d7585c66cd463346996444fd208336eaacf6335d30eb2688aa73c9cb2df`.
Its retained receipt binds it to normalized USA ROM SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`; receipt SHA-256 is
`45fa2f1f5f7163af84950ff999795cf9906055fcf749e31e6f6482e7c8ede7f8`.
The pinned OpenBW loader schema gives 228 little-endian 32-bit property words
at offset 7,032, with regeneration mask `0x80`. All 50 N64 eligible IDs
exactly matched the pinned-PC property set; neither set had an extra ID.
The derived 228-byte boolean vector SHA-256 is
`29741f6688fb9ae03b6df2cc3eab0ccb5b308e7965307c521ea241e225691125`.
This establishes data eligibility agreement. It does not establish native N64
cadence, damage/regen ordering or the correctness of a future GBA service.
No private table, ROM, boolean-vector payload or capture is committed.
