# Carried Z7 on the normal Z10 cartridge

The earlier winning HOST tape for The Culling does not reproduce on the
normal Z10 cartridge from `6e843d814da131950c92da79901985fc2e05ff78`.
From a fresh HOME boot, the physical replay matched the first 133 observations
through call 49,034, game frame 47,760. At call 49,397, game frame 48,123,
the cartridge had 57 player units and the old tape expected 58. The other
named fields, including economy, enemy actor counts, trigger state and action
timers, still matched. The runner used ordinary buttons and read-only guest
observations. No authored text box interrupted this prefix.

Two fresh ASan/UBSan HOST runs from `6127370`, with route purity checks and
the same controller arguments as the old tape, both reported 57 player units
at that checkpoint. Packed building rendering on and off produced identical
tapes. Both runs reached the controller's limit at frame 200,283 with 14
enemy buildings, no adapter fault, and a full 240-slot live unit bank: 50
player units and 190 enemy units. The player had 3,970 minerals, 4,839 gas,
and only two Mutalisks. The first mismatching observation therefore also
reproduces in HOST; the older tape is stale for this runtime.

The old policy activated its paid air plan at frame 127,146, too late for
the controller to build the desired air group on this route. The carried
Z10 test now asks the controller to start that plan after frame 85,000.
It made that decision at frame 86,706, bought eight Mutalisks by frame
112,141, and won at frame 194,998 after 197,455 calls and 16,075 held-key
transitions. The threshold only changes which buttons the controller
chooses: Lair, Spire, Mutalisks and orders still go through the game.
Three ASan/UBSan HOST runs, including packed and unpacked building flags,
produced the same winning tape. The carried Z9 and standalone Z7 defaults
are unchanged.

```sh
make CLANG='clang -D_FORTIFY_SOURCE=0 -DROUTE_PURITY_DEBUG' build/test-sc64-z10-carried-z7
./build/test-sc64-z10-carried-z7 --policy early-gas --policy-workers 6 \
  --policy-reserve 50 --policy-hydras 8 --policy-second-first 0 --policy-retreat 0 \
  --policy-home-radius 320 --home-army-r --start-delay 33 \
  --force-late-air-at-frame 85000 --limit 200000 --trace .local/sc64/z7-current.json
python3 tools/sc64_z7_normal_replay.py --physical --trace .local/sc64/z7-current.json \
  --rom build/StarCraft-Zerg-Z10-Transport1-GBA.gba --elf build/starcraft-zerg-z10.elf \
  --runner "$SC64_HEADLESS_REPLAY" --output .local/sc64/z7-current-physical
```

On this exact cartridge, a separate 16,226-call prefix averaged 1,180,361
guest cycles per main loop, or 14.21 emulated updates per second. Its last
3,993 calls averaged 1,330,628 cycles (12.61 updates per second). All 40
observations matched. These counters include the renderer and presentation
waits; they are not physical GBA frame rates. They do not meet the 30 fps
target.

- ROM SHA-256: `6224e1821edacff7a88908d2a9b5bc64b496094a567213153a1796c3d36715d3`.
- ELF SHA-256: `9bbe176b7d2916f945aa75691feccc2d1743e8b9e18b6e40807c99e05883c307`.
- Earlier winning HOST tape SHA-256: `5dbc798d0146349978c1e0ce7ced1fb8605c709a293713699c5e6957717452f7`.
- Current runtime with old HOST policy SHA-256: `33ad17ca8f227cf292e3b51ff2d01712f643fc916a4b97651b58ebb69a6cadb2`.
- Current winning HOST tape SHA-256: `e1c7693d633f1ecd486036e797baea555dcaddf1e0a409d4da397f3b2f3fbe17`.
- Physical divergence receipt SHA-256: `bf8851593eaa11eedb6fce0198abb8d5f42997140a5245e5177bf71da09c3058`.
- Timing prefix receipt SHA-256: `85dcee94d5a8078795ce485bf2522cf8e0bdc4414416fd2462b303e38a00e376`.
