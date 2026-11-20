# T6 from HOME on the normal Terran cartridge

The retained T6 host route did not reproduce on the KODAK Terran ROM.
At its SCV queue checkpoint (tape call 1,790), the guest had no selected
building, the Command Center queue was empty, and the controller remained
in Build mode. The prior B menu helper directly reset `menu_cursor` in the
host process. That reset was absent from the tape, so the same B, Down, A
sequence selected a different command on the cartridge. Later the guest
never built the expected Refinery or Factory and did not win.

The host controller now uses ordinary Up or Down to return to Move after
opening the menu. Both path-grid planning helpers restore the route state
after their queries. An ASan/UBSan run of the production Terran flags plus
the host probe entry and `ROUTE_PURITY_DEBUG` verified the route hash
between updates and regenerated a win: call 23,849, frame 23,536, with
Raynor and two Dropships in the goal, Norad alive and no faults. No game
runtime or save format changes are needed.

The physical replay starts from fresh HOME on the unmodified KODAK
Terran cartridge (runtime `cbba1c5`, ROM SHA-256
`0f5e893037f71860c66112920e7195f5d7e2db5b4aa7f9e64e222689419de3d8`).
It selects Norad II, reads all 15 briefing pages, and deploys at call 69.
The 1,059 host gameplay transitions use only physical buttons. Thirteen
in-game dialogue pages take two additional calls each; each press and
release leaves the simulation frame unchanged. All 42 host checkpoints
match across 37 guest fields, including the paid SCV, refinery and factory,
the rescue mask, and the transport and goal counts. Two host-local
controller indices are not guest state and are excluded.

The cartridge wins at physical call 23,944, frame 23,536. Raynor and both
Dropships reach the goal, Norad survives, the primary objective and mission
completion bits are set, and there are zero trigger faults. Average cost
is 1,002,317 guest cycles per update (16.74 emulated updates per second);
the peak is 25,853,365 cycles. This qualifies the route from HOME, not
physical GBA speed or fresh SRAM continuation.

- Host tape SHA-256: `59e1aa7c6524c9cf1b17d31b57858ee0e2d28c8bd0b16ee10dc3233722556bfe`.
- Physical receipt SHA-256: `6bf6180b590954b836430cd23da435bcb7da4b76bfb3780a675fac4a49ef8ecd`.

## November 20 fresh SRAM and result autosave

The normal Terran build for `cd833c8c` uses the same controller tape.
The observer opens Norad II from HOME, reveals all 15 briefing pages and
deploys with Start. It reads checkpoints and feeds ordinary GBA buttons
through the port-free mGBA observer. At tape call 19,659, frame 19,466,
Raynor is in the first Dropship and no unit or building is selected.
Start opens the pause menu and B saves the live game in journal bank 0.
A fresh process presses L at HOME to load the same observed world,
mission state and cursor. The camera reconstructed from that cursor
matches at this cut.

The next tape sample presses L to select the Dropship. Continuing
directly from HOME suppresses that edge because the load itself used L.
The observer pauses with Start, releases it, and resumes with Start.
This samples the release of L without advancing the saved simulation.
The uninterrupted and corrected fresh paths select slot 5 on tape call
19,661 and reach the source goal at the same coordinates with the same
remaining health on call 20,543. This is a controller replay correction.

The run replays 23,849 gameplay calls and acknowledges all 13 authored
dialogue pages. All 42 observations match, including nine after the
reload. The checkpoint observer verifies 34 recorded state values and
checks the campaign and per-player trigger clocks. The HOST tape leaves
the Starport and Control Tower indices unset. The guest observes those
buildings in slots 74 and 75 after production.

At frame 23,536, Raynor and both Dropships are in source location 2,
Norad survives, and the primary objective and completion bits are set
without trigger faults. The second journal record is valid and the
original pause record remains byte-identical. A third process loads the
frozen win with Dropship slot 6 still selected and exports identical
SRAM. A separate T10 check with the same ROM and observer restores
frame 2,276 and reloads the frozen victory at frame 3,361.

The T6 run averages 1,092,086 guest cycles per main loop, or 15.36
emulated updates per second across 23,952 controller calls. This includes
presentation, menus, save and load. Physical GBA performance remains
unmeasured.

An observer sampling individual updates measured the peak on the result
autosave at tape call 23,849 and frame 23,536. It used 24,630,198 guest
cycles on the uninterrupted route and 54,160,869 guest cycles on the
fresh route with the pause record present. These counts cover the whole
update. Attribution within the journal path still needs an instruction
profile.

Use the pinned local inputs to repeat the check.

```sh
python3 tools/sc64_t9_normal_replay.py --chapter 6 --save-tick 19659 \
  --trace "$T6_TRACE" --rom "$TERRAN_ROM" --elf "$TERRAN_ELF" \
  --runner "$PHYSICAL_OBSERVER" --out "$NEW_RECEIPT_DIR"
```

