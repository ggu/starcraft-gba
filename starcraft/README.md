# StarCraft: GBA Outpost

A compact Terran-versus-Zerg RTS skirmish for the shared 240×160 mode-4 renderer. Defend a small, prebuilt Terran outpost, keep workers mining, train a force, and destroy the Zerg Hatchery. This is a short mission opening, not a ladder start: it begins with a completed Supply Depot, Barracks, five SCVs, and eight guarding Marines. The first reinforcing wave arrives after four in-game minutes, leaving time to establish an attack force. The game has a title screen, persistent save/load, a pause screen, fog of war, a minimap, attack-move, pathfinding, worker construction, queued production, and defensive Zerg waves.

## Controls

- D-pad moves the command cursor. Hold R with the D-pad for faster cursor movement, including while dragging a selection box.
- A selects a unit or structure, confirms a menu choice, issues the selected command, or places a building. A single tap acts immediately. Tap A twice quickly on the same friendly unit (within about 0.4 seconds) to select all living friendly units of the same type currently on screen. Offscreen units are not included.
- With a structure selected, A on that structure queues production (or assigns an idle worker to a Refinery). A anywhere else selects what is under the cursor, and A on empty ground clears the selection. With units selected, A on open ground moves them there, and A on a visible enemy attack-moves them onto it. SELECT drops the current selection.
- B opens the command list. It shows each command's icon and mineral cost; unavailable commands are dimmed, and the panel below explains why. Choose a command with the D-pad and press A. The list reopens on the last command chosen, so B, A repeats an order or another copy of the same building. In an order or placement mode, B returns to the list. A building needs no SCV selected: the selected SCV builds it, or else the free SCV nearest the cursor.
- Tap L to cycle friendly units. Hold L with the D-pad to drag a box; release L to select living friendly units inside it. Dragging works from ground or a selected building. B cancels the box and preserves the previous selection; A is ignored while dragging. A committed drag cancels a pending move/build mode and returns to normal command mode. With a Barracks selected, tap L to cancel its oldest queued unit and refund 75% of its mineral and gas cost.
- Tap R (press and release without the D-pad) to select the Marine, Firebat, and Medic group. Holding R to speed the cursor never changes the selection. With a Barracks selected, tap R to cycle its selected training unit.
- START opens the pause screen, as does SELECT when nothing is selected. Choose Resume, Save, Load, or Restart with the D-pad and A, or use the shortcuts: B saves, L loads, R restarts, and START resumes. A save is confirmed on the pause screen.
- On the title screen, choose New Mission or Load Saved Game with the D-pad and A; B or L also loads. On the result screen, A retries and B returns to the title.

The bottom panel always names what A will do at the cursor (for example SELECT, MOVE, TRAIN, DESELECT, ATTACK, or PLACE), and the cursor changes with it: green arrows while a move is armed, a red reticle for attack-move or an enemy target, and a structure footprint during placement. Orders, attacks, and placements leave a short green, red, or gold marker where they were issued.

Box selection requires a ROM rebuilt from the updated source; an older copied ROM does not include it.

An optional build replaces menu scrolling with a hold-B command ring: aim, hold B, press a direction and release. See [`docs/COMMAND-RING.md`](../docs/COMMAND-RING.md).

In the optional race ROMs, the light-blue bar above a Protoss unit or building
shows shields; the lower bar shows HP. An empty blue track means shields are
depleted. The selection panel displays shields as `SH` separately from `HP`.
Shields absorb damage first and regenerate independently of health.

Destroyed units and buildings play a short death effect instead of vanishing.
Infantry and Zerg ground units collapse and leave remains, vehicles and
aircraft explode, Protoss units burst in blue, and buildings explode and
leave rubble for 30 seconds. These original-art effects follow the death
families and timing of StarCraft 64. They are not its frames, and they do
not affect combat or saves.
When a mission ends, the last explosions finish before the result panel
opens; buttons on the result screen still respond immediately.

