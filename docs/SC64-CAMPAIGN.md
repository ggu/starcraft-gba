# Terran campaign and computer skirmish profiles

For the public one-image release with all six missions and custom games, use the [unified build](SC64-UNIFIED.md). This guide retains specific details about the Terran missions and earlier profile builds. These are reduced adaptations based on selected data from a user-supplied verified StarCraft 64 USA ROM; the GBA engine is implemented here and does not run the original N64 engine or its complete campaign AI.

## Prepare and run

The campaign profile places Wasteland and Backwater Station in one ROM. The skirmish profile uses the same compact engine with computer opponents. Both require the supported USA ROM; preparation keeps generated maps, palettes, sprites, headers, reports, saves, and captures under ignored `.local/` and `build/` directories.

```sh
./dev sc64 prepare --rom "/path/to/StarCraft 64 ROM or ZIP" --profile campaign
./dev sc64 build --profile campaign
./dev sc64 test --profile campaign
./dev sc64 play --profile campaign

./dev sc64 prepare --rom "/path/to/StarCraft 64 ROM or ZIP" --profile skirmish
./dev sc64 build --profile skirmish
./dev sc64 test --profile skirmish
./dev sc64 play --profile skirmish
```

Campaign preparation uses the Backwater asset set as its shared palette base, then packs Wasteland's terrain and placements into that palette. Skirmish uses Backwater terrain and sprite assets. Individual Wasteland and Backwater ROM targets are also supported by the wrapper. The optional extraction dependency is GPL-3.0-or-later and external; see [third-party notices](../THIRD_PARTY_NOTICES.md).

## Included terrain, rules, and limits

The campaign ROM uses original 64×64 Badlands terrain and placements for Wasteland and Backwater. Both maps share Backwater's reduced terrain palette. Collision uses N64 VF4 flags with the project’s coarse GBA tile rule. Later Terran maps remain unsupported. A local ROM inventory identified 28 scenarios with a Terran human slot. T3, Desperate Alliance (`008/00C`), is 96×96 Badlands with 199 placements and ten trigger records. It exceeds the current 64×64 runtime limit and needs mission-specific trigger rules. The following entry (`008/00D`, “Untitled Scenario”) is 128×128 Installation with 102 placements and 38 trigger records. Later missions introduce Space Platform terrain and larger mixed-faction maps; each needs map-size support, matching terrain/collision, and authored objective behavior. The inventory does not establish playability.

The skirmish supports one human side and up to three computer sides. Each computer keeps separate resources and queues, produces workers and combat units, and attacks according to team relationships. This is a compact deterministic opponent model; it is not StarCraft's original scripted and reactive AI.

Mission objectives and selected Backwater events are authored for these maps. The runtime does not interpret arbitrary CHK triggers. It retains raw UNIT flags in imported data, but ignores them; 13 Backwater Zerglings have the burrow flag and appear unburrowed. No Backwater UNIT record sets invincibility. The shared tech tree and reduced statistics do not cover every mission technology or upgrade setting. Both profiles use a smaller unit roster, simplified combat, coarse pathfinding, and controller-focused interface. Placements, map dimensions, and terrain art are retained where available; complete engine behavior is not.

## Controls

- On the campaign title, Up/Down selects Wasteland or Backwater; A starts and B loads. After winning Wasteland, A advances to Backwater. Both missions can be selected immediately.
- On the skirmish title, Up/Down selects one to three computer opponents; A starts and B loads. Friendly markers and health bars are green; computer sides use red, gold, and purple.
- D-pad moves the cursor. A selects or issues the current order; B opens the command menu. Select an SCV to construct from that menu; select a Barracks to queue Marines.
- Tap R to select the combat group; with a Barracks selected, R cycles training type. Tap L to cycle friendly units; hold L and move the D-pad to drag a selection box. Double-tap A on a friendly unit to select matching units currently on screen.
- Start or Select pauses. A resumes, B saves, L loads, and R restarts.

## Backwater objective and route

Destroy the Zerg Infested Command Center and keep Jim Raynor alive; killing every Zerg is unnecessary. The ending waits run before the mission-clear screen. Mine with the starting SCV, keep producing Marines, and lead the combat force through the abandoned settlement and rescue bunkers. Bunker events provide Firebats and additional SCVs. Build up the rescued force, then use B → ATTACK MOVE → A on the infested base. Keep Raynor with the group and protect him. The recorded controller route wins with 13 Marines, three Firebats, and three SCVs remaining, without removing defenders or granting extra resources. This verifies one route, not all player strategies or balance choices.

If a Barracks is selected, use an L+D-pad selection box over the army before its next order. Double-tap A on a friendly unit to select matching units on screen. Backwater dialogue and sound remain internal event IDs; rescue placement and combat are reduced. Transmission presentation, burrow, bunker garrison commands, and complete campaign AI are unfinished.

## Computer skirmish opening

The arena is an authored 64×64 Badlands-style map with visible walls and passages, not an imported N64 battle map. Each computer side uses a compact Terran worker/Barracks/Marine plan, with up to eight workers and ten soldiers. It gathers mineral trips, pays for construction and training, and fights other hostile sides. It does not use original AI bytecode or artificial periodic resource grants. Eliminate opposing units and buildings to win; lose when all your units and buildings are gone.

Select an SCV with L, use B to build a Barracks near your base, and queue Marines while other workers mine. Extra Depots raise supply; the Command Center trains SCVs. Start with one computer opponent while learning the controls, then try two or three. Air units, other races, advanced tech, expansion bases, and original N64 skirmish balance remain future work.

## Measured performance and checks

The recorded mGBA ARM replay wins both Terran missions consecutively, matches 91 host checkpoints, and averages **29.13 updates/second** at the GBA CPU clock. A three-computer match averages **29.72**. Camera-heavy campaign intervals approach 20 updates/second; saves and cold route searches can hitch. Physical GBA SP testing is pending. See [VALIDATION.md](VALIDATION.md) for the reproducible checks and current snapshot results.
