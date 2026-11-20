# Terran T3 and T4 from HOME on the KODAK cartridge

Desperate Alliance (T3) and The Jacobs Installation (T4) both won on the
normal Terran T10 KODAK ROM built from runtime `cbba1c5`. The emulator
booted the unchanged ROM at HOME and received ordinary controller input.
The observer read guest state through the matching ELF; it did not write
memory, load a save or run a replacement game loop.

| Mission | Tape calls | Physical updates | Briefing pages | Gameplay text pages | Matched observations | Winning frame |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| T3 | 54,007 | 54,077 | 15 | 6 | 54 periodic plus terminal | 54,000 |
| T4 | 8,952 | 9,004 | 8 | 11 | Launch, eight periodic and terminal | 8,911 |

T3 held the original base through the full countdown and all eleven waves.
It ended with 16 player units, ten player buildings, no mission fault and
campaign completion bit 6 set. The retained route uses no field orders
after deployment. T4 finished the installation route with Raynor alive,
source switch 4 set, switch 7 clear, no trigger faults and completion
bit 7 set. Its terminal actor was alive at world coordinates (1218, 1016).

Replay both tapes through `PhysicalGame` in
`tools/sc64_skirmish_sram_continuation.py`. Follow the recorded HOME
menu input. For T3, A opens the list, Right moves to the second page and
A opens T3; T4 adds Down before the mission selection. Use A to reveal
and advance each briefing page, then use the recorded Start to deploy.
In the field, handle the authored queue as in the [T1/T2 replay](2026-11-13-t1-t2-normal-physical.md).
Each acknowledgment press and release holds the simulation frame and
restores the tape's held keys. Extra updates equal the briefing calls
and two calls per dialogue page. Compare T3's recorded scalar fields and
T4's `compare_observation` fields at the original tape boundaries.

The old T4 actor observer read every field as a signed four-byte word.
Terran T10's compact unit bank instead uses one- and two-byte fields for
this check. `read_unit_slots` now uses the ELF width and signedness and
rejects truncated banks; it no longer reads neighboring fields as part
of a coordinate or actor ID. The runtime is unchanged.

T3 averaged 591,442 guest cycles per update (28.37 emulated updates/s);
T4 averaged 721,870 (23.24/s). These include menus, render and display
waits. They qualify these two routes, not every branch, SRAM continuation,
all scenarios, or frame rate on a physical GBA.

| Retained input or receipt | SHA-256 |
| --- | --- |
| Terran ROM | `0f5e893037f71860c66112920e7195f5d7e2db5b4aa7f9e64e222689419de3d8` |
| Matching ELF | `00951a012354bf97ccd3d81682f8e4e7d2321381195c589b0d2d705b1abb8ab8` |
| Physical observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| T3 HOST tape | `b212efeeaa935c800b1ac66004da915d37b17bbba31d84e8cac647cb2c626724` |
| T4 HOST tape | `f10192aab83c53c616888b0430e354d9dc49f163d7e3a94796ff7ca362df7b7a` |
| T3 replay receipt | `1ea3bc2d5d6bb63685d9f2f700c3748840d589494eb8ce02d92fa28a69e7c42c` |
| T4 replay receipt | `2a600105ff8759b07e1156551b2e4ee798e35725aaa6258766c0c391b873424f` |

## November 20 T4 fresh SRAM and result autosave

The Jacobs Installation replay uses the normal Terran death-depth
cartridge from `cd833c8c`. The replay tools are based on `6ea84a03`.
The intervening runtime changes are in the custom-game AI planner.

The observer starts at HOME, reveals all eight briefing pages and plays
8,943 gameplay calls with physical buttons. The retained HOST tape
counts nine entry calls separately. At gameplay call 4,991, frame
4,968, unit slot 70 is selected and the game is in field mode. Start
opens Pause and B writes bank 0. A fresh process loads the save with L
from HOME and restores the observed world, mission state, cursor and
selected unit. The camera derived from the cursor matches at this cut.

All ten observations match, including four after the reload. The
observer acknowledges 11 gameplay pages with the simulation frame held.
The result arrives at frame 8,911 with Raynor alive, slot 70 at
(1218, 1016), switch 4 set, switch 7 clear and no trigger faults.
The game writes bank 1 without changing the first bank. A third process
loads the frozen victory with unit slot 70 still selected and exports
identical SRAM. A separate T10 replay with the shared harness also wins
and restores its frozen victory at frame 3,361.

The 9,009 measured controller calls average 721,782 guest cycles per
main loop, or 23.24 emulated updates per second. The largest update is
40,900,276 cycles. The measurement includes UI, rendering, save and
load. Hardware frame rate remains unmeasured.

Use the pinned local inputs to repeat the check.

```sh
python3 tools/sc64_t9_normal_replay.py --chapter 4 --save-tick 4991 \
  --trace "$T4_TRACE" --rom "$TERRAN_ROM" --elf "$TERRAN_ELF" \
  --runner "$PHYSICAL_OBSERVER" --out "$NEW_RECEIPT_DIR"
```

| Input or receipt | SHA-256 |
| --- | --- |
| Normal Terran ROM | `8abd3e731fd6db1c1c0ff449ea1426d4214213f0990c5613a3b7158b3eb611c4` |
| Matching ELF | `357b901bdb96d325fe4388650ad2ea5d97b7ce801a7a8de14f278a913ce2e2db` |
| Physical observer | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| HOST tape | `f10192aab83c53c616888b0430e354d9dc49f163d7e3a94796ff7ca362df7b7a` |
| Replay harness | `1a00633499c6fb8cbc44e990f4ff3328c2638772cce5559845aa32c0302cdb82` |
| Checkpoint observer | `0c819ade9f9e31975e8c789f40dd3783a6276bfc7cc1d22b125733e5ca4a6de5` |
| Pause SRAM | `8852e2cad249e10a1081f59babea7a838134eef4ab23a2058fa80f29556ffdb9` |
| Victory SRAM and fresh readback | `a3d129b4805acc78b8b6ca72422585d0b47ef7d8a21885dc80728706d078c9b1` |
| Replay and both journal checks | `29bfc1ed61f6b0fe34697c8c00989d5614ec71ac0e81a8361c203b64e0fffdc2` |
| T10 regression | `7385746eb7904fcf48d76a655d3de34bc2583adb7b0c31c8167ac78626609200` |


### Current packed-reader release

The normal Terran ROM built from `b2087fac` repeats the same Jacobs
Installation route. All ten observations match through the fresh
load at frame 4,968 and victory at frame 8,911. The pause, victory
and fresh result readback files are each byte-identical to the
death-depth run. The current release `fb6968c2` uses the same
compiled inputs. The 9,009 controller calls average 719,693 cycles
or 23.31 emulated updates per second.

The ROM and ELF SHA-256 values are
`80cf073bd02d580b127ccfd710feaf65bf0e2c285a27c2d6411d452fb3200576`
and `4ea80a38436e943fc9ca6f41812be27152a5abcb8e431ec07d7cabacd2fb87b1`.
The comparison receipt SHA-256 is
`fb0fb317d670cc484a1c515a49978a8376dec0140db27493e1ebd6a7d1fe5837`.
