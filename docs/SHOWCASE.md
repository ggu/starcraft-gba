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

## Unit facing

The showcase build also turns on `SC64_UNIT_FACING`, which draws units in the
32 StarCraft directions instead of only the north-facing frame. Before
building, `tools/sc64_unit_facing.py` imports frames 0–16 of each unit's art
into `.local/sc64/facing/generated/`; the other 15 directions are mirrored.
A ground unit faces the path waypoint it is walking to, and a flyer faces
its move target, so the 8-way movement steps do not jitter. A unit turns
toward its target when it fires and keeps that facing while its weapon cools
down. Siege Tanks draw their hull and turret subunits in the same direction
(raw 5/6, or 30/31 when sieged).

Facing is presentation-only. It is kept in one byte per actor slot that only
the drawing code reads, and it is not saved: after a load,
units face north until they move or fire. With per-update rendering, the
host showcase simulation ends every scene with byte-identical unit and
building state whether facing is on or off. The facing frames add about
416 KiB of ROM. The showcase ROM keeps the IWRAM end it has without facing
(0x030049e8) and ends BSS at 0x0203dfd8, under the 0x0203e000 guard.
Canonical ROMs leave the flag off. A tank cannot yet aim its turret
independently while driving.

## Death animations

The showcase build also turns on `SC64_DEATH_SOURCE_ART`: units and
buildings play the source death frames that `tools/sc64_death_sprites.py`
imports into `.local/sc64/death/generated/` (see
[Death animations](DEVELOPMENT.md#death-animations)). Deaths are
presentation-only and do not change simulation or saves. They draw through
`SC64_SPAN_BLIT`, which skips transparent pixels. The death frames and their
span streams add about 1.8 MB of ROM and move the IWRAM end from 0x03004bb0
to 0x03004d00; BSS still ends at 0x0203dfd8. `make -B showcase
SHOWCASE_DEATH_FLAGS=` builds the authored deaths for comparison.

For a controller-only mGBA replay, captures and timings:

```sh
python3 tools/sc64_showcase_smoke.py --out .local/build/showcase-recording-check
```

Use a new output folder for each run. Captures, generated inputs, ROMs and
saves remain local and must not be committed.
