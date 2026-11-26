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

## Independent Z7 control on the standard Z10 cartridge

Starting from HOME, the retained 270,585-call recording won at frame
267,585 with zero mission faults and zero enemy buildings. Comparing the
206 observations against the older unlimited-reserve controller recording,
the gas balance differed at 67 checkpoints. Every other recorded field
matched. The finite-reserve cartridge finished with 6,646 gas. Its fresh
victory re-export was byte-identical, SHA-256
`3f3f96fe3634cb85e0347fdb17c68efe5a136f5d3123f87d716f8cdeb93ba159`.
The recording SHA-256 was
`022d0f08d4020ab86b3ac69fcb8dfe430b7837c3817d37ae785e241a62cd3ad1`.
A separate run loaded the actual Z6 readback, advanced to Z7, and matched
all 206 same-cartridge observations, including every gas balance. The final
result preserved the seven completion bits (0x7c003). A fresh boot matched
the world and exported identical SRAM, SHA-256
`05c8bf28416eef8c52cfcd9d59901e31b1ffe6f74598a023a321201c26939639`.

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
The newer packaged Z10 ROM at `51c7f18f` independently cleared the
same HOME route and re-exported the identical journal. The recording SHA-256 is
`f0479c6c302d377f792f23a7dfc5d61d8b22f12779f4e97e99dcd14440509685`.

From the earned Z7 victory, the same recording matched all 29 checkpoints
and won at frame 70,229 with both Nexuses destroyed and Kerrigan at 400 HP.
The fresh-loaded victory re-exported byte for byte with SHA-256
`39ef09adaccc4a8e3aabc01cfddbc4ed261ad714fa96225234a57ffe500b8739`.

Use matching private generated inputs and a new output directory. Omit
`--previous-victory` for the HOME replay. Supply the actual Z1–Z7
completion journal to check the linked campaign instead.

```sh
make build/test-sc64-z8-normal-home
build/test-sc64-z8-normal-home build/z8-gather.jsonl 180000 --gather-west
python3 tools/sc64_z8_linked_victory.py --trace build/z8-gather.jsonl \
  --rom "$Z10_ROM" --elf "$Z10_ELF" --runner "$RUNNER" --out "$OUT"
```

## Z9 and Z10 from the earned Z8 result

The archived Z9 route first diverged at call 8,425, with one fewer friendly
unit and 24 fewer minerals on the standard cartridge. The archived Z10 route
first diverged at call 8,785, with one fewer enemy unit. Both archived tapes
later restarted their mission. Regenerating the controller recordings with
the standard cartridge flags produced winning routes on this ROM. Z9 used
`--guarded-air` to assign six gas workers, wait for 22 Mutalisks before the
air attack and allow return orders with 300 countdown units remaining.

A on the fresh Z8 RESULT started Z9. All 30 recorded checkpoints matched
through victory at frame 67,745. A fresh HOME load restored the same RESULT,
and its re-exported SRAM advanced directly to Z10. All 26 recorded Z10
checkpoints matched through victory at frame 57,633. Fresh HOME loads held
both results for eight neutral updates and re-exported identical SRAM.
The last journal retained all ten Zerg completion bits (`0x3fc003`).
These runs complete the standard Zerg campaign from blank SRAM on the
cartridge identified above.

| Artifact | SHA-256 |
|---|---|
| Z9 controller recording | `5270bdc222c6bc95e441a8315674434f53d298064c0e3203eab715844633c71d` |
| Z9 victory and fresh SRAM re-export | `7d42bffeb2f5a5486202dba0861bb1cb12a2d34e4ee203a31785cc3ebb4d683d` |
| Z10 controller recording | `39d348a2f2ac61ce1e55fc81740503e519a7ee99777fc0f79a0991ea7d33e82e` |
| Z10 victory and fresh SRAM re-export | `1123eb7c9e8977228e33d24db85f14953a202da2a4c731bfdb4b6d940768b954` |

Use the matching prepared inputs and a fresh output directory for each run.

```sh
make build/test-sc64-z9-normal-home build/test-sc64-z10-normal-home
build/test-sc64-z9-normal-home build/z9-guarded-air.jsonl 200000 --guarded-air
build/test-sc64-z10-normal-home build/z10-standard.jsonl 200000
python3 tools/sc64_z10_fifo_normal_replay.py --chapter 9 \
  --seed "$Z8_READBACK" --trace build/z9-guarded-air.jsonl \
  --rom "$Z10_ROM" --elf "$Z10_ELF" --runner "$RUNNER" --out "$Z9_OUT"
python3 tools/sc64_z10_fifo_normal_replay.py --chapter 10 \
  --seed "$Z9_OUT/readback.sav" --trace build/z10-standard.jsonl \
  --rom "$Z10_ROM" --elf "$Z10_ELF" --runner "$RUNNER" --out "$Z10_OUT"
```
