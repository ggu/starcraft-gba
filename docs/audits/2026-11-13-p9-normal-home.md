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
