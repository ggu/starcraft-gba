# P7 from HOME on normal Protoss P10

Homeland (P7) won on both the staged `c906058` and November 13 KODAK
`cbba1c5` normal Protoss P10 ROMs.
The replay selected mission 26 from fresh HOME, read all 11 briefing
pages, pressed START, and resumed the retained carried-P7 host tape at
zero-based update index 11, its first neutral gameplay update. The
replay acknowledged ten in-game dialogue pages through physical
press/release pairs while verifying that each pair left the game
frame unchanged. It neither injected SRAM nor wrote guest state.

The carried P8-flags host tape ended in a strict win at frame 55,892.
Both P10 runs reached the same frame on physical call 57,678
after replaying through tape index 57,615. The two trailing host calls
after the result were omitted. Every replayed key transition matched
the tape's post-update frame. The 59 selected snapshots were identical
across ROMs and tracked mission
identity, outcome and trigger fields, authored heroes, and original
Nexuses; those readbacks do not establish whole-world equality.

The game reported WIN with no defeat or runtime fault. All three
authored heroes (raw IDs 75, 78, 79) survived. Both hostile original
Nexuses (slots 34 and 74, source owner 0) were destroyed; the
source-owner-4 Nexus in slot 109 and neutral Nexus in slot 6 survived.
The victory switch was set, the victory cursor reset, and action timers
were clear. This verifies one ordinary route on existing binaries; it
does not qualify a save continuation or physical hardware performance.

| Input or receipt | SHA-256 |
| --- | --- |
| Carried P7 held-key tape | `d55d3a80681bb0186ec2971718c949937f25ceea6435ff33c14449b1fc06b775` |
| Port-free observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| Full replay harness | `8f60e3cee59a40428ca965d21307fc0308bcab2204e0cbc70b6545fa0a2db701` |
| P7 DWARF decoder and snapshot reader | `0618b79528455a7769e41c86d2d5245ed74686a0df0b7dfc5f2538d034e5c35b` |
| Staged P10 ROM | `b24184ee2b10b8c6f64153672a25d79a39ef6dc098b8c4cc7582ec8016538474` |
| Matching staged ELF | `863a74f69e2ada72129875a6e5558c526b98a60dd5e59685e4f5d97cab992e78` |
| Staged physical receipt | `57c023d073c674785bd00da2267415acd56b0071308b74a61df839619b9c416f` |
| KODAK P10 ROM | `7a8aa4da8bf156993db753b18a2fc2e02ad6bd880a58f014b6593b983a781310` |
| Matching KODAK ELF | `667bf7f4c435916c58c7715892485c9533fce4f9830011387bfbd8b6f8a3f73d` |
| KODAK physical receipt | `8ff96db01eaf51181cbc8116cef0985564cbddfaadfe51e10be48fae18465ed3` |

The bit-verified nearest-depot promotion candidate also completed the
same route. Its receipt matched every recorded value in the staged
receipt apart from the provenance hashes, including the 59 named
snapshots, all dialogue observations, and the absence of key-transition
frame drift.

| Promotion input or receipt | SHA-256 |
| --- | --- |
| P10 candidate ROM | `64a2bb9e43840c737b30934831f689fa948c43e640198f0473dadce7c9bfd525` |
| Matching candidate ELF | `1a810dd3768a10e0bbd75e7cf8a1ca48d6c1101eb71c6f5e8f1b2fa4ccf4676c` |
| Candidate replay harness | `24c8feb7839157293727bae31939d57d6364eaa8801717c047f4b322f5a04e0c` |
| Candidate physical receipt | `8cd6076b129b8f921c07ba7f95ae03358e68856406dfc643fb599cc31ed0a273` |

## November 20 normal cartridge and fresh SRAM

The normal Protoss cartridge from release `ea041527` reproduces the
full HOME win. The same held-key tape, 11 briefing pages and 10 field
pages reach victory at frame 55,892 after 57,678 ordinary controller
calls. All 59 named checkpoints and every input transition's game
frame match the retained run. The mean over the full run is 1,891,409
emulated cycles per main-loop update, including presentation and menus.
That is 8.87 emulated updates per second for this run. Hardware fps
is outside this observation.

A separate run saves after zero-based tape index 9,089, with the field
active at frame 8,845 and buttons released. START opens Pause and B
writes the original 32 KiB SRAM journal. Its active payload is 16,308
bytes, with a valid checksum and mission 26, frame 8,845. A different
emulator process starts at HOME and uses a single B press to load.
This leaves the tape's next L edge available without advancing the
game frame.

The fresh load restores the named mission snapshot, sampled live
actor fields and active production queues, owner banks, mineral
fields, P7 persistent bytes, cursor, camera, and remembered command.
The semantic sample includes 179 live units and 112 live buildings.
The normal loader clears the selected Gateway in slot 4. Its four queued
units and current production progress restore unchanged. The
[N64 T1 capture](2026-11-13-n64-t1-save-selection.md) directly shows a
similar building-selection reset for the Command Center. Native
Homeland load behavior has its own evidence requirement.

From that loaded state the unchanged suffix wins at frame 55,892.
All 49 post-load checkpoints and all remaining dialogue and key-frame
observations match the uninterrupted control. The route uses zero
extra gameplay inputs after loading and keeps the original SRAM
byte-for-byte. All three heroes survive and both hostile original
Nexuses are destroyed. This verifies one busy P7 save cut. Other
save frames and full-state equality throughout the suffix need
separate evidence.