Select an empty worker and press A on a visible mineral field to begin gathering trips of up to eight minerals; STOP leaves the worker idle. Loaded workers finish their current trip before accepting a mineral reassignment. The current STOP command clears carried resources, so avoid STOP while a worker is carrying cargo. To build, select an SCV, open B, choose a structure, move to the site, and press A. Sites snap to an 8-pixel grid and a Refinery snaps to its geyser. The footprint is green when A will place it, red when the site is blocked, and gold when the site is clear but the build still needs minerals, a prerequisite, or a selected SCV; the panel below names the reason. Barracks require a completed Supply Depot; Academies require a completed Barracks. Refineries can only be placed on one of the two marked geysers. Select a worker and press A on a completed Refinery to assign gas harvesting, or select the Refinery and press A to assign an idle worker.

Command Centers train SCVs. Barracks train Marines by default; select a Barracks and press R to choose Firebat or Medic. Production queues hold five units and reserve supply while training. A completed Depot provides eight displayed supply. The Engineering Bay offers up to two weapon and armor upgrades.

## Fidelity and limits

This is an original-art RTS demake rather than a full StarCraft engine. It uses fixed-size unit/building pools, a compact map, simplified collision, direct routing across open ground, and BFS detours around blocked routes on a 16-pixel grid, with authored map balance. Combat uses the basic unit roles, hit points, armor, costs, supply, build times, and weapon values referenced below, scaled to the GBA's 30 updates per second. Resource trips preserve the eight-mineral harvest amount, with a shorter final trip for depleted patches. Gas uses a small worker pool and two fixed geysers.

It omits campaign triggers, air units, large tech trees, unit abilities, damage-type/size modifiers, pathing influence from other mobile units, multiplayer, and original game art/audio: shots, deaths, unit responses and refused orders (too few minerals or gas, or the supply limit) use short PSG tones. The optional imported StarCraft 64 profiles play the game's own samples instead, including the race advisor's refusal lines; see [Development](../docs/DEVELOPMENT.md#optional-starcraft-64-sound-effects-and-music). Firebat splash, upgrades, fog visibility, and Zerg wave behavior are compact approximations for a short handheld mission. Medics do not have a healing order yet.

Combat effects use small authored weapon-family animations for the implemented attacks. They are presentation only: attacks still apply damage immediately, and the short effect timing does not reproduce the original game's projectile or IScript behavior. In Protoss profiles, unit and building overlays show shields in a separate blue bar above health, and the selection panel reports current shields for shielded actors.

The opening scenario averaged about 21.92 updates/second in a 240-tick mGBA ARM sample, with pathfinding hitches. Unit timers use a fixed step, so slowdown also slows in-game time. The renderer targets about 30 updates/second when update and drawing costs fit the frame budget. See the [validation record](../docs/VALIDATION.md); physical hardware performance has not been measured.

## Rule references

BWAPI's public stat tables were checked at repository commit `d727fed68558c506163048ea889131d8cbb33915`. Unit identities are in [`UnitType.h`](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/include/BWAPI/UnitType.h); default HP, armor, mineral/gas costs, build times, and doubled supply values are in [`UnitType.cpp`](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/BWAPILIB/Source/UnitType.cpp#L559-L919); default damage, cooldown, and range values are in [`WeaponType.cpp`](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/BWAPILIB/Source/WeaponType.cpp#L152-L216). The extracted reference records are in [`../docs/reference-stats.json`](../docs/reference-stats.json). BWAPI reports supply in doubled internal units and weapon ranges in pixels; the demake stores supply in half-units and maps the 32-pixel original terrain tile to a 16-pixel GBA tile.

OpenBW's mining loop extracts up to eight minerals at a time, including the remaining amount in a depleted patch: [`bwgame.h`](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L2149-L2156). BWAPI stat times and cooldowns are simulation frame counts; this game scales those frame counts by 30/24 to retain their approximate durations at the GBA renderer's 30 updates per second.


Gas buildings can be placed by selecting a worker, opening the field menu with
B, choosing REFINERY / EXTRACTOR / ASSIMILATOR for its race, and pressing A over
the geyser. Placement and its preview snap to the geyser center when the cursor
hits its body. With a worker selected, press A on a completed friendly gas
building to gather gas. Harvesting workers show a blue/cyan mineral cue or a
green gas cue beside their sprite while loaded.
