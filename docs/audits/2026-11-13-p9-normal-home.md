# P9 from HOME on the normal P10 cartridge

The November 13 KODAK Protoss cartridge runs **Shadow Hunters** from a
fresh HOME screen using physical button masks and read-only observation.
It wins at simulation frame 144,361 after 149,924 ordinary calls.
All 59 host checkpoints match, including the pause-save observation,
both Cerebrate objectives, and the surviving Zeratul and Fenix.
The terminal campaign records phase 3, objective flags 1, both switches,
P9's completion bit, and zero unsupported-action faults.

The older P10-vector tape won at frame 150,969 in a replacement-loop
replay. Its standalone P9 counterpart was retained, but neither older
proof revealed the normal cartridge's briefing. The current P9
controller uses its existing route, a private BFS that reads the live
walkability map, and checks route-state purity between updates. It
reveals all 11 authored briefing pages. The winning route does not
trigger an in-mission text box; the source mission's two DisplayText
actions are on defeat branches.

With prepared inputs, build `make build/test-sc64-p10-p9-normal-home`.
Run the controller with a new trace path and redirect its standard
error to a separate page witness file. Then use
`tools/sc64_p9_normal_replay.py --trace ... --pages ... --rom ... --elf ...
--runner ... --out ...`. The host proof uses ASan/UBSan. The cartridge
and matching ELF are unchanged.

The full normal run takes 304,310,098,509 guest cycles, or 2,029,762
per update (8.27 emulated updates/s). The peak is 53,643,591 cycles.
These are emulator main-loop measurements, including UI and waits;
this run does not qualify physical hardware or fresh-process SRAM.

- ROM SHA-256: `7a8aa4da8bf156993db753b18a2fc2e02ad6bd880a58f014b6593b983a781310`.
- ELF SHA-256: `667bf7f4c435916c58c7715892485c9533fce4f9830011387bfbd8b6f8a3f73d`.
- Host input SHA-256: `6f6ae7ac3d36d93b0a4cde2379b47854fa2e54674f4111e77bc4933519d4a74f`.
- Full replay receipt SHA-256: `ea44beb58d5e39a32b89e7cd745488503603244a8afb52130e43ec2cf5d69bf9`.


## November 20 fresh SRAM victory

The sanitizer controller built with the current normal P10 flags records
98,177 ordinary calls from HOME through victory. The corresponding normal
ROM from release `d464e1f6` saves at field frame 30,109 with a Gateway,
Forge and Pylon still warping in. The 32 KiB cartridge journal is
byte-identical to the HOST journal. It contains a valid sequence-one
record with a 16,308-byte payload, and its other bank remains erased.

Pass `--fresh-save` to the observer above. It follows the tape through
the pause-menu B save, exports SRAM and launches a new process. HOME L
replaces the original unpause START at tape index 30,974. The next
neutral update advances the world to frame 30,110, as on HOST. The
replay checks the named world, controls and complete P9 persistent
runtime block at load. A separate fresh load checks the three construction
orders and their saved progress.

All 42 gameplay checkpoints match, including 30 after fresh load. The
11 briefing pages also match. Both Cerebrates are destroyed and Zeratul
and Fenix remain alive and player-owned at the frame 95,737 victory.
The primary objective and completion bits are set with zero unsupported
mission actions. The journal bytes remain stable through the replay.

The run averages 9.35 controller calls per emulated second. The earlier
KODAK run used a different route. Measure the current movement and save
costs before comparing performance across these two runs.

| Input or receipt | SHA-256 |
| --- | --- |
| Normal P10 ROM | `34859980af3065e5401e13b892adf204bea77fc6e65a1dc084f7cabef184baf5` |
| Matching ELF | `2b92433c1dfc2201d1e8a383d8317f4bbeef50d285fe2b81e76364203414aeda` |
| Controller tape | `63fb7de5b0aa47170cf7e59cfaace18721dfa992aff0b283e2f0dcb481ffde2f` |
| Briefing witness | `d97c964a95150acc7f08ab2e5c51a8aa9a7b29607469e142e3e53a0207419ce2` |
| Pause SRAM | `7cfe4b14b562c8f7c3bfe04bdd03c898054d09e8f2251264a338b1bba79b80b6` |
| Active construction witness | `f67393f21f1ef1f7cb31d68c3168274b11144f081f889ffbbc89235e9fb9f00a` |
| Fresh-load replay receipt | `45004a7cedec296e86b7d7d01ac88dead8365922499d95d06ca1ef3957b1db81` |

