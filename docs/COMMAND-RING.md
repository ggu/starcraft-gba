# Hold-B command ring

`SC_COMMAND_RING` is an optional control scheme for the default original-art
game. It is off in `build/StarCraft-GBA.gba`; build the variant with:

```sh
make ring
make test-command-ring
```

The output is `build/StarCraft-GBA-Ring.gba` (game code `SCGR`).

## Controls

1. Aim the cursor where the order should go.
2. Hold **B**. After 0.2 seconds a ring of up to four commands opens around
   the cursor, and the command panel names the page.
3. Press a direction to highlight a command. A direction that leads to a
   submenu opens it at once.
4. Release **B** to carry out the highlighted command at the cursor.

| Selection | Up | Right | Down | Left |
| --- | --- | --- | --- | --- |
| Units | Move | Attack-move | Stop | Build ▸ (workers only) |
| Build page | Supply Depot | Barracks | Refinery | Tech ▸ |
| Tech page | Academy | Engineering Bay | | |
| Barracks | Marine | Firebat | Medic | |
| Command Center | SCV | | | |
| Engineering Bay | Weapon upgrade | Armor upgrade | | |

Each building and training slot shows its mineral and gas cost, in red when
it cannot be afforded. While a training or upgrade slot is highlighted, **A**
queues one more without leaving the ring. **L** backs out of a submenu.
Releasing **B** with nothing highlighted cancels. Building slots open the
usual placement preview at the cursor, and **A** places it.

A short tap of **B** with no direction still opens the classic command list.
As with that list, the simulation waits while **B** is held. Experienced
players can flick: a release within 0.2 seconds never draws the ring, so
"hold B, Right, release" is a single attack-move gesture.

## Behavior and limits

The ring calls the same order, placement, training and research functions as
the command list. `tests/command_ring_run.c` checks each ring command against
the list path for the same command. After 240 simulation updates, units,
buildings, resources, supply and research must match byte for byte. It also
covers tap, hold-to-cancel, L back, the worker-only build page and order-mode
cancelling.

Press counts in the scripted before/after captures (button presses, plus
D-pad presses made inside a menu; cursor aiming is the same on both sides):

| Task | Command list | Ring |
| --- | --- | --- |
| Select the army and attack-move | 5 | 3 |
| SCV builds an Academy | 9 | 6 |
| Barracks trains a Firebat, then a Marine | 6 | 5 |

Training saves only one press. Its gain is that every option and its cost is
visible, instead of cycling a hidden type with **R**.

The ring covers the default Terran command set only and fails to build with
the imported race profiles (`SC64_RACES`). There, the menus change per
mission. The ring's state is five bytes and is not saved. It does not yet
change the pause key, add control groups, or select units.