| Input or receipt | SHA-256 |
| --- | --- |
| Normal Terran ROM | `8abd3e731fd6db1c1c0ff449ea1426d4214213f0990c5613a3b7158b3eb611c4` |
| Matching ELF | `357b901bdb96d325fe4388650ad2ea5d97b7ce801a7a8de14f278a913ce2e2db` |
| Physical observer | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| HOST tape | `59e1aa7c6524c9cf1b17d31b57858ee0e2d28c8bd0b16ee10dc3233722556bfe` |
| Replay harness | `d0ad3fa7b8f692330dbe93711f25cf59ffe5b34af825fd2eded744538c9f097c` |
| Checkpoint observer | `0b13fc5abebb43f4760ca54e16187191b46e7df42b7a9c109e09146994a27e8c` |
| Pause SRAM | `f6bab6c2cdde4397e90621f1045aca1a01b6631ea87aa8dc8fb6d2778b636a1c` |
| Victory SRAM and fresh readback | `af0fa049ba02103ef3944a3f8f324b45c9eb0abc5b8db23318d70969713a87bc` |
| T6 replay and both journal checks | `de8db15b15b9b551cf9285cdc79beba35ab0e21523b437443ca1c5ab844c3041` |
| T10 regression | `5d80269aed0f6a5315e9b23340dc63931094f404e24995e50c2ce61a4dd5b991` |


## Compact unit read timing

The normal Terran ROM at `cd833c8c` reads each field of its 38-byte
compact unit rows one bit at a time. Trial `8bc3c7ce` calls the
shared bounded byte reader. Every record keeps the same wire format.
The writer, semantic validators and older-bank fallback use the
same paths.

The two ARM builds use the same prepared inputs and retain the same
RAM endpoints. Both load the verified frame 19,466 pause bank through
HOME and run the next 4,190 ordinary gameplay calls with the same input
and dialogue handling. All 27 actor, selection and dialogue observations
match. Each produces the exact previously verified victory image,
retains the original first bank, and re-exports the same SRAM from
the frozen-result load.

| Measured update | Control cycles | Trial cycles | Reduction |
| --- | ---: | ---: | ---: |
| HOME load of pause journal | 39,232,491 | 30,514,031 | 22.22% |
| Victory autosave with the pause bank present | 54,160,869 | 40,885,763 | 24.51% |
| HOME load of frozen victory | 41,585,079 | 32,874,369 | 20.95% |

The table measures complete emulator updates. It captures three different
save operations and leaves hardware timing separate. Compile-time and
sanitizer checks cover old unit rows, 10,000 round trips, malformed
owner and reserved bits, the other compact codecs and journal fallback.
The composed ARM gate at `727c766f` preserves the ROM and RAM
guards on all four profiles. Z10, P10 and Showcase are byte-identical
to the `6ea84a03` controls. Normal Terran produces identical
32 KiB journals during active Bunker construction and with four
passengers loaded. Both fresh HOME continuations match their control
worlds, resource banks and assignments through 600 updates. The ARM
matrix receipt SHA-256 is
`4a7b1cbb885a3c2f36df264b4060a650ecfef1b78a1b23e292fe5b1961c2201a`.

The replay comparison receipt SHA-256 is
`8c76801a0d3753b742bc0da11448a33a5e39ba9318f2d1eb3997e21b79f41aea`.


### Instruction attribution at victory

A read-only instruction trace of the original Terran ROM reproduces
the same paused-bank victory and frozen-result SRAM. Its complete
input-to-input interval takes 54,161,500 cycles. The game loop's
counter measures 54,160,869 inside that interval. The instruction
histogram, recorded stack totals and emulator clock delta agree.

| Function on the original ROM | Inclusive cycles | Calls |
| --- | ---: | ---: |
| `race_save_game` | 51,792,193 | 1 |
| `sc64_unit_save_decode` | 28,088,518 | 2,881 |
| `sc64_packed_unit_get` | 14,240,077 | 25,936 |
| `sc64_t6_runtime_program_valid` | 5,896,350 | 4 |

The counts are nested. The T6 scenario adapter checks the immutable
program before calling the generic runtime validator, which checks
the same program again. Reuse the verified program within the
adapter call while retaining its mutable-state checks and the generic
validator's independent checks. Profile the composed runtime to
measure the remaining journal cost. The profile receipt SHA-256 is
`4b1b2e1925f8ed75f3cdf6de597c4de5584201c0577aa892dfc121882d37e4c7`.


### Repeated program checks on the current build

The normal Terran `b2087fac` and test `ebddf82d` ROMs repeat the same
4,190-update Norad II suffix with 27 matching observations. They
export identical victory and frozen-result journals. Factoring a
source-level duplicate inside the scenario adapter changes the
victory update from 40,885,763 to 40,886,141 cycles. The pause and
frozen-result loads move by 25 and 268 cycles. The linked state
validator already calls the immutable program validator once per
invocation in both builds. The four profiled checks come from
separate mission-state validations inside the journal operation.
The trial stays out of the normal builds. The comparison receipt
SHA-256 is
`ddaa162492afc6b0f16f6af9ecd3378a0da6d9b79b4921f606c1668646897887`.
