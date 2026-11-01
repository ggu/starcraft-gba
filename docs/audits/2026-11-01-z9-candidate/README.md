# Z9 candidate profile (2026-11-01, not qualified)

Zerg mission 9, "The Invasion of Aiur", now runs in an explicit `zerg-z9`
candidate. It is the Z1–Z8 candidate plus Z9 as the ninth campaign entry,
with code `SCZ9`, title `SC ZERG Z9` and generation-48 saves. The canonical
`zerg` alias and the existing `zerg-z3`…`zerg-z8` ROMs are unchanged (hashes
below). Z9 has no ordinary-input win, ARM replay or timing yet, so this is
not a mission qualification.

## Source

The private descriptor comes from `tools/sc64_zerg_late_pack.py pack
--mission Z9`. It reads the retained scenario from the user's ROM through the
pinned extractor commit `09b57f78…` and verifies against a per-mission
contract:
- global catalog entry 20, map path `008/01C`;
- CHK SHA-256 `0eff2a2e…`;
- descriptor fingerprint `38921cb2…`.

`tools/sc64_zerg_late_program_pack.py` renders the descriptor into ignored
ROM tables at `build/generated/z9/sc64_z9_program_generated.h`:
- 14 triggers, 16 conditions, 54 actions and 64 locations;
- active owners 2, 3, 4 and 7, with forces 4, 1, 3 and 2;
- the PUNI permission bitmap.

The map, with 371 retained rows, was already in the shared staged race-map
header behind `SC64_Z9_DATA_ENABLED`.

The program, as decoded from the authenticated rows:
- **Defeat:** the human commands no units (T0).
- **Retrieval:** a human Drone that enters location 2 at the Khaydarin
  Crystal formation is removed (T3). After presentation, T3 sets a
  600-second countdown and switch 0.
- **Return:** when the countdown expires, T4 recreates a human Drone,
  re-centres location 3 on it and creates the crystal (raw 219) there.
- **Victory:** the crystal is brought to location 1, the hive cluster (T1).
- **Towns:**
  - Slot 2 (`Zer9`, 5,000 resources) starts at once.
  - Slot 7 (`Z9ly`/`Z9ry`, 8,000) starts when the human enters location 4, 5
    or 23.
  - Slot 4 (`Z9lo`/`Z9ro`, 10,000) starts when the human enters location 25.
  - Both later towns also run `VluA` at several locations.

## Engine additions (shared with Z10)

- `starcraft/sc64_zerg_late_triggers.h` is the Z8 scheduler with the rows Z9
  and Z10 add. It reuses Z8's row types, cadence and helpers but keeps its
  own state and dispatch, so the Z8 scheduler and its saves are untouched.
  The additions:
  - Countdown (condition 1) and SetCountdownTimer (action 14). The timer
    decrements on the elapsed-second clock and stops at zero; this cadence
    is authored, not traced.
  - Deaths (condition 15) and RemoveUnitAtLocation (action 25) through
    world callbacks.
  - RunAIScriptAtLocation checks the program's own FourCC list.
- `starcraft/sc64_zerg_late_runtime_adapter.h` binds either mission to the
  live pools through the Z8 world adapter. Z8 is never active with Z9 or Z10,
  so they share its world-view object; the EWRAM reserve requires that.
- Raw 173 (Khaydarin Crystal formation) gets a `SC64_Z9_RUNTIME`-only
  building identity with pinned BWAPI facts: 4×3 tiles, 64/48/63/47 source
  pixels, invincible.
- Raw 219 (the crystal powerup) maps to the Z2 Young Chrysalis powerup
  actor. It keeps raw 219 on the actor, cannot collide or be selected, and is
  invincible.

Authored parts, all documented in the code:
- Transmission lasts 4,000 ms, CenterView snaps the camera, and
  presentation actions have no world effect (as in Z8).
- **Crystal carry:** the crystal follows its carrier Drone, as Z2's powerup
  does. A loose crystal is picked up by the nearest human Drone within
  16 engine pixels. A carrier that dies drops it.
- **RemoveUnitAtLocation** clears matching units without a death, using the
  P5/T5 removal pattern. Modifier 0 removes all matches.
- **AI:** each town script starts that town's authored backend, the same
  policy as Z8. It queues paid Gateway, Stargate and Robotics products from
  the town's wallet and sends attack-move waves at the human. `VluA` is
  accepted with no effect; its native meaning is not established. This is
  not a decode of the native AI programs.

## Saves

`starcraft/sc64_zerg_late_state.h` keeps a 160-byte compact form of the
880-byte scheduler. It holds the countdown, the three town wallets, the AI
bytes and the crystal carrier. It fits the 180-byte shared Zerg union, so
`RaceSave` stays 16,296 bytes. The remaining union bytes must be zero.
Restore rebuilds derived fields from the program and setup, then validates
the result before use. Candidate expansion borrows the existing route
scratch union.

## Evidence

Run at base `72b6de1f` (`codex/p8-playable-20261029`) plus this change.

- `make zerg-z9` links:
  - BSS end `0x0203df88`, IWRAM end `0x03004bf0`, within the `0x0203e000` /
    `0x03005000` guards;
  - 120 bytes of BSS spare;
  - ROM SHA-256 `db82a7f1…`, with the Z8 sprite bank and the inputs listed
    below.
- `make test-zerg-z9` (ASan/UBSan host fixture):
  - Z9 is launched through the ordinary campaign menu as the ninth Zerg
    entry, with no unsupported rows.
  - It checks the binding: 14 instances, active mask `0x9c`, forces, the
    human's starting units, the invincible neutral formation and the town
    Nexus owners. The authored FourCC constants match the authenticated
    action rows.
  - The opening triggers run: 750/750 human resources, the three town
    wallets, and only slot 2's town started.
  - A human Drone placed in location 2 (a fixture-only state write) is
    removed by T3, and the countdown starts at 600 seconds.
  - An ordinary pause-menu save with the countdown running reloads from the
    title screen into a fresh game and matches the uninterrupted run for
    3,000 updates.
  - Corrupt candidates are rejected without changing live state: instance
    flags, a non-zero union tail, AI reserved bytes, the countdown limit and
    the carrier slot. A save with a mismatched source owner is also rejected.
  - From that save, a fixture write expires the countdown. T4 creates the
    Drone and crystal, and the Drone picks the crystal up.
  - A second pause-menu save, with the crystal carried, matches for 1,000
    updates.
  - The carrier placed in location 1 (fixture-only) completes T1's Victory,
    and the mission is marked complete.
- The asset-free `make test-sc64-zerg-late-state` checks the scheduler
  additions on an authored program and round-trips the compact state on all
  3,000 ticks. `tests/test_sc64_zerg_late_pack.py` covers the descriptor
  contract and the table renderer.

## Byte identity

To be filled from the final run.

## Not qualified

- **No ordinary-input host controller win.** The fixture moves the Drone and
  carrier by state writes and shortens the countdown.
- **No input-only ARM replay and no fresh-process SRAM continuation.**
- **No ARM frame timing.** With 371 map rows and three towns this is the
  heaviest Zerg map so far, and it is unmeasured.
- **Sprite art:** the default bank is the Z8 bank. It carries every other
  Z9 actor, including the Archon, Shuttle, Arbiter, Carrier, Fleet Beacon and
  Arbiter Tribunal, but has no art for raw 173 or raw 219. The renderer draws
  its generic fallback for those two.
- **Authored AI, countdown cadence and crystal carry** have not been audited
  against the native AI programs or a native trace.
- Mission text, sound, native kill/death ledgers and physical-GBA behavior
  are not claimed.
