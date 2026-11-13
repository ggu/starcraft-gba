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
