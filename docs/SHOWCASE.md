# Showcase scenes

Build the optional filming ROM after preparing the private P7 inputs:

```sh
make test-sc64-showcase
make showcase
```

The output is `build/StarCraft-Showcase-GBA.gba`. The ordinary menu opens
first. On HOME, hold **R**, add **L**, then tap **Select** while keeping both
shoulders held. Release the buttons when SHOWCASE SCENES appears. Choose a
scene with Up/Down and A. This hidden menu is enabled only in the showcase
build; canonical campaign ROMs keep it disabled.

Scenes start paused so recording can begin before any action. A starts or
resumes, Start pauses, and R on the pause screen resets the same setup. Select
returns to the scene menu at any time. B returns to scenes from pause/results;
A replays from a result. Regular selection, camera and commands work during
play. No showcase path writes a save or records campaign completion, and the
showcase build has its own ROM game code (`SCSH`).

| Scene | Starting setup |
| --- | --- |
| Carrier Approach | Three Carriers, four Scouts and six Zealots approach twelve Hydralisks and an Ultralisk. |
| Siege Line | Six deployed Siege Tanks face a rush of 32 Zerglings. |
| Terran Last Stand | Workers fall while the last Command Center takes its final shots. |
| Zerg Last Stand | The same climax around the final Hatchery. |
| Protoss Last Stand | The final Nexus falls, with ordinary shield regeneration included. |
| Protoss Crossfire | Zealots, Dragoons and Scouts clash with Hydralisks and Zerglings. |
| Three Way Battle | Terran infantry, a Zerg swarm and a Protoss squad fight one another. |
| Base Under Siege | Marines advance on a defended Zerg outpost with Tanks holding position behind them. |

These are staged starting positions running the current game simulation.
Carrier Interceptor combat is unfinished: Carrier Approach is explicitly
labeled a preview, with escorts doing the fighting. Tank weapons use the
current adaptation's combat. The current build includes authored weapon-family
shots and impacts instead of the earlier common straight-line tracer; these
visuals preserve the existing damage timing. These scenes do not demonstrate native StarCraft
weapon or campaign parity.

Without player intervention, each last stand ends through ordinary damage
and the normal loss rule at update 150: five seconds on the game's 30-update
clock. The setup fixes HP and initial cooldowns; it does not force a timed
destruction or defeat. Wall-clock duration depends on actual frame rate.
The map is a flat, fully revealed filming arena, with economy AI disabled so
it does not rebuild or change the setup.

For a controller-only mGBA replay, captures and timings:

```sh
python3 tools/sc64_showcase_smoke.py --out .local/build/showcase-recording-check
```

Use a new output folder for each run. Captures, generated inputs, ROMs and
saves remain local and must not be committed.
