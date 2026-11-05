# Guardian morph research (staged design, not implemented)

Base source: `9f596ad1`. Pinned local stock profile records Guardian raw 44
at 50 minerals, 100 gas, 600 source frames, 150 HP and four doubled supply;
Cocoon raw 59 has 200 HP. The local catalog says Guardian morphs from raw 43
Mutalisk and requires raw 137 Greater Spire. Campaign and skirmish have
profile-dependent Guardian aliases; their existing IDs must remain unchanged.
Skirmish already supports Lair/Hive and Spire/Greater Spire building morphs;
campaign currently exposes only Lair and Spire. Guardian and Mutalisk sprite
export mappings exist, but Cocoon sprite coverage is not established.

## Primary implementation evidence

OpenBW `4b046d5f65302b10cb0a745f0fecd37ec85b20a8`,
[`bwgame.h`](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h):

- `order_ZergUnitMorph` lines 6299–6334 transforms a Mutalisk in place into a
  Cocoon, initializes the remaining time to the product's build time, counts
  down per simulation tick, then waits for an animation signal to transform.
- `cancel_building_unit` lines 2384–2420 fully refunds the queued product cost,
  changes Cocoon back to Mutalisk in the same unit, clears remaining time and
  enters birth animation. Building morph cancellation instead refunds 75%.
- `reinitialize_unit_type` lines 2932–2969 changes HP proportionally to the new
  maximum with integer truncation and a minimum of one. Thus damage during the
  Cocoon affects both completion and cancellation HP; original HP is not simply
  restored. These are OpenBW semantics, not installed N64 timing measurements.

## Existing wire fields and collisions

| Codec | Row size | `type` / `raw_type` | `order` | `build` | `work_timer` | `target` | `flags` |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Z/P packed | 35 bytes | 6 / 9 bits | 4 | signed 10 | 16 | signed 10 | 16 |
| Compact | 38 bytes | 6 / 9 bits | 4 | signed 10 | 16 | signed 10 | 16 |
| Z7 derived | 29 bytes | 6 / 8 bits | 4 | signed 10 | 16 | signed 10 | 8 |
| P7 typed | 28 bytes | 6 / 8 bits | 4 | biased 8 | 11 | biased 9 | 14 |
| Legacy word array | `2 * RACE_UNIT_WORDS` bytes | signed 16 each | signed 16 | signed 16 | signed 16 | signed 16 | unsigned-recovered 16 |

Derived HP uses type plus raw identity; P7 typed derives HP and armor from type
alone. Existing flags include 0x02 burrow, 0x08 special actor state, 0x10
invincibility and worker-specific 0x20/0x40/0x80 cargo and gather state. Other
profile-specific meanings require a full scoped collision check. `build` is a
worker depot/building reference, `work_timer` serves worker actions and other
unit services, `target` is a combat/entity reference, and `order` drives common
movement and combat. These are not globally spare fields.

Proposed staged wire: append a dedicated Cocoon type after all existing enum
entries, gated so earlier IDs do not shift; raw identity 59; canonical HP 200
and armor from pinned facts; `work_timer` stores 1–750 engine ticks if the
existing 5/4 build-time conversion is retained, and `target` stores the sole
Guardian product identity under a Cocoon-only tagged contract. Idle order,
zero cargo, valid owner, selection and in-bounds coordinates are checked. The
Cocoon type itself disambiguates these fields; no worker, transported unit or
ordinary combat actor may be reinterpreted. This needs codec-specific
canonicalization including P7 typed, a distinct feature journal namespace,
strict old-tag rejection or proven migration, and tests for malformed state.
The exact feature tag must be allocated against the complete tag registry and
transport/cargo variants; no numeric tag is reserved yet. Six-bit enum capacity,
full runtime memory, double-journal payload size, graphics and linker guards
must be measured for every intended product before activation. The 5/4 mapping
and animation timing remain approximations requiring native comparison.
No code, save semantics or default profile changes are implemented here.

## Prototype observations

