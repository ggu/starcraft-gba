# N64 parity tech-tree labs: production spike and `tech-terran` (phase 2, first part)

Date: 2026-11-27, on main `8b42d7a7`. This round adds production input to the tape compiler,
from a spike on the N64 command wheel, and the first tech-tree lab, `tech-terran`. That lab
measures the build and train time and the cost of every Terran product the cartridge lets the
human make in Blood Bath. Each GBA-vs-N64 difference below is a GBA defect to fix (owner
decision, 2026-11-26). The baseline holds them only so that `./dev parity` reports them as
KNOWN. Captures, scenario files, tapes and goldens stay in the private fixture home; this page
keeps the controls found, the verdicts and the measured numbers.

## Production input on the N64 (spike)

The spike used exploration captures from a Blood Bath lobby state: human Terran against one
Zerg computer, the other rows closed. Each control below showed in recorded guest RAM, not
only on screen.

| Control | Finding |
| --- | --- |
| Command wheel | A selected unit or building shows its C-button wheel. SCV: C-Left Build, C-Down Build Advanced, C-Up Repair, C-Right Move. Production buildings: C-Right Build, C-Down Lift Off (then Land). The add-ons are on C-Left (Factory, Starport, Command Center, Science Facility) or C-Up (the Science Facility's second add-on); the Science Facility's C-Right opens its research grid |
| Grids | A 3 × 3 grid whose highlight starts on the centre. Each stick push (10 steps at full stick, 30 idle) moves it one cell, and A takes the cell; a tooltip names the cell. A unit grid stays open after training, and B closes it |
| Placement | After a building is picked, its footprint follows the cursor and A places it. A red footprint means blocked: a cliff, another building, or unexplored ground (nothing can be placed in fog the player never saw) |
| Add-ons | The wheel button shows the building and add-on footprint. With the cursor at the building's centre plus (64, 16) (Command Center: plus (64, 0)) the add-on is built in place; anywhere else the building lifts off and flies there |
| Evidence | A worker's order byte (`+0x4D`) reads 30 (PlaceBuilding) one tick after the placing A. The building's row is born when the worker arrives, and the bank is debited then, not at the command. The production queue is five u16 slots at `+0x94` (228 empty) with a head index byte at `+0xA0`, so a new unit goes into slot (units queued so far) mod 5. A unit's row is born when its production starts, and status bit 31 sets at completion |
| Gas workers | After building a Refinery the SCV starts harvesting gas (order 84) and never idles |
| Own targets | B over one of the player's own buildings gives an attack-move (order 14) and no damage: the cartridge has no attack on own units, so the arsenal labs need enemy targets |
| Bank edits | A guest write to the human bank (10,000 minerals and gas) holds. Writing 0 to the computer's bank every 200 steps keeps it from building or attacking for the whole run |

`tools/sc64_tape_compile.py` turns these into closed-loop ops (`build`, `train`, and `wait`
with repeated `pokes`; [`PARITY-HARNESS.md`](../PARITY-HARNESS.md#tape-compiler)). Every press
is preceded by a check that no text box is open, and followed by a check of its evidence, so a
tape that drifts stops with exit 9 instead of building the wrong thing.

## `tech-terran`

One SCV places every building in turn, each started only once it is idle again, then the
add-ons and every unit follow. The run lasts 26,240 source frames, about 94,000 frontend
steps, and each capture took about 9 minutes. Both captures have identical hash chains (the
golden requires it). The golden is `n64-staged` (bank edits).

The SCV places 14 buildings: Supply Depot ×3, Barracks, Engineering Bay, Academy, Bunker,
Missile Turret, Factory, Starport, Science Facility ×2, Armory and Refinery. Then come five
add-ons (Machine Shop, Control Tower, Covert Ops, Physics Lab, Comsat Station) and 13 units:
SCV, Marine, Firebat, Medic, Ghost, Vulture, Siege Tank, Goliath, Wraith, Dropship, Science
Vessel, Valkyrie, Battlecruiser.

Not covered by this lab:
- **Nuclear Silo and nuke:** a second Command Center does not fit on the human's plateau,
  and the expansion below it is unexplored.
- **Research-dependent abilities:** Siege Mode, Spider Mines and the others belong to phase 3.
- **Lift-off and landing.**

### Results

`./dev parity --only tech-terran`: 41 PASS, 17 KNOWN (each line below, accepted in its own
commit), 29 REPORT. The GBA run takes 16 s of one core.

| Check | Result |
| --- | --- |
| Train time, 11 units | PASS: every unit the GBA makes is within ±2 frames (N64 361 vs GBA 360 for a Marine, 2,401 vs 2,400 for a Battlecruiser) |
| Mineral cost, 29 products | PASS for all 26 the GBA makes |
| Gas cost | REPORT; equal wherever both engines made the product |
| Add-on build time (Machine Shop, Control Tower, Covert Ops, Physics Lab) | PASS (N64 600, GBA 599.2) |

### GBA defects (accepted as KNOWN)

1. **Worker-built buildings finish about 2.8 frames early.** On the N64, a building's row
   takes its table time plus 2 ticks from birth to completion. A Supply Depot takes 602 ticks
   and a Barracks 1,202; add-ons take exactly their table time (600). The GBA completes 599.2
   and 1,199.2 frames after the birth. That leaves 11 building lines (Supply Depot, Refinery,
   Barracks, Academy, Factory, Starport, Science Facility, Engineering Bay, Armory, Missile
   Turret, Bunker) just outside the ±2-frame band.
2. **No Valkyrie.** The engine has no identity for it, so train time and cost read `absent`, 2 lines.
3. **No Comsat Station.** It is not in the release roster: `absent`, 2 lines.
4. **Science Vessel refused.** The GBA never takes the Science Vessel at a Starport with its
   Control Tower, with a Science Facility complete, where the N64 trains it: `refused`, 2
   lines (a new marker for products the engine has but will not make; see
   [`PARITY-HARNESS.md`](../PARITY-HARNESS.md#tech-tree-labs)).

### Wall time

`tech-terran` adds one GBA run of about 16 s to `./dev parity`. It runs in the worker pool
beside the skirmish lobbies, so the measured increase of the full run is in the PR.

## Next

`tech-zerg` and `tech-protoss` need their own production spikes: Larva and Eggs, Drones
morphing into buildings, Probe warp-in and Pylon power. After those come the arsenal labs.
With no attack on own units, those use enemy targets, as the coverage map's fallback
describes.
