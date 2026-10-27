# Z8 trigger scheduler

`starcraft/sc64_z8_triggers.h` is a standalone scheduler for the 39 triggers
of the authenticated Zerg mission 8 scenario. It is **not integrated**. No
build target includes it, the campaign menus do not offer Z8, and nothing here
claims that Z8 is playable, complete or performance-qualified.

It reuses the private descriptor from `tools/sc64_z8_pack.py`, described in
[the Z8 source foundation](audits/2026-10-27-z8-source-foundation/README.md).
Its row structs use the descriptor's JSON field names. It adds no importer and
no descriptor format.

## Run the checks

The synthetic test is asset-free:

```sh
python3 -m unittest discover -s tests -p 'test_sc64_z8_triggers.py' -v
```

`./dev test` also runs it through its `test_sc64*.py` discovery. To build the
C test directly:

```sh
clang -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic \
  -fsanitize=address,undefined tests/sc64_z8_triggers.c -o build/test-sc64-z8-triggers
./build/test-sc64-z8-triggers
```

An optional private probe checks the real rows. Set `SC64_Z8_DESCRIPTOR` to a
descriptor that `sc64_z8_pack.py pack` wrote under `.local/build/z8-*`. The
probe validates that descriptor with the pack tool's own `validate_structure`.
It renders the rows as C only in a temporary directory, then runs 20,000
frames against a stub world and fails on any fault.

## What Z8 uses

These are the operation types in the authenticated descriptor, with the
operand variants each row actually uses:

| Kind | Opcode (count) | Variants used |
| --- | --- | --- |
| Condition | Command 2 (14) | AtLeast, AtMost; groups 1, 2, 13, 17, 19, 20 |
| Condition | Bring 3 (9) | AtLeast, AtMost; groups 1, 13 |
| Condition | Kill 5 (1) | AtLeast; group 13 |
| Condition | Switch 11 (14) | Set, Cleared |
| Condition | ElapsedTime 12 (17) | AtLeast |
| Action | Victory 1 (1), Defeat 2 (5), PreserveTrigger 3 (13), Wait 4 (10) | — |
| Action | Transmission 7 (6), CenterView 10 (5) | timed presentation |
| Action | PlayWAV 8 (6), DisplayTextMessage 9 (6), SetMissionObjectives 12 (1), MinimapPing 28 (3) | world presentation |
| Action | CreateUnitwithProperties 11 (16) | count 0 (one actor), property number 0 |
| Action | SetSwitch 13 (34) | Set, Clear |
| Action | RunAIScriptAtLocation 16 (8) | `Zer8`, `Ze8T`, `MvTe` |
| Action | SetResources 26 (3) | Set To |
| Action | MoveLocation 38 (2) | unit-anchored |
| Action | SetInvincibility 43 (3) | Enable |

Triggers are enabled for player slot 3 and for Forces 1–3 (selectors 18–20).
FORC places slot 3 in Force 1, slot 1 in Force 2 and slot 2 in Force 3. With
slots 1–3 active, the 39 triggers form 39 instances. Every trigger starts
with zero execution flags and a zero cursor, and no row is disabled.

None of the 16 creation rows names a UPRP property record, and UPUS marks
none in use. "With properties" therefore means the default property path for
Z8. A world callback that applies UPRP overrides still has to handle that path
for later missions.

## Behavior and evidence

The primary evidence is the USA N64 executable, at the private recompilation
recipe pinned in [References](REFERENCES.md). Addresses identify the traced
routines; no native code or generated C is included. The pinned OpenBW
`bwgame.h` (`4b046d5f…`) supports cross-checks for the PC engine. Its
differences are listed where they occur.

The native timing rules below are derived from static control-flow analysis.
The synthetic tests check this scheduler model; they do not constitute a
differential runtime trace against the N64 executable. Native runtime
comparison remains an integration qualification step.

