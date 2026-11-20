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

## November 20 T2 fresh SRAM and result autosave

Backwater Station uses the normal Terran ROM built from `b2087fac`.
The campaign replay source is based on `856668b3`, which has the same
compiled inputs. The observer begins at HOME, reveals all eight
briefing pages and plays 29,380 gameplay calls from the original T2
tape. The trace counts seven earlier HOME inputs separately.

At gameplay call 18,213, frame 18,204, worker slot 67 is selected.
Start pauses and B saves bank 0. A fresh emulator presses L at HOME
and restores the observed world, campaign and Backwater trigger
state, selection and cursor. The camera derived from that cursor
matches at this cut.

All 29 tape checkpoints plus launch and victory match. Twelve
observations follow the fresh load. The observer acknowledges 12
authored gameplay pages with the simulation frame held. At frame
29,366, Raynor survives in slot 56 and the Infested Command Center
is gone. The Backwater outcome and primary objective record a win.
Terran retains 14 Marines, three Firebats, three SCVs, 1,164 minerals
and 200 gas. The game writes bank 1 and preserves bank 0. A third
emulator loads the frozen result with all 18 selected slots intact
and exports byte-identical SRAM. The shared observer also replays T1
on this cartridge and reproduces both of its saved journals.

The timed portion covers 29,446 controller calls and averages
666,900 guest cycles per main loop, or 25.16 emulated updates per
second. The largest measured update is 27,537,912 cycles. The counts
include UI, rendering, save and load. Physical GBA performance
remains unmeasured.

Use the pinned local inputs to repeat the check.

```sh
python3 tools/sc64_t9_normal_replay.py --chapter 2 --save-tick 18213 \
  --trace "$T2_TRACE" --rom "$TERRAN_ROM" --elf "$TERRAN_ELF" \
  --runner "$PHYSICAL_OBSERVER" --out "$NEW_RECEIPT_DIR"
```

| Input or receipt | SHA-256 |
| --- | --- |
| Normal Terran ROM | `80cf073bd02d580b127ccfd710feaf65bf0e2c285a27c2d6411d452fb3200576` |
| Matching ELF | `4ea80a38436e943fc9ca6f41812be27152a5abcb8e431ec07d7cabacd2fb87b1` |
| Physical observer | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| HOST tape | `48074d9c15899db45034e9e741d82fdf095fe08d6df5504d963250bb79c1b022` |
| Replay harness | `de24ef805e862593f27ed2ae0045b49577ebcc7223b0ccc631baed7c5ff6f35c` |
| Shared unit checkpoint observer | `d2838373c2bc9e822bf712ec053c0dfcdad02a8ca6a099f7f5e4d020e3f42648` |
| Pause SRAM | `c51c27561d5b60f97b289603991c962de5f4fb6f3f529b23b5b798936ed72ca8` |
| Victory SRAM and fresh readback | `0b90174a29f4502a026a8e99dfe8aca3c1a4b027aa87fb3e36ee956614eb7094` |
| T2 replay and both journal checks | `721535989a87ffe08f0c6557d0fd006b6478d2d259bc820c7756702ad7a030f9` |
| T1 regression | `67283fa918311ee86feff81bf3a5e5d20f4697ae668b7e0a46b839e3a3729f1e` |

## November 20 T1 fresh SRAM and result autosave

The replay uses the normal Terran death-depth cartridge from
`cd833c8c` and the observer based on `fa9b0ce3`. The intervening
runtime changes are in the custom-game AI planner.

The November 13 tape diverges on the newer cartridge by 50 minerals
at its 7,000-call checkpoint, even before a fresh process is involved.
It presses Down twice from the old default command after loading.
The normal save now remembers Attack. Two Down presses from Attack
reach Supply Depot. The
[November 18 repeat-command tape](2026-11-18-saved-repeat-command.md)
presses Down once and reaches Barracks. Its earlier ARM receipt verifies
the in-process load.

