# Into the Darkness through SRAM on the normal P10 cartridge

The normal Protoss 10 ROM from the current release `ec5c3f77` carries the
same runtime as `0f4d63c4`. The sanitizer HOST controller regenerates the
winning [P6 controller tape](2026-11-14-p5-p6-normal-home.md) and page log
byte for byte. The physical-input verifier starts at HOME in mGBA.

At tape call 65,753, frame 64,800, the verifier uses START and B to write
a 32 KiB SRAM image from the normal pause screen. Tassadar is alive and
Zeratul's rescue is still ahead. Sequence one has 16,308 payload bytes in
the first bank. The verifier checks the checksum, inner and outer versions,
mission and frame. The second bank remains erased.

HOME L in a fresh process restores the same frame, actor observations,
controls and complete P6 persistent state. The original SRAM hash is
stable through the remaining inputs. All 113 checkpoints match, including
40 after the reload. All six briefings and 17 authored dialogue pages
match the controller recording.

The run wins at frame 99,906 after 101,370 recorded calls. Zeratul is
player-owned with 60 HP at (1267, 1701), inside the extraction area.
Tassadar remains player-owned with 65 HP. The scenario and campaign record
victory, the P6 completion bit is set, and there are zero trigger faults
and unsupported actions. The authenticated N64 scenario starts its victory
sequence when the player owns Zeratul. The successful extraction route and
Tassadar's survival are additional checks in this GBA replay.

This route averages 13.12 recorded calls per emulated second, including the
briefing and other UI calls. Hardware timing and other save cuts remain
open.

## Earlier P10 save

The same ROM also loads the [November 6 frame-121 journal](2026-11-06-early-protoss-save/README.md)
through HOME L. Its observed state matches the original recovery both at
frame 121 and after 120 further updates at frame 241. The exported SRAM
matches the original image byte for byte.

## Reproduce the mid-mission check

Build `build/px-p6-normal-home` in a prepared tree. Run the controller with
`--trace` pointing to a new destination and capture its standard error for
the page log. Use the matching pinned inputs with
`tools/sc64_p56_normal_home_replay.py --mission 25 --fresh-save-call 65753`.
Use `--quiet-guest` with the noisy observer identified below.

| Input or receipt | SHA-256 |
| --- | --- |
| Normal P10 ROM | `2e5d672205b4f455aa8e266ee12d71f1326006120e7218fd187946a192571af9` |
| Matching ELF | `ea0327fd231bdcb601d5ada3aec329399224d2fb588c515bbff193ddca72e581` |
| Physical observer | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| Controller tape | `f405c609b278c07827ceb227e0fb7947f9f90aba6b8f40abd5dc5d0c47582bd4` |
| Authored page log | `c892ed584c8bb8b333d8a9f2d7ae7f242beed4c59af7bfae24734ac1808882ed` |
| Mid-mission SRAM | `c7a39caf8ccdd86d5eeac986b9e265db528d269d8081597ba634699f3175832d` |
| Fresh-process victory receipt | `34044cdf5b5f5705ee811112e4d1502bef1efb20daab706f9cf1e76da53545ea` |
| Earlier P10 SRAM | `e974f20a66daf714978ee64ef21af5fcbb1c82307c51fefb870ba0a87b64816a` |
| Earlier SRAM continuation receipt | `af5f8540ad9f290f53fd4ff7a3a01a0d590ad1a32351a0c459837f186e3d1747` |