## Scripted destruction and result autosave

The exact packed-Nexus P10 replay reached RESULT, but its SRAM export
still held the pause journal. A read-only terminal trace reached the
synchronous save at frame 95,737 and passed all 240 unit rows. The
typed building writer then rejected the Hive in slot 10 because its HP
fell below the signed journal range. The save returned before either
SRAM writer ran. Both authored building kill actions subtract a lethal
packet through the shared destruction path. P9 now supplies enough
damage to reduce the building to zero HP before its death row is saved.

The normal candidate on `defd807a` loads the original frame-30,109
pause journal and follows the same 67,202 physical controller updates
to frame 95,737. All 30 named gameplay checkpoints match. The dead
Hive's row retains its identity and zero HP. The 30 pre-result captures
match both indexed pages and the palette from the earlier replay. At
the final result one indexed page differs.

Victory writes a valid 16,308-byte sequence-2 payload and retains the
original pause bank byte for byte. A second fresh HOME load restores
the winning screen, live world, and dead Hive. The world stays frozen
for eight neutral updates and re-exports the identical 32 KiB journal.
The normal P10 link verifies its cartridge header, ELF payload, and
40 prepared inputs. The focused sanitizer preflight invokes the
authored Hive kill and encodes and decodes its dead row.

| Retained evidence | SHA-256 |
| --- | --- |
| Original failed save trace | `26ae41f602bcf49b51dccb733064f2526e44b95ef6c6f64e924793676963bc50` |
| Normal repair ROM | `0e68933eee0f03216c24d36478533bffb536e0b53d96f2be2dab6ae285fc1e3f` |
| Normal ARM link | `787f8017e89f492e58b30a924f7643fdef961cdee1925798f09763378dd39b84` |
| Physical replay and fresh result load | `c9a6137dbc5b2020d94b571494c1ad59187422746c2e69c44eb8dd8dedc54485` |
| Victory and fresh-load SRAM exports | `5efb8db7b9f4b5f75d1e91ad36550ecd626c7e8df0ea7fb22f6d51978e6b06a2` |

### Interceptor pulse-depth release

The normal P10 link on `64192d4e` verifies the 40 prepared inputs and
retains the release RAM endpoints. The same physical controller suffix
matches all 30 named gameplay checkpoints and all 31 palette and
indexed-page captures from the first repair. The frame-95,737 victory
writes the same sequence-2 journal bytes. Fresh HOME L restores the
result and re-exports those bytes after eight neutral updates.

| Retained evidence | SHA-256 |
| --- | --- |
| Normal P10 ROM | `81e018081beec7bf65f44dfc2cc4c4daae49b8aa1dfc515e38ea6e4bfad763ff` |
| Normal ARM link | `5a1843a00aee1e452fe83d5eee13f782fa5cbb194d288627b6f8be1a5ba6b60b` |
| Physical replay and fresh result load | `8df6b0df32bff8e4d075c531e8fe12de12e70080f4d08ab8ebac0bf778fd5b7d` |

The normal P10 recipe on `b127c1c0` with this repair regenerates the
exact cartridge used by the Interceptor-release replay. The 40 pinned
inputs, header, linked payload and RAM endpoints pass again. The
composition receipt SHA-256 is
`b856004427cb59a0be966066e82da065472e32a62242a55632c7f0875da690a4`.

## November 23 current cartridge victory and result reload

The November 20 tape matches through frame 19,844 on `c84883de`.
At frame 22,096, the regular cartridge has 1,924 minerals and the
old tape has 1,870. The other named fields at that checkpoint agree.
A fresh ASan/UBSan controller built with the current cartridge flags
takes a longer ordinary-input route.