The fresh replay reveals all nine briefing pages from HOME and sends
the November 18 inputs through the tape's original pause boundary.
Raynor starts neutral in slot 21 and is human-owned by frame 990.
At gameplay call 6,222, frame 6,214, Start pauses and B saves. The
observer replaces the tape's restart and in-process load with HOME L
in a fresh emulator. The observed world, mission state, cursor,
remembered Attack command and selected unit slot 9 match the saved
state. The camera reconstructed from the cursor also matches.

All ten tape checkpoints plus launch and victory match. Four tape
checkpoints and the result follow the fresh load. The observer
acknowledges all eight gameplay dialogue pages with the simulation
frame held. At frame 10,120, Raynor remains alive in slot 21 under
player control. Terran has ten Marines, five SCVs and 417 minerals.
Bank 1 records the win and leaves the first bank byte-identical.
A third process loads the frozen result and exports identical SRAM.
A shared-harness T10 check also restores the frozen win at frame 3,361.

The timed portion covers 10,195 controller calls and averages 572,897
guest cycles per main loop, or 29.28 emulated updates per second.
The largest update is 28,678,403 cycles. These counts include UI,
rendering, save and load. Physical GBA performance remains unmeasured.

Use the pinned local inputs to repeat the check.

```sh
python3 tools/sc64_t9_normal_replay.py --chapter 1 --save-tick 6222 \
  --trace "$T1_TRACE" --rom "$TERRAN_ROM" --elf "$TERRAN_ELF" \
  --runner "$PHYSICAL_OBSERVER" --out "$NEW_RECEIPT_DIR"
```

| Input or receipt | SHA-256 |
| --- | --- |
| Normal Terran ROM | `8abd3e731fd6db1c1c0ff449ea1426d4214213f0990c5613a3b7158b3eb611c4` |
| Matching ELF | `357b901bdb96d325fe4388650ad2ea5d97b7ce801a7a8de14f278a913ce2e2db` |
| Physical observer | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| November 18 HOST tape | `27b78c4bcd6ee7076126a224c4a6fe362190fb7105631b446e19bde5689fc9ba` |
| Replay harness | `d68772eb69b23473b726446dbdc9570ea98afb3229d1d7425d29f346901ad5ed` |
| Checkpoint observer | `4b2a37d649192e2e7d72237eaa3015bb7547ccb09380e45fe506be9d20228152` |
| Tape validator | `9bba24c4a2167902609475b0cc54dbae03c879dc43c974992cf31936eb6d3ea9` |
| Pause SRAM | `66d8f528bcc1d5b17a07c63c7ac832220db023c879ec306273c0ea10db30632a` |
| Victory SRAM and fresh readback | `111f48fc4c566ade3dbc4d3bb5b2829827c55266e77027966cb64c08e8e5c381` |
| Replay and both journal checks | `91d5b371472c7148222a96499ae843833eb3cfe3299a51dbb57a45b7ba3ee276` |
| T10 regression | `fc3c52647470848023ddf2aa724737433fc2a5b8ac8ca184b56774ed13ae633d` |


### Current packed-reader release

The exact normal Terran ROM at release `b2087fac` reproduces all 12
observations, the nine briefing pages and all eight gameplay dialogue
pages. The pause, victory and frozen-result re-export are byte-identical
to the death-depth run. The 10,195-call replay averages 572,156 guest
cycles, or 29.32 emulated updates per second. Its peak is 25,053,520
cycles. This measures emulator work including presentation waits and
UI.

The ROM SHA-256 is
`80cf073bd02d580b127ccfd710feaf65bf0e2c285a27c2d6411d452fb3200576`.
The corresponding ELF SHA-256 is
`4ea80a38436e943fc9ca6f41812be27152a5abcb8e431ec07d7cabacd2fb87b1`.
The replay receipt SHA-256 is
`4551342f881a6bb80ed4ecb9f3ce96aac2dcf747761e5e543450e02596ec45c0`.
