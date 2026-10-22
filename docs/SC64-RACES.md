# Zerg, Protoss, and mixed-faction GBA matches

For a single public cartridge image with all six missions and custom games, use the [unified profile](SC64-UNIFIED.md). This guide preserves details of its Zerg/Protoss missions and race-specific behavior. These are reduced adaptations using selected content from the user's own verified StarCraft 64 USA ROM; the GBA engine is authored here and does not run the original N64 engine.

## Prepare and run

```sh
./dev sc64 prepare --rom "/path/to/StarCraft 64 ROM or ZIP" --profile unified
./dev sc64 build --profile unified
./dev sc64 test --profile unified
./dev sc64 play --profile unified
```

Converted maps, graphics, audit reports, and previews remain under ignored `.local/sc64/`; built images remain under ignored `build/`. The source release does not contain the ROM or generated game data. See [SC64.md](SC64.md) for the supported image format and [third-party notices](../THIRD_PARTY_NOTICES.md) for extractor provenance.

## Frontend and controls

The title flow offers Campaign, Custom Game, and Load Game. Campaign selects Zerg or Protoss, then one of that faction's two missions; a short typewriter briefing leads to Deploy. Custom Game sets the human faction and one to three computer opponents before Launch. Terran is available in custom matches. B backs through menu pages, and L loads from any page.

From the menu, Up/Down selects the faction, opponent count, or Launch row; Left/Right changes the selected faction or count. A accepts; B returns to the previous menu page. From the battlefield, D-pad moves the cursor. Hold R with D-pad to scroll faster; tap R without movement to select the army. A selects or issues contextual orders. Tap L to cycle friendly units and move the cursor to them. Hold L while moving to drag a selection box; release L to commit or press B to cancel. Double-tap A on a friendly unit to select matching living units on screen.

B opens commands. Select a worker before placing a structure. The five construction entries vary by faction: Terran Barracks/Depot/Refinery/Academy/Engineering Bay; Zerg Pool/Hatchery/Extractor/Den/Evolution Chamber; Protoss Gateway/Pylon/Assimilator/Core/Forge. Idle human workers seek minerals. Select a worker and use A near a completed gas structure to assign gas harvesting.

Select a producer and press A to queue a unit; R cycles compatible types and L cancels the oldest order. Hatcheries offer Drone, Overlord, Zergling, and Hydralisk; one Zergling order produces two. Nexus produces Probes; Gateway offers Zealot and Dragoon. The basic construction menu does not expose every advanced scenario structure. Egression's Chrysalis can be picked up by a Drone and carried to the Warp Point; if its carrier dies, it drops for another Drone to collect. In First Strike, entering the source rescue region transfers the passive Protoss base. Start/Select pauses: A resumes, B saves, L loads, and R restarts.

## Missions

| Scenario | Source BOLT entry | Size | Required objective |
| --- | --- | --- | --- |
| Among the Ruins | `008/014` | 64 × 64 | Build a Spawning Pool and Hydralisk Den, protect the Chrysalis, destroy the Terran buildings. |
| Egression | `008/015` | 96 × 96 | Pick up the Chrysalis with a Drone and carry it to the Warp Point. |
| First Strike | `008/01E` | 96 × 96 | Rescue Fenix and the Protoss base, then destroy the Zerg buildings; Fenix survives. |
| Into the Flames | `008/01F` | 96 × 64 | Wait for the 900-second reinforcement trigger, protect Fenix, and destroy the Cerebrate. |

The four maps have 99, 143, 117, and 141 non-editor placement records, respectively, including resources. Terrain and placement coordinates use half the source pixel scale. Jungle and Space Platform terrain are converted to a shared world palette. The implementation includes 88 static sprite snapshots. Ownership preserves human, enemy, neutral, and rescue-passive roles.

For Among the Ruins, expand the Drone economy, build an Extractor and Den, and replace Drones consumed by Zerg construction. Hydralisks handle Wraiths that Zerglings cannot hit. Defend the economy, take the southern passage around the lake, and keep rebuilding tech structures to support reinforcements. Issue a new attack order promptly when a target falls; Goliaths can outrange an idle Hydralisk's acquisition range in this engine. The controller route builds toward eight Drones and thirty Hydralisks and continues production during the assault.

The Egression controller route takes the four-connected southern passage around disconnected terrain and the Protoss hub. It advances the Hunter Killers and carrier through short waypoints; the final carrier order goes past the Warp Point centre to account for movement stopping distance. This proves one controller-driven completion route, not the survival or timing of every player route. SRAM retains walking construction orders, shields, larva, fog, and mission state in a 12,024-byte packed payload.

## Reduced mechanics and limits

The engine implements race workers and resource trips, paid queues, doubled supply, larva limits and regeneration, Drone consumption, Probe return during warp-in, approximate creep/Pylon coverage, regenerating shields, ground/air weapon eligibility, per-hit Zealot/Fenix damage, and basic static defenses. Source weapon ranges are scaled to the half-size world; reference 24-frame timings are converted to the 30-update simulation. Normal gas trips carry eight; gas reservoirs do not yet deplete.

Enemy strategy is authored and much smaller than the original campaign AI. Construction footprints, power/creep coverage, pathfinding, combat damage types, upgrades, and special abilities are incomplete approximations. Larva orders finish sequentially in a Hatchery queue rather than as independent eggs. There is no complete tech tree, transport system, spell roster, directional animation, original audio, or cinematic presentation. The shared limits are 128 units and 64 buildings across all owners. The build does not interpret arbitrary CHK triggers or execute original AI scripts.

Original scenario geometry and objective rules can be preserved more closely than the complete engine behavior. An omitted feature remains unfinished work unless measured hardware evidence establishes a tradeoff.

## References and validation

The source ROM is checked against SHA-1 `a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`. Extraction uses [sc64-maps](https://github.com/dmang-dev/sc64-maps) commit `09b57f787a247107d1de82b8c5eab2734667ea1d`; unit and weapon facts are checked against [BWAPI](https://github.com/bwapi/bwapi) commit `d727fed68558c506163048ea889131d8cbb33915`; [OpenBW](https://github.com/OpenBW/openbw) commit `4b046d5f65302b10cb0a745f0fecd37ec85b20a8` is a behavioral reference. License status and source locations are in [REFERENCES.md](REFERENCES.md) and [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md).

`make test-races` checks economies, production, gas, shields and power, objective transitions, air collision/targeting, melee boundaries, save round trips on all four maps, and packed shortest paths under ASan/UBSan. Objective fixtures exercise state transitions; they are not controller completions. `make test-races-controller` runs controller drivers for all four campaign maps. `tools/sc64_races_profile.py --unified` replays the traces against the actual ARM ROM in mGBA and compares host checkpoints.

The recorded campaign controller runs complete all four missions. Fast bounded cell arithmetic and sixteen packed reverse path fields addressed earlier 12–20 fps measurements on larger maps. Emulated timing is separate from hardware testing; see [VALIDATION.md](VALIDATION.md). Physical GBA SP/EverDrive validation is pending.
