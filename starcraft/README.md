# StarCraft: GBA Outpost

A compact Terran-versus-Zerg RTS skirmish for the shared 240×160 mode-4 renderer. Defend a small, prebuilt Terran outpost, keep workers mining, train a force, and destroy the Zerg Hatchery. This is a short mission opening, not a ladder start: it begins with a completed Supply Depot, Barracks, five SCVs, and eight guarding Marines. The first reinforcing wave arrives after four in-game minutes, leaving time to establish an attack force. The game has a title screen, persistent save/load, a pause screen, fog of war, a minimap, attack-move, pathfinding, worker construction, queued production, and defensive Zerg waves.

## Controls

- D-pad moves the command cursor. Hold R with the D-pad for faster cursor movement, including while dragging a selection box.
- A selects a unit or structure, confirms a menu choice, issues the selected command, queues production, or places a building. A single tap acts immediately. Tap A twice quickly on the same friendly unit (within about 0.4 seconds) to select all living friendly units of the same type currently on screen. Offscreen units are not included. This shortcut applies to units; building selection and contextual commands keep their existing behavior.
- B opens the command menu. Choose Move, Attack Move, a building, a weapon or armor upgrade, or Stop/Hold with the D-pad, then press A. B cancels the active order mode.
- Tap L to cycle friendly units. Hold L with the D-pad to drag a box; release L to select living friendly units inside it. Dragging works from ground or a selected building. B cancels the box and preserves the previous selection; A is ignored while dragging. A committed drag cancels a pending move/build mode and returns to normal command mode. With a Barracks selected, tap L to cancel its oldest queued unit and refund 75% of its mineral and gas cost.
- Tap R to select the Marine, Firebat, and Medic group. Holding R during a box drag only speeds cursor movement; tap R separately after release to select the group. With a Barracks selected, tap R to cycle its selected training unit.
- START or SELECT opens the pause screen. There, A resumes, B saves, L loads, and R restarts.
- On the title screen, A starts a mission and B loads the last save. On the result screen, A retries and B returns to the title.

Box selection requires a ROM rebuilt from the updated source; an older copied ROM does not include it.

SCVs automatically gather mineral trips of up to eight minerals and return to the nearest Command Center. To build, select an SCV, open B, choose a structure, move to the site, and press A. Barracks require a completed Supply Depot; Academies require a completed Barracks. Refineries can only be placed on one of the two marked geysers. Select a worker and press A on a completed Refinery to assign gas harvesting, or select the Refinery and press A to assign an idle worker.

Command Centers train SCVs. Barracks train Marines by default; select a Barracks and press R to choose Firebat or Medic. Production queues hold five units and reserve supply while training. A completed Depot provides eight displayed supply. The Engineering Bay offers up to two weapon and armor upgrades.

## Fidelity and limits

This is an original-art RTS demake rather than a full StarCraft engine. It uses fixed-size unit/building pools, a compact map, simplified collision, direct routing across open ground, and BFS detours around blocked routes on a 16-pixel grid, with authored map balance. Combat uses the basic unit roles, hit points, armor, costs, supply, build times, and weapon values referenced below, scaled to the GBA's 30 updates per second. Resource trips preserve the eight-mineral harvest amount, with a shorter final trip for depleted patches. Gas uses a small worker pool and two fixed geysers.

It omits campaign triggers, air units, large tech trees, unit abilities, damage-type/size modifiers, pathing influence from other mobile units, multiplayer, and original game art/audio. Firebat splash, upgrades, fog visibility, and Zerg wave behavior are compact approximations for a short handheld mission. Medics do not have a healing order yet.

The opening scenario averaged about 21.92 updates/second in a 240-tick mGBA ARM sample, with pathfinding hitches. Unit timers use a fixed step, so slowdown also slows in-game time. The renderer targets about 30 updates/second when update and drawing costs fit the frame budget. See the [validation record](../docs/VALIDATION.md); physical hardware performance has not been measured.

## Rule references

BWAPI's public stat tables were checked at repository commit `d727fed68558c506163048ea889131d8cbb33915`. Unit identities are in [`UnitType.h`](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/include/BWAPI/UnitType.h); default HP, armor, mineral/gas costs, build times, and doubled supply values are in [`UnitType.cpp`](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/BWAPILIB/Source/UnitType.cpp#L559-L919); default damage, cooldown, and range values are in [`WeaponType.cpp`](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/BWAPILIB/Source/WeaponType.cpp#L152-L216). The extracted reference records are in [`../docs/reference-stats.json`](../docs/reference-stats.json). BWAPI reports supply in doubled internal units and weapon ranges in pixels; the demake stores supply in half-units and maps the 32-pixel original terrain tile to a 16-pixel GBA tile.

OpenBW's mining loop extracts up to eight minerals at a time, including the remaining amount in a depleted patch: [`bwgame.h`](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L2149-L2156). BWAPI stat times and cooldowns are simulation frame counts; this game scales those frame counts by 30/24 to retain their approximate durations at the GBA renderer's 30 updates per second.
