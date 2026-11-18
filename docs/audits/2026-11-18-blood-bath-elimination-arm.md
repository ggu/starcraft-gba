# Blood Bath elimination on ARM

This opt-in build carries forward the [CPU retirement host preview #445](https://github.com/ggu/starcraft-gba/pull/445).
It uses the same resolved starts and allied-CPU options on Blood Bath. A
player at zero buildings can transfer its surviving units to neutral owner
11. Mobile units and boarded passengers keep their slots. The loaded
passenger keeps its transport link. Transfer waits for all of a CPU's
actors to qualify. On human defeat, the usual result still publishes when
an unsupported linked actor prevents transfer.

The preview stores eliminated CPU ranks in the existing resolved-start
word. The neutral Overlord research bit uses the existing transport trailer.
A retired Zerg CPU can seed that bit. A terminal Blood Bath save can also
retain the completed research of a defeated Zerg human after transfer.
The host fixture covers an Overlord with a boarded Drone and the fresh
terminal load. Older readers can reject either preview marker and fall back
to an earlier supported journal bank.

## Ordinary-input checks

These observations used the Terran ARM preview on the `ca822ea7` source
release and normal physical buttons in mGBA. Both tests loaded a verified
frame-3,001 save from the previous Blood Bath start preview.

- In a named three-CPU Free for All, the Zerg CPU retired at world frame
  **13,905**. Its surviving Overlord changed to neutral owner 11. The human
  and the two other CPUs still had buildings. Both pause-save writes passed.
  A fresh emulator loaded the marked save and matched 900 updates against
  uninterrupted play. The predecessor Melee preview stayed on HOME when it
  encountered two marked journal banks.
- In a named three-CPU Melee, the human lost at frame **11,706**, the same
  frame as the prior preview. Both human survivors transferred to neutral.
  The terminal SRAM loaded in a fresh emulator with the same result and
  recorded world.
- The Z10, P10 and Showcase previews opened one, two and three CPU lineups
  across the two Blood Bath modes. Each saved after 1,400 physical updates
  and matched another 300 after a fresh load. These cases start and resume
  a match before terminal play.

The four preview ROMs passed the existing link guards, and their four
flag-off rebuilds were byte-identical to the corresponding release at
`ca822ea7`. The host test passed under ASan/UBSan with all four prepared
product vectors. Use the existing prepared private inputs with

```sh
make sc64-native-skirmish-elimination-arm
make test-sc64-native-skirmish-elimination
```

The ARM target builds four independently identified preview cartridges.
Their files and the input data stay in ignored local storage.

## Native qualification still open

The [native end-rule audit](2026-11-13-n64-blood-bath-end-rules.md) identifies
defeat at zero buildings, neutral transfer and opponent-slot victory.
The latest [live building loss](2026-11-14-native-result-opening.md) cut one
owner's count from six to five, and all result bytes stayed zero. A live
native final-building event with a pending shot is still needed to determine
whether a transferred target keeps, rejects or cancels that shot.
Simultaneous outcomes, coupled Carrier, Scarab, Egg, Ghost and Battlecruiser
state, and physical-GBA timing still need qualification before default
enablement.

| Verified result | SHA-256 |
| --- | --- |
| CPU retirement and 900 resumed updates | `01c7860a07db37d010e9967e254cf74da6860336dc74ca6d14af8b816d327ec7` |
| Human defeat and terminal SRAM | `1c62ced8deee8c6159854dd9b61b7b3781cdfeefffdf463267fe2f4aa519fab8` |
