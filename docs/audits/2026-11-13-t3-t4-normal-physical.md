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