| Behavior | Evidence | Scheduler |
| --- | --- | --- |
| Pass cadence | `0x8008DAEC` decrements a counter each frame; on underflow it reloads 30 and runs a pass. Mission init (`0x8008DFA4` → `0x8008CE00`) sets it to 1. OpenBW also starts at 1 (19983) and reloads 30 (13126). | Passes on frames 2, 33, 64, … |
| Elapsed seconds | `0x8008D4B8` reloads a counter with 15 on underflow and increments the seconds value (`0x800B9368`) that ElapsedTime (`0x80086398`) reads. Init sets the counter to 15. OpenBW compares frames (18893). | `elapsed_seconds = frames / 16` |
| Wait timers | `0x8008D4B8` subtracts the frame step from each waiting owner's millisecond timer. It skips −1, and expires a timer that is no larger than the step. The step is a byte from the speed table at `0x80001424` (167, 111, 83, 67, 56, 48, 42). OpenBW uses 42 and expires only when the timer is smaller (13107–13117). | `frame_ms` is set at init and validated against the table |
| Expiry reschedules | `0x8008CCF8` clears the wait and sets the pass counter to 1. The same frame decrements it, so the pass runs on the next frame. OpenBW sets 0 and runs in the same frame (13116). | Pass on the frame after expiry |
| Owner order | `0x8008DAEC` walks player slots 0–7 in ascending order through the single-player branch of `0x8007E9D4`. It skips finished players. | Ascending owner order; latched owners are skipped |
| Instances | `0x8008E094` appends one instance to an active player's list for each matching enabled selector. Selector 17 is all players, and 18–21 are Forces 1–4. | One instance per (owner, trigger) in trigger order. A duplicate is rejected |
| Per-instance pass | `0x8008D240` skips done instances. If an instance is not executing, it tests conditions (`0x8008D038`), resets the cursor on a match, and then executes. An executing instance resumes at its cursor without rechecking conditions. OpenBW resets the cursor on every execution (13146). | Same as native |
| Conditions | `0x8008D038` visits up to 16 slots in order. It skips disabled rows; an all-zero slot passes and the first false row fails. | Same; descriptor gaps are zero slots |
| Comparisons | `0x800862F0`: 0 is at least, 1 at most and 10 exactly. `0x800863C4`: 2 is set and 3 cleared. Other values are silently false. OpenBW uses 2 for exactly (18815). | Other values are rejected when the program is validated |
| Command/Bring/Kill | `0x80086438`/`0x80086484` count completed actors unless the comparison is AtMost. Bring is false for location 0. `0x80086530` reads a kill census. | `count` callback; Bring location 0 is false |
| Actions | `0x8008D108` sets the executing flag, skips disabled rows, and ends at an all-zero slot or slot 64. A zero return keeps the cursor. | Same |
| Completion | `0x8008D108`: a preserved instance clears flags `0x51` and its cursor; any other instance is marked done (`0x08`). OpenBW matches (19316–19322). | Preserved instances are re-evaluated in a later pass; one-shot instances retire |
| PreserveTrigger | `0x8008071C` sets flag `0x04` on the current instance. | Set wherever the action runs, even after a stall |
| Wait | `0x8008073C`: the action stalls while its owner waits. An armed action then completes. Otherwise it arms the owner's timer with `time` ms and stalls. OpenBW matches (19056–19066). | One wait per owner, with the armed marker on the instance |
| Transmission, CenterView | `0x80080DA8`/`0x80081444` complete with no effect for location 0 or when a player check (`0x8003E258`) succeeds. Otherwise they stall while the owner waits, complete if armed, or start and arm the shared wait. CenterView arms an indefinite wait. Its camera-pan completion (`0x8008CD34`) releases the wait of the player stored at `0x800B00FC`. | `timed` callback: ELIGIBLE, START, FINISH; `sc64_z8_release_wait()` |
| SetSwitch | `0x80081ADC` acts on a mission-wide 256-bit bank: 4 sets, 5 clears, 6 toggles and 11 randomizes. Init clears the bank. | 4, 5 and 6 are implemented; 11 is rejected |
| Victory/Defeat | `0x8008063C` writes 3 to the owner's pass outcome. `0x80080690` writes 2 unless the outcome is already 3. The pass continues. | Victory overrides Defeat in a pass; the outcome is latched afterward |
| Location guard | `0x80080620` returns the location byte; Create, RunAIScriptAtLocation, MinimapPing, SetInvincibility, MoveLocation and the timed actions complete with no effect when it is zero. | Same, without calling the world |
| CreateUnitwithProperties | `0x800819EC` makes `modifier` actors, or one when it is 0, from a fixed RNG seed. It restores the RNG afterward. | `action` callback; `sc64_z8_create_count()` |
| MoveLocation | `0x800840EC` finds an anchor through `0x80080CF0` (the first matching actor, or else the source centre). It moves the destination by the centre difference, then clamps left, top, right and bottom once in that order with 16-bit stores. OpenBW matches (19219–19247). | Mutable rectangles are stored in the scheduler state; `find_unit` callback |
| SetInvincibility | `0x80084DB8` walks actors in the current location rectangle with the unit, player and modifier operands. | `action` callback with the current rectangle |
| RunAIScriptAtLocation | `0x80081CC8` passes the FourCC, the trigger owner and the current location rectangle to `0x8005EAF0`. | `action` callback receives the raw FourCC; only the three Z8 IDs validate |

Every handler that Z8 uses, apart from Wait, Transmission and CenterView,
always returns success natively. A state at rest can therefore stop only at
a timed action, and the restore validator relies on that.

## State and footprint

`Sc64Z8SchedulerState` is **876 bytes** and fixed in size. It holds:

- 64 four-byte instances
- 64 mutable location rectangles (512 bytes)
- the 32-byte switch bank
- eight millisecond wait timers
- outcomes, forces, counters and fault fields

Immutable program rows stay in caller-owned tables. A condition row is 24
bytes and an action row 32 bytes. All work is bounded by 64 instances × (16
conditions + 64 actions) per pass.

