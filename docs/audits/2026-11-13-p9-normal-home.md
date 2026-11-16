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
