# Terran Outpost skirmish win on two normal ROMs

A Terran player beat one Zerg CPU on Outpost from fresh HOME on both the
November 13 KODAK cartridge (`cbba1c5`) and the later staged cartridge
(`c906058`). The port-free mGBA observer applied the same 1,853 held-key
transitions to each ordinary cartridge. No guest writes, save inputs or
debug scheduler were used.

The host policy keeps completed builders working, adds a second Barracks,
Depots, a Refinery, an Academy and a Bunker as resources allow, and queues
Marines, Firebats and Medics. It attacks nearby visible threats before
sending a larger group out, then clears remaining buildings when the enemy
unit pool is empty. All game commands use the ordinary controls. As a test
policy it can inspect internal game state, including the final enemy count;
this is not a proof of a visibility-only strategy.

Both ARM runs matched launch, 93 action checkpoints (frame and, where
recorded, post-action bank), and the terminal result. They won on main-loop
call 42,257 at simulation frame 41,951. Terran had 22 units, eight buildings
and 362 minerals / 1,846 gas; Zerg had zero units and buildings. The host
witness counted 43 Marine, 20 Firebat and 14 Medic queue transactions.

Both replays averaged 614,664 cycles per loop, or 27.29 emulated main-loop
updates/s including UI and presentation waits. The peak was about 14.04M
cycles. The result establishes one complete ordinary custom win, not
hardware FPS, full roster or three-CPU coverage, or N64 elimination parity.

## Reproduce

With the prepared Terran game inputs and the physical mGBA observer used
for the [campaign replays](2026-11-13-normal-campaign-matrix.md):

```sh
make build/test-sc64-skirmish-match-terran
SC64_SKIRMISH_COMBINED_ARMS=1 ./build/test-sc64-skirmish-match-terran \
  0 0 1 /tmp/sc64-outpost-trace.json 65000
python3 tools/sc64_skirmish_normal_replay.py \
  --trace /tmp/sc64-outpost-trace.json --rom "$NORMAL_TERRAN_ROM" \
  --elf "$MATCHING_TERRAN_ELF" --runner "$SC64_HEADLESS_REPLAY" \
  --rom-sha256 "$NORMAL_TERRAN_SHA256" --output /tmp/sc64-outpost-check
```

Choose a new output directory per run. The host option is limited to this
map, race and opponent and leaves the default matrix policy unchanged.

| Input | SHA-256 |
| --- | --- |
| Held-key trace | `ddc541b30061fd0f2c3292bf96f56d7c1fb5a87aa7427a60637deddc2a74c43a` |
| Physical mGBA observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| KODAK Terran ROM | `0f5e893037f71860c66112920e7195f5d7e2db5b4aa7f9e64e222689419de3d8` |
| Matching KODAK ELF | `00951a012354bf97ccd3d81682f8e4e7d2321381195c589b0d2d705b1abb8ab8` |
| Staged Terran ROM | `6f869a5e295aa95d9ecfecebc0f18eba4be4b8ce5b5f5a7ee67a6ba664304a0c` |
| Matching staged ELF | `1baa1e885a45e951d675cfb55e7be8cbfa5569eb6b31de8684e4535819256d3d` |