An ARM7TDMI Thumb `-O2` freestanding probe of init, tick, restore and wait
release had 4,736 bytes of text, no data or BSS, and no undefined symbols. That
probe is not a ROM-size or frame-rate measurement.

## Failure policy

`sc64_z8_program_valid()` rejects the following:

- an opcode outside the table above
- an unknown comparison or modifier
- an AI FourCC other than `Zer8`, `Ze8T` or `MvTe`
- a nonzero execution flag or initial cursor
- an unsupported enabled-player selector
- an out-of-range location or switch
- rows that are out of order

The world callback contract is strict. A missing callback, an unsupported
result, or any value outside the listed contract records a fault with its
owner, trigger and slot. The current tick then returns 0, and every later tick
returns 0 until init or restore. A faulted state cannot be restored. Effects
that earlier instances completed in the same pass are kept.

## Not resolved

- **Partial pause.** `0x8008DAEC` skips the whole frame only when the value at
  `0x800F0C20` is nonzero and the flag at `0x800CD274` is zero. With that flag
  set, waits and passes advance but elapsed seconds stop. The meaning of these
  flags is unknown, so the scheduler models only "paused" (no change) and
  "running".
- **Player eligibility codes.** The native iterator includes player types 1,
  2, 3, 4 and 7 that are not finished. Victory and Defeat record only for types
  1 and 2. The mapping from OWNR to these codes is not traced, so the caller
  supplies the active owners and filters outcomes.
- **Post-pass resolution.** `0x8008D768` post-processes every player's pass
  outcome after a pass. It was not fully traced. The scheduler latches each
  owner's outcome and leaves the mission result to the caller.
- **Save restore of instances.** Native init `0x8008DE28` clears every
  instance's executing and done bits and its cursor. Whether a native save
  restores or rebuilds trigger instances is not traced. Restoring the exact
  state is an authored GBA policy.
- **Transmission duration.** `0x80080DA8` derives the duration from `time`,
  `modifier` (7 set, 8 add, 9 subtract) and a sound length. An unlabeled global
  flag can make the wait indefinite instead. The `timed` callback supplies the
  duration.
- **Anchor order.** `0x80080C64` returns the first matching actor in a
  location. Its candidate order is not traced.
- **Presentation player check.** `0x8003E258` is applied to the owner's
  player-type byte before Transmission and CenterView. Its meaning is not
  traced; the `timed` callback's ELIGIBLE phase stands in for it.
- **Camera release owner.** `0x8008CD34` releases the wait of the player at
  `0x800B00FC`, which appears to be the local player, rather than the trigger
  owner. The caller chooses which owner to pass to
  `sc64_z8_release_wait()`. That routine also refuses an owner that is not
  waiting, which the native routine does not check.
- **Unmodeled flag paths.** These are rejected rather than emulated:
  - execution flags `0x02` (ignore Defeat), `0x10` (skip waits) and `0x20`
    (paused game)
  - SetSwitch randomize
  - duplicate instances from overlapping selectors
  - Transmission or CenterView eligibility changing while armed
  - the multiplayer player-order table in `0x8007E9D4`

## Typed world bridge (not bound to the game)

`starcraft/sc64_z8_world_adapter.h` now supplies callback implementations
against a typed ordered view of live actors/buildings, source force ownership,
wallets, and explicit engine services. It implements Command/Bring census and
source-pixel overlap, delegates Kill census to a required per-owner/type
service, dispatches property-aware creation to an explicit engine service,
mutates live invincibility flags, applies SetResources, and routes timed and
presentation actions. Its asset-free sanitizer fixture covers these adapter
contracts. See [Z8-WORLD-ADAPTER](Z8-WORLD-ADAPTER.md).

This bridge is not included in `game.c` and does not create the live view,
provide kill accounting, create actors, render/audio presentation, or persist
mission state. `MoveLocation` searches the supplied actor order, which must be
bound to the native traversal order before it can claim parity. The engine
services for all three AI FourCCs remain unimplemented and fail closed; a
generic AI substitute is not acceptable. Thus this adapter does not make the
mission playable or complete.

## Integration follow-up

The scheduler deliberately does not touch these:

- **Tables.** Generate private ROM tables from the descriptor under an ignored
  path, reusing `sc64_z8_pack.py` validation.
- **Game binding.** Construct the typed view from real game pools, provide a
  per-owner/type kill census, bind creation/presentation/timed services, and
  verify native actor traversal order. Then integrate linked Nydus transit
  and Observer detection.
- **Frame mapping.** Map the GBA update loop to native frames and choose the
  speed-table entry.
- **Saves.** Add the 876-byte state to the shared SRAM save structure and
  bump its generation.
- **Build and menus.** Add a Makefile target or profile flag, and enable Z8 in
  the campaign menus.
- **Qualification.** Run controller, ARM-win and frame-rate qualification as
  described in [Validation](VALIDATION.md).

Each of these changes `game.c`, the `Makefile`, the shared save structures or
the menus. Each needs its own reviewed change.
