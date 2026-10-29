# P9 "Shadow Hunters" candidate — in progress (2026-10-29)

This records the P9 work so far. **P9 is not qualified yet.** The runtime,
menu entry, save identity and AI backend are wired, and the source program
runs without faults under ordinary input. The host controller has not yet
won the mission, so there is no ARM replay or SRAM qualification.

The `protoss-p9` candidate builds on the unmerged P8 candidate (#151). It
adds P9 as the ninth campaign entry, with code `SCP9` and generation-45
saves. Canonical Protoss stays P7: its rebuilt ROM is byte-identical to the
base. The P8 host replay on this branch still matches all 68 observations of
the P8 qualification run.

## What is wired

- **Runtime.** `starcraft/sc64_p9_runtime_adapter.h` validates the
  authenticated compact program: 12 triggers, 34 actions, 17 conditions.
  - It runs one runner per executing owner: the human (slot 7, force 1), the
    Red Zerg (slot 0), the Baelrog brood (slot 6) and the Leviathan brood
    (slot 5).
  - The runner follows the shared P8 cursor, pending and blocked contract,
    with 16-bit trigger masks.
  - New source behavior P8 did not need:
    - **PreserveTrigger:** a completed trigger re-arms for a later pass.
    - **KillUnitAtLocation:** kills all matching actors, checking the whole
      set first.
    - **MoveUnit:** moves all matching units to the destination centre;
      native placement is not modeled.
    - **Force-wide SetResources:** applies per slot, including the
      rescuable player in force 1.
    - **Building create:** creates the Cerebrate respawn.
- **Victory logic.** Each Cerebrate that dies while Zeratul is inside its
  location sets a switch. Victory needs both switches. A Cerebrate that dies
  without Zeratul present is recreated by the source PreserveTrigger rows,
  which also kill and move units in its area.
- **Rescue.** Zeratul and Fenix are rescued on contact. Native rescue
  transfers ownership, so they take the human source slot. The source
  "current player Zeratul" census needs that.
- **Saves.** The P9 state is 128 bytes in the existing 180-byte Protoss
  mission union, so the save record keeps its size.
- **AI.** `starcraft/sc64_p9_ai.h` is an authored backend for `Pro9`, `Pr9W`
  and `Pr9Y`, built like P8's. It produces Zerglings, Hydralisks and
  Mutalisks from larva through the native-shaped request heap and admission
  gate. Only `Pro9` sends waves. Native script behavior is not claimed.
- **Bindings.** P9-gated bindings add raw 46 (Defiler) and raw 136 (Defiler
  Mound), with pinned BWAPI dimensions. Defiler spells are not implemented.
- **Shared bridge.** Without Z7, the Z8 pool bridge now falls back to the
  pinned BWAPI `unitDimensions` table for actors without a feature fact,
  which covers P9's Zerg roster.

## Controller status

`tests/sc64_p9_campaign_run.c` uses ordinary inputs from the title screen.
Its policy:

- Rescue the heroes and build a Nexus by the start's mineral line.
- Build Gateways on a spaced lattice, a Forge and cannons by Fenix's shelter.
- Order the army by double-tapping type groups, so Fenix and Zeratul are
  never part of army orders.
- Keep Zeratul behind the front, and send him into each Cerebrate's location
  only when it is nearly dead and its Sunken Colonies are gone.

Several runs killed the Baelrog Cerebrate with Zeratul inside, which set
switch 0 and suppressed the respawn as the source requires. Other runs
showed the PreserveTrigger respawn working when he was absent. No run has
yet also killed the Leviathan with Zeratul present. The remaining failures
are controller-policy problems:

- the start field (about 8,000 minerals) runs out;
- Pylon placement stalls supply;
- Zeratul is lost to the Leviathan's defenders.

Native Zeratul is permanently cloaked. P6's cloak and detection services are
gated to P6, so here he is an ordinary target.

## Not done

- A host controller win, then the input-only ARM replay and the
  fresh-process SRAM save and continuation (same tooling as P8:
  `make test-sc64-protoss-p9`, `make sc64-p9-arm-replay`).
- Zeratul's cloak, Defiler spells and the P7-only abilities in P8/P9.
- Docs claims in `RACE-CAMPAIGNS.md` and `VALIDATION.md`. Nothing is claimed
  for P9 until it qualifies.