The current tape wins at frame 272,857 after 281,447 controller calls.
All 93 gameplay checkpoints and 11 briefing pages match the regular
ARM cartridge, including 80 gameplay checkpoints after a fresh
HOME load. Both heroes survive and both Cerebrate switches are set.
The frame-32,071 pause SRAM matches the controller byte for byte
and restores with one structure under construction.

`--fresh-save --verify-result-save` checks the pause bank from an
empty SRAM start, resumes in a second process, and verifies victory
in the second bank with the pause bank intact. A third process
restores RESULT using HOME L, checks the named world, controls and
P9 runtime, advances eight neutral updates, and exports byte-identical
SRAM. The observer checks runner shutdown before publishing its
receipt. It monitors a 32 MiB output budget, enforces a 16 MiB per-file
limit, and retains a 256 MiB free-space reserve.

This route averages 11.14 controller calls per emulated second. Its
different gameplay and length preclude a speedup comparison with
the November 20 route. Crowded gameplay timing on GBA SP remains open.

| Item | SHA-256 |
| --- | --- |
| Regular P10 ROM | `e44f7e1fa8dd07dfb9ae8c00d2651821fd80d862d2b0f723e946e9a947de01d3` |
| Matching ELF | `118026540e3d178fae985fae925736c09156ae08b73ca4ea80a97701ba308775` |
| Current controller tape | `9013de170b21ac59250698d922e4d62ddd15485a3a0eedd011e0418bcfe4d355` |
| Current briefing witness | `34783cb55ec8aa46fa96c7bcf61f37f809911c6a5059f839ebf8b601e5e59922` |
| Pause SRAM | `519c27ed6b72767e891208b68e113159a7b6d52b80b36d9ef86baeefa9153277` |
| Victory and readback SRAM | `f77d58315d7cbfe62604d5fdb8f2df34fa6fc9705f6f09526164d13269a3dec9` |
| Full replay receipt | `f326c5705e93d21928d16e0a8ab779867f00f9f0b54d09d1793c98d9509e1c19` |

### Cost of the post-save location checks

Two 16-call windows on the same regular cartridge use the fresh
pause journal and the recorded ordinary buttons. The instruction
profiler matches the uninstrumented run at all 32 sampled cuts in
guest cycles, named world, indexed display pages, palette and route
counters. The call ranges are 33,134–33,149 and 34,934–34,949.

| Mean guest cycles per update | First window | Second window |
| --- | ---: | ---: |
| Main loop | 1,351,643 | 1,312,859 |
| Simulation update | 884,864 | 884,698 |
| Orders | 389,599 | 390,812 |
| P9 runtime tick | 276,382 | 275,466 |
| Shared actor-world count | 242,186 | 260,553 |

Actor reads within the count cost about 188,000 and 203,000 cycles.
The direct Command provider already filters the live pools without
constructing an actor view. A location-sensitive Bring count still
constructs a view for every live row. Measure a direct Bring path
that validates every live row and computes exact geometry only for
eligible actors. Preserve cargo, construction, invalid rows and
changes made by earlier trigger actions. Inclusive scopes overlap.
These short windows measure one part of the replay on an emulator.

The attribution receipt SHA-256 is
`5ebc36359fe31a3487dd644c835d561eb0f8bff116a1e7590759e5878a9ebcf2`.

### Direct location-count trial

A live-pool trial keeps validation and rescans the same slots on
every query. It computes a full actor view only for matching
location candidates. The compiler specializes the existing Command
path so its scan has no per-match rectangle branch.

| Mean guest cycles per update | First window | Second window |
| --- | ---: | ---: |
| Actor-world count, current | 242,186 | 260,553 |
| Actor-world count, candidate | 52,592 | 53,348 |
| Main loop, current | 1,351,643 | 1,312,859 |
| Main loop, candidate | 1,140,985 | 1,101,228 |

