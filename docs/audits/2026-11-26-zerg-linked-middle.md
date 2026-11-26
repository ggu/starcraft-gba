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

## Z5 and Z6 from the Z4 result

The fresh Z4 readback carried into Z5 on the same standard Zerg10 ROM.
Ordinary A on the restored RESULT started Z5 at frame zero. The 14 retained
controller stages loaded only the saves produced earlier in this run. Each
stage endpoint matched the reference world fingerprint. Z5 won at frame
18,974 with all five completion bits (`0x1c003`). A fresh HOME load
restored the same result and world, held it for eight neutral updates and
re-exported identical SRAM. That readback was the input for Z6.

Ordinary A on the restored Z5 RESULT started Z6 at frame zero. A separate
comparison loaded stage saves from package `5f758095` and the newly earned
stage saves on the current cartridge. The first 20 observed worlds matched
after load.
At the stage 21 storm recovery checkpoint (frame 100,092), two unit slots
had 80 shields each on the current route and 60 each in the archive. Later
world comparisons also differed. The archived saves were used only for
comparison. The campaign replay loaded its own saves at every stage.

The first 50 stage endpoints matched the recorded frame, screen, mission,
completion mask and win/loss state. Stage 51 paused at frame 174,369
instead of 174,305. The recorded final input sequence ended with the
mission active at frame 177,219, with 15 hostile units, 21 hostile buildings
and Kerrigan at 400 HP. Ordinary pause and save inputs produced a recovery
journal at frame 177,220.

The recovery trained 28 paid Hydralisks and saved at frame 194,496. Four
replacement attacks cleared the objective regions while Kerrigan stayed
out of combat. The last attack ended in victory at frame 212,428, with
ten Hydralisks surviving, Kerrigan at 400 HP, zero mission faults and all
six completion bits (`0x3c003`). A fresh HOME load restored the same RESULT
and observed world, held for eight neutral updates and re-exported the
same SRAM.
An independent replay from the recovery journal matched all 70 recorded
observations and all five stage SRAM hashes through victory.

| Artifact | SHA-256 |
|---|---|
| Z5 victory and fresh SRAM re-export | `5c96d9961a7e32a6366b88a9ba7fc999d364bb6a506a67827a5b4ce801d6a9f2` |
| Z6 recovery pause save | `999647ddb2115bc568859ffcb65def51e8c2af0753449e4d1fed2a2a0c554d68` |
| Z6 victory and fresh SRAM re-export | `5485c06c84a64a26001ae3a80daf4466bdf9c3ba3ecd7cde96371ca195287b04` |

The stage recordings and emulator receipts remain local. This is
ordinary-button mGBA evidence on the standard ROM identified above.

## Z8 controller on the standard Z10 cartridge

The previous winning recording first diverges at call 9,696. It expects
55 enemy units where the standard ROM has 54. The default controller,
regenerated with the standard cartridge flags and dialogue handling,
destroys the eastern Nexus but loses Kerrigan at frame 91,637.

The optional `--gather-west` controller delays the western assault until
frame 66,000, requires the forward group to have at least three quarters
of the largest group's strength before following it, and advances a waypoint when
60% of the group is within 210 pixels. All three adjustments drive
ordinary controller inputs. From HOME the cartridge wins at frame 70,229
(call 71,502). Both Nexuses are gone and Kerrigan has 400 HP. Fresh SRAM
re-export SHA-256 is
`6a505816f87fb71ffa218f73a72e262477c014b6de7a19d622099ec4766ecf0f`.
The recording SHA-256 is
`f0479c6c302d377f792f23a7dfc5d61d8b22f12779f4e97e99dcd14440509685`.

Use matching private generated inputs and a new output directory. Omit
`--previous-victory` for the HOME replay. Supply the actual Z1–Z7
completion journal to check the linked campaign instead.

```sh
make build/test-sc64-z8-normal-home
build/test-sc64-z8-normal-home build/z8-gather.jsonl 180000 --gather-west
python3 tools/sc64_z8_linked_victory.py --trace build/z8-gather.jsonl \
  --rom "$Z10_ROM" --elf "$Z10_ELF" --runner "$RUNNER" --out "$OUT"
```
