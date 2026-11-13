# Terran T1 and T2 from HOME on the KODAK cartridge

Both retained HOST controllers won from HOME on the exact normal
Terran T10 KODAK cartridge. The physical-input runner used the
retained ROM and matching ELF from runtime `cbba1c5`. No cartridge
was rebuilt for this check.

| Mission | Tape updates | Physical updates | Briefing pages | Gameplay text pages | Matching observations, including result | Winning game frame |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| T1 | 10,151 | 10,201 | 9 | 8 | 11 | 10,120 |
| T2 | 29,387 | 29,441 | 8 | 12 | 30 | 29,366 |

Each replay started in a separate emulator process on HOME. It retained
the tape's campaign and mission selection. At the briefing, the runner
used ordinary A presses to reveal every page, then used the tape's
Start to enter play. In gameplay it read the authored text queue,
pressed A (or Start if A was already held), and released the
acknowledgment back to the tape's held keys. The game frame stayed fixed
through every press and release. The ordinary tape advanced only
between these acknowledgments. The observer read cartridge state and
did not write guest memory or install a game state.

T1's authored rows, in order, were 2 (two pages), 6, 0, 1, 4 (two
pages), and 5. It ended with Raynor alive, 10 Marines, five SCVs,
and 417 minerals. T2's rows were 0, 1, 2, 4, 6, 8, 9, 10 (two
pages), 11, 12, and 13. Its final group of pages appeared at game
frame 29,351, before the result screen. It ended with Raynor alive,
14 Marines, three Firebats, three SCVs, 1,164 minerals and 200 gas;
the Infested Command Center was gone.

Every retained scalar checkpoint and final scalar expectation matched.
T1's additional HOST hash covers the host's raw actor and building
arrays, whose layout differs from the ARM build, so it was not used
as an ARM comparison. The observed pages are those reached by these
tapes. This check does not cover every authored branch, save and
resume, or physical GBA hardware.

To repeat the check, use the physical observer through the existing
`PhysicalGame` helper in `tools/sc64_skirmish_sram_continuation.py`, with
a new output directory for each mission. Apply each tape's `held`
transitions at the zero-based event ticks and advance one physical
update per tape tick. Before T1 tick 4 or T2 tick 6, check the
briefing is selected and add press/release pairs of A to reveal or
advance its pages; keep the final page open for the tape's Start.
After each physical gameplay update, read four little-endian
halfwords at `0x06013fe0`: queue count, page, acknowledgment key and
front row. With a nonempty queue, insert a physical update with the
acknowledgment key added, verify the key latched and the frame held,
then restore the tape's held keys for a second update and verify
the frame still held. Repeat until the queue is empty. Compare the
tape's scalar fields at each one-based checkpoint and result using
ELF field layouts for the unit and building banks. The difference
between physical and tape updates is exactly the extra briefing
and acknowledgment calls above.

| Unchanged input | SHA-256 |
| --- | --- |
| Normal Terran ROM | `0f5e893037f71860c66112920e7195f5d7e2db5b4aa7f9e64e222689419de3d8` |
| Matching ELF | `00951a012354bf97ccd3d81682f8e4e7d2321381195c589b0d2d705b1abb8ab8` |
| Physical observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| T1 HOST tape | `5b6fc3b6af2ad1206f587f35de4587d26ed567a1e98e93d487b4ec758a29ee38` |
| T2 HOST tape | `48074d9c15899db45034e9e741d82fdf095fe08d6df5504d963250bb79c1b022` |