| Current input or receipt | SHA-256 |
| --- | --- |
| Normal P10 ROM | `7e03188adcc9f1b7c1759a684e2c708d2b98d650109d003f9b7ee9d4e4829ff2` |
| Matching ELF | `39c328261fa9bed9584936d0920b9abd1d19192dc989ba915a6b4ad213c9b471` |
| Physical observer | `ff2c5a8b1f6764ed399d8a1b59df6ffde135ae2cd01a6d67500684b8429d23b7` |
| Normal uninterrupted receipt | `96bbc1811d9b37cec88a1dca8995eee0924adb567583bbb63467227468f39a70` |
| Save-boundary field observations | `c77fb23e533ded8212e28b46f41c2bea5c3f32156f459ce71a3b32d6e3b6f07c` |
| Original 32 KiB pause save | `1a7f52aebdbce3d46947e2318d8280238078c010afd13fa5be1d784a02f0c691` |
| Fresh-load and continuation receipt | `7008184314786a4b540ccb1dc8e2c277c807408652776b59da8726e93fe36e23` |

The separate `SC64_SAVE_UI_CONTEXT` preview uses the same 40 prepared
inputs and the same RAM limits. Its ordinary save and fresh load at
frame 8,845 preserve the selected Gateway and match the same sampled
pre-save world and controls. This preview check stops at the load.
The normal continuation already wins with its selection reset, so the
full-context option stays off in normal builds.

| UI preview input or receipt | SHA-256 |
| --- | --- |
| Preview P10 ROM | `e79fb055746c0377508acd3c9b44dffef4b97bc693252bd0d242897dbc06e478` |
| Preview save and fresh-load receipt | `24ac9483ffc8d3efd45d040794ccc1cca117498914682e0b8cafb77f2261808d` |

## November 22 facility release

The older host controller's first field command lands during an
authored transmission on the facility P10 cartridge. The host-only
controller now sends the ordinary advance key and release for each
queued field page before issuing that command. The prepared host
target is `make test-sc64-p10-p7-normal-home`. Its 4,929 input
transitions exactly match the qualified tape's held keys, input calls
and game frames. The controller wins at frame 55,892.

The exact facility cartridge saves through START/B at frame 8,845.
The pause journal matches the November 20 journal byte for byte.
A fresh process boots to HOME and loads with L, restoring the
recorded named mission snapshot, heroes and Nexus census. The
unchanged suffix matches the recorded frame at every input
transition and wins at frame 55,892. All three heroes survive,
the hostile original Nexuses are gone, and the mission records
zero trigger faults. This run measures the named save snapshot and
the input schedule.

The paired uninterrupted and fresh-load runs both win at frame
55,892. The ten retained checkpoints match units, buildings, economy,
P7 persistence, fog and queued impact counts. Raw route diagnostics
differ after load, and one checkpoint has different transient trace
slot bytes. Both indexed video pages and the palette match at each
cut, with the physical page addresses swapped at six cuts. The
observer did not record which page was displayed.

A third process loads the victory from HOME and exports the same
journal bytes. The sampled victory state, heroes, units, buildings,
economy, P7 persistence and fog restore. The terminal snapshot has
two pending impacts. They are cleared in the loaded result screen.

| Facility input or receipt | SHA-256 |
| --- | --- |
| Normal P10 ROM | `ae19f284f5447c269026cb5b651913028f2cc0379c8f0c36a0605611fc82edb0` |
| Matching ELF | `2908a8568690b6c5c4e3e93a8334eca3663d92a099dad6e596d5f2a01a248c43` |
| Prepared host target tape | `973cbb1b57b67337c80286f0f66e9550a1efe4b03f3dc62ae6bfa525cc50e93d` |
| Facility ARM input tape | `34a82f4cbb54e0ceeb9a066804fd26c7c58bedfa15911255ed213e46b10c2fdb` |
| Fresh HOME through victory receipt | `0f05a0636263ba98922c81fe5840aeccc29ab2d5876825baf8506db5a8e79e7d` |
| Uninterrupted paired receipt | `a20c9fde9d97a157473ff6bd0565243fc1c9104400c64aa2c3c998ee42cb15df` |
| Fresh HOME paired receipt and terminal readback | `8691c4b26147d70d0240a85c737833632c0968069e2debed6e5f1920eb44c8d7` |
| Fresh run victory journal | `dee1bd8a66002d1e0dfde9fd8beabc7c7e99c2ea4cb35f1f3642007b4a05eb09` |

## November 23 Melee-enabled cartridge

P7 repeats the archived ordinary controller route on packaged
Protoss 10 at `7f7eab41`. The normal HOME path saves at frame
8,845 on call 9,096. A fresh process restores the named mission
state. The two replay segments match the recorded frame at all
4,928 input transitions. Victory follows at frame 55,892 on
call 57,636, with all three heroes alive and zero trigger faults.

A third process restores the result from HOME. Eight neutral
updates leave the re-exported 32 KiB SRAM byte-identical to
the victory journal. Across all 12 pre-save, post-load and result
cuts, the fields recorded by the facility release agree with
the current cartridge. Both physical video pages and the palette
also match at every cut. The two transient impacts pending at
victory are cleared when the result is restored, as in the
facility replay.

| Local receipt | SHA-256 |
| --- | --- |
| Pause and result replay | `d221274865613359e3e1382fa9aaf5c13e51a1c30e971c4a3ea79d725e710c4e` |
| Twelve matched snapshots | `20a1bf7b197e4bdbab1df12174c96dc65200f4723bdcd9735f1c205ab998b0d0` |
| Victory and readback SRAM | `dee1bd8a66002d1e0dfde9fd8beabc7c7e99c2ea4cb35f1f3642007b4a05eb09` |
