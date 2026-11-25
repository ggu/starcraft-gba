# Z1 from HOME on the normal Z10 cartridge

The November 13 KODAK Z10 cartridge wins Z1 from a fresh HOME screen at
frame 50,669. The Chrysalis survives; the player still has 30 Hydralisks,
an Extractor, a Spawning Pool and a Hydralisk Den. The mission sets both
objective bits and the campaign completion bit, with zero adapter faults.

The retained carried Z1 HOST tape begins with ordinary campaign menu
inputs and skips the briefing with Start. The physical replay selects
the same entry and reveals all six pages with A. It then plays the
original field inputs, inserting two physical updates per authored page
to arm and release its acknowledgment. The observer checks the exact
page transition, frozen simulation frame and restored gameplay buttons.
Ten pages appear across four simulation boundaries. It does not write
guest memory or load a save.

All 50 periodic checkpoints and the final observation match the HOST
tape. Actor counts use runtime type IDs from the matching ELF, as the HOST
controller does. Imported source IDs are not used for counting new
buildings, which may lack an original map actor ID. The 50,816 tape calls
become 50,856 physical calls with the complete briefing and messages.

```sh
make build/test-sc64-z10-carried-z1
./build/test-sc64-z10-carried-z1 --trace /tmp/z1.json
python3 tools/sc64_z1_normal_replay.py --trace /tmp/z1.json \
  --rom build/StarCraft-Zerg-Z10-Transport1-GBA.gba \
  --elf build/starcraft-zerg-z10.elf --runner "$SC64_HEADLESS_REPLAY" \
  --out /tmp/z1-normal
```

The full emulator replay averages 706,693 guest cycles per update, or
23.74 emulated updates/s, with a 19,084,480-cycle peak. It measures one
winning route and does not cover fresh save reload or GBA hardware.

- ROM SHA-256: `e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907`.
- ELF SHA-256: `74c4b2394c4aee619bf2ea0549c31de7987827a915c32c5c50e7a4fec58ad6c1`.
- Retained HOST tape SHA-256: `68e1d10ab6b12a9a7baedf5b65c39deca7e9cc7952354e761584b4512aeb3efc`.
- Full physical receipt SHA-256: `e56afd192433ab6fc1af7f2f24be5a8c06781524633cf7c4a09e86b4f2c170f3`.

## November 26 standard cartridge

The standard Z10 ROM after #913 (SHA-256
`2d1d00751fbc9e82a012823ec54fedda54dd3a55b0cf5d1a001be2de268fe7fe`)
cleared Z1 from blank SRAM. All 50 periodic checkpoints and the final
observation matched the newly generated HOST trace. The Chrysalis survived,
30 Hydralisks remained and the victory occurred at frame 50,669. The 50,856
main-loop updates averaged 27.04 Hz including UI and presentation waits.

The command above now exports the victory, reloads it from HOME in a new
emulator process, compares the live world and objectives, re-exports both
SRAM banks unchanged, and advances to Z2 at frame zero. The result SHA-256
was `8941d1f93e685690f88f84d1ad23c878206e97a4e373abcc4b42178a201c6c8d`.

From this exact victory a separately retained physical-button route cleared
Z2 at frame 10,209. The Chrysalis carrier was slot 18, the objective flags
were 17 and both mission completion bits were set. The fresh Z2 victory
re-export was `393a8325d5cc34625aec5a3e8de6e5d5dd9630195421aa95f4589b6ae30fc9e4`.
The repository command above reproduces the Z1 win and Z2 handoff. The Z2
victory used the locally retained recording.

A second Z1 route started from blank SRAM on the same ROM, paused at frame
984, and continued through 17 physical-button segments to victory at frame
49,270. That result also reloaded and re-exported byte for byte (SHA-256
`5245f01d0add6695c44ad0ece21156e0ee7f7c1668376a30cbdde6d0611c1091`).
