# Gameplay references

These pinned repositories were consulted during implementation. The default StarCraft prototype is original code and authored data; it does not bundle their source or Blizzard assets. See [third-party notices](../THIRD_PARTY_NOTICES.md) for license and provenance notes.

| Project | Revision | Use |
| --- | --- | --- |
| [OpenBW](https://github.com/OpenBW/openbw) | `4b046d5f65302b10cb0a745f0fecd37ec85b20a8` | Behavioral and file-format research. The inspected snapshot had no root license file; do not copy implementation code. |
| [BWAPI](https://github.com/bwapi/bwapi) | `d727fed68558c506163048ea889131d8cbb33915` | Unit and weapon identities and default numeric values. Its repository includes LGPL-3.0 with additional permission notices. |
| [sc64-maps](https://github.com/dmang-dev/sc64-maps) | `09b57f787a247107d1de82b8c5eab2734667ea1d` | Optional StarCraft 64 BOLT extraction tools; GPL-3.0-or-later. |
| [StarCraft64-recomp](https://github.com/n0madic/StarCraft64-recomp) | `425f409e02edacf897354cd0872c61ce67a9067a` | Optional exact-ROM native-code research through its N64Recomp recipe. Generated game code remains private; no implementation is incorporated here. |

## StarCraft mechanics and values

BWAPI's [`UnitType.h`](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/include/BWAPI/UnitType.h) and `WeaponType.h` define identifiers. `bwapi/bwapi/BWAPILIB/Source/UnitType.cpp:559–919` records default hit points, armor, mineral/gas cost, build time, and doubled supply; `WeaponType.cpp:152–216` records default damage, cooldown, and range. The selected values are transcribed in [`reference-stats.json`](reference-stats.json). Supply is doubled internally by BWAPI; the GBA game stores half-supply values. The 32-pixel original terrain unit is represented on a 16-pixel GBA grid.

T4 raw unit 20 is `Hero_Jim_Raynor_Marine`, distinct from raw-19 `Hero_Jim_Raynor_Vulture` (`UnitType.cpp:43–44`). The marine has 200 HP, 3 armor, zero supply, and 4.0 top speed (`UnitType.cpp:559–561`, `853–855`, `908–910`, `1402–1404`). Its ground and air weapons are `Gauss_Rifle_Jim_Raynor` (`UnitType.cpp:1317–1343`); weapon index 1 supplies 18 base damage, +1 damage per upgrade, 15-frame cooldown, and 160-pixel range (`WeaponType.cpp:152–162`, `194–199`). The GBA engine applies its established half-scale range and 5:4 simulation-frame cooldown conversion.

OpenBW's [`bwgame.h`](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h), especially its worker gathering and resource credit paths around lines 2120–2156 and 4485–4495, informs the eight-mineral worker trip and selected simulation behaviors. `openbw/data_loading.h:1470–1515` documents data-field interpretation. OpenBW itself reads unit and weapon records from game DAT files; this project does not include those files.

Build times and weapon cooldowns are simulation-frame counts. The default skirmish scales reference times to its 30-update target. The prototype uses simplified combat, collision, route finding, AI, and scenario balance; these references do not imply full original-game fidelity. Numeric facts and identifiers are documented for provenance and are not copied BWAPI implementation code.

## Optional StarCraft 64 missions

The optional unified profile reads map and graphics data from a user-supplied verified USA N64 ROM through `references/sc64-maps/extract_sc64_maps.py` at the pinned `sc64-maps` commit. The local adapter is `tools/sc64_import.py`; terrain, sprite, and race packing is implemented in `tools/sc64_assets.py`, `tools/sc64_sprites.py`, `tools/sc64_races_pack.py`, and `tools/sc64_race_sprites.py`. Imported outputs remain local. The result is an authored GBA engine and mission adaptation, not a port of the N64 engine. See [the unified guide](SC64-UNIFIED.md).

T4's numeric trigger, location and THG2 records are preserved by
`tools/sc64_t4_scenario_pack.py`. It emits private immutable tables, including
unused slots and opaque bytes. The authored trigger runner uses a documented
30 Hz timing policy; its synthetic tests establish ordering and resumability,
not equivalence to the N64 scheduler. Native trigger research uses the separate
recompilation recipe above with the verified local ROM. Handler behavior alone
does not prove how raw CHK operands are converted into native runtime records;
that conversion must be traced before binding uncertain action operands.

For the USA image, native converter `0x8002C3EC` walks 20-byte conditions and
32-byte raw actions, packing actions into 28-byte runtime records. It preserves
the opcode/modifier/flags bytes. The action table at `0x800C7564` dispatches
opcode 42 to `0x80084AB4`: raw states 0 and 6 toggle, 4 enables and 5 disables.
Opcode 11 at `0x800819EC` defaults a zero count to one creation. These are
behavioral facts transcribed into the authored helpers in
`starcraft/sc64_t4_actions.h`; native instructions and generated C remain
private. Exact actor selection, animation and collision transitions still
need separate evidence and tests.

The source setup importer also preserves FORC, OWNR, SIDE, UPRP and UPUS;
these records distinguish original ownership, force membership, controllers
and creation properties. Native trigger instantiation at `0x8008E094`
normalizes the enabled selector by subtracting 13: switch case 4 (selector
17) expands All Players, while cases 5–8 (selectors 18–21) filter Forces 1–4.
Each matching source slot gets independent execution state. The operand
namespace therefore agrees with the current Command/Bring helper for these
selectors. Other enabled group selectors are rejected until implemented.

Native Move Unit handler `0x8008498C` defaults count zero to a remaining-match
limit of `0xF423F`; its callback decrements after matching. This establishes
that zero processes all matching actors within the GBA's bounded pools.
Destination placement and exact native candidate order remain separate work.
The native action table's RAM address maps to ROM offset `0xC8164` in this
image; entries 11 and 42 provide independent anchors for that mapping.

The 1,280-byte property loader at `0x8002C394` copies its section to
`0x800C7654`; `0x80085EDC` then converts 64 records in place at a 20-byte
stride. The create-property path validates an action number in 1–64 and
addresses `0x800C7640 + number*20`, so number 1 selects source UPRP entry 0.
The authored index helper follows that mapping. Applying the individual
property override fields to live actors remains unimplemented.
