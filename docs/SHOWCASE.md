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

## Field controls

Every race-engine ROM (Showcase, the Terran, Zerg and Protoss ROMs, unified
and the race test profiles) builds with `SC64_FIELD_CONTROLS`
(`starcraft/field_controls.h`), the race-engine port of the default
profile's controls and menus:

- With units selected, A on open ground moves them there and A on a visible
  unit or structure the engine treats as hostile (`owners_hostile`, so every
  skirmish opponent) attack-moves onto it. A selected worker over a friendly
  gas building still gathers gas first. SELECT drops the selection; with
  nothing selected it pauses, as before. With a structure selected, A trains
  only when the cursor is on that structure and clears the selection
  elsewhere.
- R acts on a clean tap; holding R to speed the cursor keeps the selection.
- B opens the framed order list: each row has an icon and its mineral cost,
  rows that cannot be used are dimmed, and the HUD names the highlighted
  command and its gas cost, or why it is unavailable (missing structure,
  minerals or gas short by an amount, no free worker, nothing selected,
  fully upgraded). Contextual rows (Nexus, Cannon, Lair, Spire, Creep
  Colony, spells, transports, lift/land) keep their places. The list
  reopens on the last command, so B, A repeats it. The Showcase draws this list as a command card (below).
- Placement snaps to an 8-pixel grid; gas structures snap to their geyser.
  The footprint is green when A places it, gold on clear ground that still
  lacks tech, funds or a worker, and red when the ground rejects it, with the
  reason (blocked, Pylon power, creep, geyser) in the HUD. No worker needs
  to be selected: the selected one builds, or else the nearest free one.
- Armed orders change the cursor (green arrow for Move, red reticle for
  Attack Move) and the HUD title colour; the hint row names what A does.
  Orders and placements leave a short green, red or gold marker.
- Pause is a list (Resume, Save, Load, Restart) with the old one-key
  shortcuts, a clock, "game saved"/"no save found" notices, the mission
  objective and a controls reminder. The result panel shows time, units
  and buildings, and offers the next campaign mission when there is one.

The StarCraft 64 title and briefing screens keep their own navigable
menus, and Showcase scenes keep their own pause and result panels. All of
it is presentation and input routing: no saved state is added. The HUD
resolves the A action once per simulation update rather than per drawn
frame, because crowded scenes draw several frames per update; a scene
restart or a load clears that result, since both jump the update counter.

## Command card

The Showcase build also turns on `SC64_COMMAND_ICONS`, which replaces the
text order list with StarCraft's command card. Before building,
`tools/sc64_command_icons.py` imports 41 command buttons from the verified
ROM's icon sheet into `.local/sc64/command-icons/generated/` (about 4 KB):
structure portraits by unit id, the Move, Stop, Attack, Unload and Lift/Land
buttons, and the upgrade and spell buttons named by `upgrades.dat` and
`techdata.dat`. The importer checks each order and spell icon against the
ROM's own button sets and each upgrade against its stat_txt name. It keeps
the icons at their native size of up to 20 by 20 pixels, in three shades.

- B opens the card bottom right: Move, Stop and Attack on the top row, then
  the list's other commands three to a row in their usual order. Buttons are
  green when usable, grey when not, and gold and framed when highlighted; a
  red pip marks a command that is only short of minerals or gas.
- The D-pad moves on the card. Left and right wrap within a row, up and down
  within a column, so B, Left, A arms Attack Move.
- The HUD shows the highlighted command's button beside its name, cost and
  explanation, and keeps the button while a move, attack or placement is
  armed.

The card only changes drawing and D-pad routing: the list's row numbers,
B, A repeating the last command, and every order and build path are
unchanged. A list longer than 15 commands keeps the framed text list. Other
race ROMs keep the text list.

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

For a controller-only mGBA replay, captures and timings:

```sh
python3 tools/sc64_showcase_smoke.py --out .local/build/showcase-recording-check
```

Use a new output folder for each run. Captures, generated inputs, ROMs and
saves remain local and must not be committed.
