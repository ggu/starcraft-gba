# Unified StarCraft GBA build

`StarCraft64-GBA.gba` combines the six implemented campaign missions and
three-faction custom games in one cartridge image. It uses the illustrated
Campaign / Custom Game / Load Game frontend. Choose Terran, Zerg or Protoss,
then either of that faction's two missions. Winning the first mission advances
to the second with A; B returns to the frontend. All six missions are immediately
selectable for testing.

| Faction | First mission | Second mission |
| --- | --- | --- |
| Terran | Wasteland | Backwater Station |
| Zerg | Among the Ruins | Egression |
| Protoss | First Strike | Into the Flames |

Custom Game offers all three human factions and one to three computer players,
including mixed factions. It uses the controller gestures documented in the
[default game guide](../starcraft/README.md), with race-specific construction
and production choices.

## Prepare and launch

```sh
./dev sc64 prepare --rom "/path/to/StarCraft 64 ROM or ZIP" --profile unified
make unified test-unified
./dev play unified
```

Preparation requires the user's verified USA N64 ROM and the separately
obtained, pinned `sc64-maps` extractor. Converted maps, graphics, menus, and
reports stay under ignored `.local/sc64/`; the ROM and build outputs stay under
ignored `build/`. The project does not distribute source ROMs or extracted game
content.

A single three-faction engine loads one map at a time, using fixed-size GBA
unit, building, path, visibility and terrain storage. Terrain is converted
offline into the shared Backwater palette. The combined ROM does not load two
engines into working RAM or require a second cartridge image.

The handheld display palette lifts dark terrain and sprite detail. Secondary
text, panel borders and
explored fog have clearer contrast. Palette conversion runs at initialization
and when launching/loading from the frontend, without adding work to gameplay
frames. The illustrated menu artwork retains its own palette.

## Mission and save behavior

Wasteland retains the opening mineral grant, Raynor contact rescue, Barracks
and ten-Marine objective, and Raynor/player survival conditions. Backwater reuses
the existing source-derived region callbacks, passive rescues, timed
reinforcements, Infested Command Center objective, Raynor survival and ending
sequence. The missing Terran raw-type mappings for Raynor's Vulture, Bunkers,
Academy and Infested Command Center are included in the combined loader.

Zerg and Protoss retain their existing scenario rules. Stable internal IDs are
Z1/Z2/P1/P2 = 0–3 and T1/T2 = 4–5; the visible menu orders Terran, Zerg, Protoss.
Campaign progress uses six completion bits. Packed SRAM includes the active
map, faction, entities, fog, resources, queues, mission state and Backwater's
pending trigger timers and invincibility state.

Pause with Start or Select. B saves; L loads; A resumes; R restarts. From the
frontend, Load Game or L loads the saved run. The unified ROM uses its own
`SCU1` save tag and `StarCraft64-GBA` filename. Existing separate-ROM saves are
preserved; they are not automatically imported into this new save format.

## Fidelity and validation

This combines the implemented GBA adaptations. It does not add later missions,
full original AI, complete CHK interpretation, spells, cinematics, transmission
dialogue or annoyed unit responses. Imported sound effects, unit responses and
music play when the optional audio bank is prepared; see
[Development](DEVELOPMENT.md#optional-starcraft-64-sound-effects-and-music). The original scenario terrain and placements, reduced mechanics and
remaining limitations are described in [Terran campaign](SC64-CAMPAIGN.md) and
[Races](SC64-RACES.md). Reference revisions and license status are listed in
[REFERENCES.md](REFERENCES.md) and [third-party notices](../THIRD_PARTY_NOTICES.md).

`make unified test-unified` exercises all six frontend launches and SRAM round trips,
Terran objective/rescue/ending fixtures, all four existing Zerg/Protoss
objectives, faction economies and production, terrain changes, shields and
save validation under ASan/UBSan. Fixture-driven objective checks are separate
from a full controller-driven playthrough.

`python3 tools/sc64_unified_smoke.py` checks the actual ARM ROM in mGBA: all six
campaign menus/briefings/launches, palette restoration, all nine custom-game
faction/opponent combinations, and fresh-process Terran save loads. Outputs
are in `build/emulator/sc64-unified/`. Controller traces can be profiled with
`tools/sc64_races_profile.py --unified`; emulated timing is separate from GBA SP
hardware testing. Reproduce the asset-free and optional-profile checks listed
in [VALIDATION.md](VALIDATION.md).
