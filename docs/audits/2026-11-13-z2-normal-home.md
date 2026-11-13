# Z2 from HOME on the normal Z10 cartridge

The November 13 KODAK Zerg cartridge wins **Egression** from a fresh
HOME screen at frame 8,164 after 8,440 ordinary input updates. The
emulator receives only physical button masks; the observer reads guest
memory. The eight periodic host checkpoints and final observation match
exactly, including the rescued Chrysalis, the carrier slot, campaign
phase and completion bit. The final state is victory with objective
flags 17 and no loss.

The controller reveals all six authored briefing pages. It acknowledges
two pages at frame 455 (row 1, pages 0 and 1) and the final message at
frame 8,164 (row 0, page 0). The ARM replay verifies the row and page
before each press, the armed key, and the release, with the simulation
frame frozen through each acknowledgment.

The retained 8,412-call HOST tape has HOME transitions, but uses START
to skip the briefing and contains no authored modal acknowledgments.
Playing it unchanged on the normal cartridge leaves an open message
at call 1,000. The new controller keeps the route and game inputs,
reveals the briefing and acknowledges each message normally. A
route-state hash is checked between every controller update. The host
tape was produced under ASan/UBSan with the prepared Z10 release flags,
plus the controller's briefing and modal test options.

With prepared inputs, build `make build/test-sc64-z10-z2-normal-home`
and run it as `--controller 1 --trace /path/to/z2.json`, redirecting
stderr to a separate page witness file. Use
`tools/sc64_z2_normal_replay.py --trace ... --pages ... --rom ... --elf ...
--runner ... --out ...` with the matching physical mGBA runner.

The full normal cartridge run takes 6,476,264,394 guest cycles, or
767,330 per update (21.86 emulated updates/s). The peak is 17,342,606
cycles. This measures the whole main loop in emulation, including
presentation and menus. It does not qualify physical hardware or SRAM.

- ROM SHA-256: `e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907`.
- ELF SHA-256: `74c4b2394c4aee619bf2ea0549c31de7987827a915c32c5c50e7a4fec58ad6c1`.
- Host input SHA-256: `2d28a6963030a7ef909488f728f82c3e889069c1fa1d881b6fa4eaf27eba305c`.
- Full replay receipt SHA-256: `cd35122cfd084c71517a4c0c35133624bbab50ca62d6b2d0bf6a3feff9c5d311`.
