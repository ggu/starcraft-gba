# Zerg campaign continuity after the Custom Game Larva change

## Z3 from the Z2 victory

The standard Zerg10 cartridge loaded the Z2 result earned on the same
cartridge. Pressing A on that result started Z3 at frame zero. The route used
ordinary buttons to build the economy, train Mutalisks and finish with a
coordinated ground attack. Each stage loaded the previous stage's SRAM in a
new emulator. The result at frame 68,639 retained progress bits 0, 1 and 14
(0x4003), recorded zero mission faults, and left the friendly Chrysalis alive
at 170 HP. A fresh mGBA instance loaded the same world and re-exported
identical SRAM. A on that loaded result started Z4 at frame zero.

| Artifact | SHA-256 |
|---|---|
| Standard Zerg10 cartridge | `2d1d00751fbc9e82a012823ec54fedda54dd3a55b0cf5d1a001be2de268fe7fe` |
| Z2 victory input | `393a8325d5cc34625aec5a3e8de6e5d5dd9630195421aa95f4589b6ae30fc9e4` |
| Z3 victory and fresh SRAM re-export | `793660f3b86f374647357ffe100555c82d9c86f139bd96ef4c3865d1bfffa018` |

The [recorded inputs](../../tests/fixtures/zerg-z3-paid-route.json) include
the expected SHA-256 for each of the 32 stage saves. With matching private
inputs, run the following command using a new output directory. The receipt
includes per-stage guest cycles and elapsed updates.

```sh
python3 tools/sc64_z3_linked_victory.py --rom "$Z10_ROM" --elf "$Z10_ELF" \
  --runner "$RUNNER" --previous-victory "$Z2_SAVE" --out "$OUT"
```

The independent replay matched all 32 expected save hashes. Across 69,243
runner updates, the emulator measured 49,415,530,907 guest cycles, or
23.51 updates/s at the GBA clock rate. This average covers the recorded
inputs, including menu and reload inputs; it does not measure displayed
frame changes or physical-device pacing.

## Z4 from the Z3 result

A fresh instance loaded that Z3 result and advanced to Z4 at frame zero.
The recorded opening left the mission active at frame 28,754. Eight
subsequent controller stages carried the campaign through rebuilding the
Spawning Pool, completing the Lair and Spire, massing air units and attacking
the objective. The result showed victory at frame 64,147, zero mission faults
and all four completion bits (0xc003). A fresh emulator loaded the same
93-unit, 66-building world and re-exported the same 32 KiB journal.
The Z4 victory and readback SHA-256 was
`d1daba6a15928cebeea2ee9158ff860c7708325cd1d8798e58d722801e609b0d`.