The mean falls 15.59% and 16.12% in these windows, to 14.70
and 15.24 emulated updates per second. World, both indexed pages,
palette and route counters match all 32 sampled cuts. A separate
9,489-update P10 victory tail matches 81 cuts and final SRAM. Its
mean rises 0.022%, compared with 0.233% in the first shared-scanner
trial. The P9 attribution and specialized P10 tail receipts are
`66bf6844678e551b79b85f61e4c768796573e4cce416747296a5d29334217921` and
`839bc809a8c7d141f2f3f5a3de32c963695fde837be60985ab49828a7f6219ea`.

### Full victory and journal with direct location counts

The specialized candidate replays all 281,447 ordinary tape calls
through victory at frame 272,857. The 93 named checkpoints, 11 briefing
pages, and 80 checkpoints after a fresh pause reload agree. The pause,
victory and independent HOME result readback images all match the
baseline byte for byte. The third emulator advances eight neutral
result updates before exporting its journal.

The mean falls from 1,505,610 to 1,379,948 guest cycles per call,
an 8.35% reduction. The same run rises from 11.14 to 12.16 emulated
calls per second. The busy campaign still exceeds the 30 fps budget.
The four standard links pass the existing RAM checks. Z10 and P10
use eight additional EWRAM bytes with unchanged IWRAM endpoints.
Terran and Showcase reproduce their control ROMs.

All 30 ordinary campaign entries launch, 24 Custom Game fresh
continuations pass, and six three-CPU starts match all 120 state
and pixel cuts. Those six matches stay within 0.001% in total
guest cycles. P10 and Z10 pass the sanitizer comparison of the
original actor provider and the direct counts, with and without
raw validation caching.

| Item | SHA-256 |
| --- | --- |
| Complete specialized P9 replay | `a33d94d2bd3a4457d5a72ac44e59a6c1ec1048098117acfbbd3282939449c36c` |
| Victory and fresh readback SRAM | `f77d58315d7cbfe62604d5fdb8f2df34fa6fc9705f6f09526164d13269a3dec9` |
| Six three-CPU matches | `b3826e59900d6a91af7b0e0858d5c69468f741e35cf8138a9ac2ff88cc55ae48` |
| Custom Game fresh continuation matrix | `d39e46b0081b6626e0e190feabb94c550487e5bc2563ff2bf8878d2c3f74056e` |

### Inline validation for cached actor types

The normal Protoss and Zerg products already use the specialized
location scan through PR #747. This trial inlines the common memo hit
for a validated raw type in both shared scanners. Each live row still
checks its owner and current coordinates. An unresolved or out-of-range
type goes through the existing validator. Bring still checks current
completion, cargo and location using the existing actor geometry.

| Mean cycles per sampled update | First window, control → trial | Second window, control → trial |
| --- | ---: | ---: |
| Command and Bring scanners | 52,393 → 34,844 | 53,149 → 35,314 |
| Whole loop | 1,089,720 → 1,071,698 | 1,090,299 → 1,072,435 |

The two 16-update windows start at input calls 33,134 and 34,934
after the retained ordinary P9 pause loads. Frames, world, both
indexed pages, palette and route counters match at all 32 cuts.
Whole-loop cost falls by 1.65% and 1.64%.
The 9,489-update P10 seeded victory tail saves 0.54%. All 81 recorded
cuts, the victory journal and fresh result re-export match the packaged
control. These are emulator measurements.

All four prepared links pass their original RAM guards with identical
EWRAM and IWRAM endpoints. Terran and Showcase ROMs are byte-identical
to their controls. The 30 campaign openings, four predecessor campaign
save loads, 20 Custom Game continuations and 20 cross-version cases
repeat the packaged receipts. The Protoss and Zerg sanitizer fixtures
also match the shared slow provider across the actor and geometry cases.
The full paired P9 victory and result-journal runs are in progress.

| Local receipt | SHA-256 |
| --- | --- |
| Packaged P9 attribution | `9242fc4c4699bd5a256747c66620760629828be9d0e294b601522f97f555dc2e` |
| Inline validation P9 attribution | `b37cae335a6a2632f50162509676b18617d5d05288abb58f1939153b32988192` |
| Inline validation P10 seeded win | `d14bdae7ef35ff7f4584b51a24feee2e6d8739f5c8b367d3762fd96f79d5f306` |