The test-only adapter in `tests/sc64_guardian_wire_real.c` converts the staged
runtime Cocoon to Overlord wire type with raw 59 and reconstructs it only when
its test discriminator is staged. It is not connected to journal load/write.
At `ab09d9e3` actual `game.c` profile fixtures passed warning-clean ASan/UBSan:
T10 legacy/Cocoon ID 44 and 38-byte row, Z7 ID 42 and 35-byte row, P10 ID 53
and 28-byte row, Showcase ID 49 and 28-byte row. All used Overlord ID 8. Each
fixture checks full/damaged HP, owner/selection, timer bounds and malformed
Cocoon fields, rejects legacy reconstruction and unconverted runtime rows.
The P10 test used a separate copied fact header changing only the roster static
assert from `SC_RU_COUNT` to `SC_RU_LEGACY_COUNT`; original SHA-256
`bd13f2f29a5e49c4f97124c0ed627a5476eb98583f277eca4072d1d747df5a7e`,
overlay `5f421804062e3c34143f565dac9337d78f211135ec847e9dfd4960e527fc0149`.
The generated-input receipt itself was not modified. A generator fix and
normal stage build are still required.

The staged actor facts candidate `633b02f3` maps raw59 to 200 HP, armor 0,
flying, no weapon and source dimensions 16/16/15/15 (eight GBA pixels).
Pinned BWAPI `d727fed68558c506163048ea889131d8cbb33915` raw59 flags are Flyer
and OrganicUnit and sight is four tiles; the candidate uses stationary,
selectable/collidable/drawable engine actor flags. Warning-clean accessors passed
on T10 at `bab84c9e`, and on Z7/P10/Showcase at `7025a459` after fixing the
fixture to inspect raw sight without requiring an active Z7 mission. This does
not prove native movement, sprite, targetability or actual mission sight state.

OpenBW `increment_unit_counts` uses an Egg/Cocoon's queued product supply,
not raw59's zero stock supply. Guardian and Mutalisk each use four doubled
supply units, so the stage rule reserves four for this specific morph. OpenBW
also treats morph-from-another-unit admission specially. Lifecycle owner/supply
accounting, transport and pending-impact interactions still require real tests;
no production morph command, journal tag, or ordinary menu is enabled.

## Namespace and stage-off checks

`tests/test_sc64_guardian_tag_registry.py` reads five pinned source revisions,
including the Carrier/Defiler draft and ordered-Z10 candidate, and enumerates a
conservative cross-product of all base generations 2–50 and the roster, BC,
cargo, transport/Carrier and Scarab selector bits, including unsupported flag
combinations. Its 5,056 historical values and 5,056 values with proposed XOR
`0x10000000` are disjoint. This verifies those checked source definitions, not
unpublished candidate namespaces; it does not enable journal admission.

At `5cdc4b29`, ordinary stage-off `make race-campaigns showcase` produced ROMs
byte-identical to detached base `9f596ad1`, with all 1,958 receipt inputs
verified before and after each build. SHA-256: Terran
`39e30ca9aa7e4b5a6b80b3625b5eb3db473b9aa0d72fbcd0ccb264142eb3f670`, Z7
`0c98893676158019a5b643f52be2a80d1506a6fa52276fad6ca41342e5b4c117`, P10
`2f2eae2eb64a9874adf47826bee8be0b1d9ce134a6955dc8ee9313783d7a0cb0`, and
Showcase `2dbce71c6f06a98fd84baca366a581d79bf6fd84a181d2ef775eb0399a50ba8f`.
The generator test `guardian-fact-generator` at `feab7649` produced identical
stage-off and stage-on generated P10 headers, SHA-256
`5a344c30921fed5c468343c9e32eaf41ff456ae818dc669709e8f280d633efcd`.
Compared with the existing pinned header, the only output difference adds a
stage-conditional roster assertion; feature guards and fact data are unchanged.

The HOST-only journal prototype now carries the staged Cocoon namespace; it is
not enabled for guest builds. A Terran legacy save can load an Overlord row
whose source raw ID is 44, so that pair cannot identify a staged Guardian.
The candidate instead appends a distinct Guardian after the Cocoon only when
neither existing Guardian alias is available. In the actual T10 profile these
IDs are 44 and 45; the compact row has a six-bit type and the old bound rejected
both. Only the stage build permits the latter through that leaf codec, and the
journal decoder requires the authenticated stage tag and validates its raw ID,
health, armor, orders, and target bounds. Existing Guardian aliases remain
unchanged. Raw-44 scenario import, morph lifecycle, menus, guest admission,
source art, and full actor/render qualification remain outside this prototype.
The staged Guardian's gameplay facts still need complete lifecycle review.

The staged collision accessor uses source boxes 16/16/15/15 for Cocoon and
22/22/21/21 for Guardian, yielding the engine's 8- and 11-pixel radii. This
only changes staged actor mappings, not the general flyer collision policy.
Terran Guardian sight uses the raw-44 source sight lookup (352 source pixels).
Its flying motion uses the existing raw-44 stock-speed path, which does not
model source acceleration, turning, upgrades, or IScript movement.
