# Custom Game defense targeting

Completed static defenses in Custom Game now share a spatial index of
live units for the duration of a defense update. The pass builds the
index when the first weapon is ready and checks nearby cells in each
query. It retains the same hostility, coverage and target rules,
including the strict circular range and lower actor slot on ties.
Each query checks the actor's live state after earlier defenses fire.
If a live unit has an out-of-domain coordinate or owner, the update
uses the scalar target scan. A full impact pool leaves the weapon ready
and preserves the existing next-update firing order.

Campaign target queries use their existing dispatch and storage. The
Custom Game wrapper holds its temporary cell links on the regular
stack and runs from ROM. All four normal IWRAM and EWRAM endpoints
match the Carrier release. The linked Custom Game wrapper uses a
1,224-byte stack frame before its callees. Setting
`CANONICAL_DEFENSE_CUSTOM_CELLS=0` reproduces all four preceding ROMs
byte for byte.

## Verification

`make test-sc64-custom-defense-cells` compares the normal Custom Game
dispatcher to the scalar pass with prepared Terran and P10 inputs.
Sanitizer checks compare actors, weapon effects, random seed and
pending impacts. Both profiles pass 800 randomized worlds. Explicit
fixtures cover equal distance, an early kill, strict range, fog,
cooldown, invalid input fallback and a saturated impact queue.

All 54 ordinary Custom Game starts and fresh HOME continuations pass
on the three race cartridges. The cases cover both maps, every human
race and one to three CPU seats. All 108 paired journals match the
preceding release byte for byte. Campaign openings and the first
Showcase scene match both indexed pages through frame 121. The P10
finale restores at frame 36,761 and wins at 46,057, with 68 matching
checkpoints and identical result SRAM.
The Showcase Carrier preview matches 20 Carrier Approach scene cuts
and 35 ordinary Blood Bath cuts, including the fresh load and SRAM.

Separate Z10 stack-canary ROMs replay the same 3,600 busy Blood Bath
updates. The 31 sampled worlds and both indexed pages match the
ordinary builds. Both versions leave at least 6,636 continuous bytes
of untouched system stack above the IWRAM code guard during this run.
The canary results cover this emulator replay.

## Paired emulation timings

The 3,600-update Blood Bath replay starts at an ordinary Custom Game
pause with three AIs. All 31 world and display checkpoints, 16 fresh
continuations, route counters and the pause journal match. The
three Outpost Run tapes use each cartridge's ordinary controls.
Their 800-update windows match route counters. Across all three tapes,
270 world and indexed-page cuts match. At P10 call 13,042 the displayed
page bit differs while both indexed pages match.

| Workload | Previous mean guest cycles | Cell query mean | Mean change |
| --- | ---: | ---: | ---: |
| Z10 Blood Bath | 880,942 | 871,702 | −1.05% |
| Z10 Outpost Run window | 1,201,159 | 1,148,720 | −4.37% |
| Terran Outpost Run window | 1,046,185 | 1,012,464 | −3.22% |
| P10 Outpost Run window | 1,250,523 | 1,198,141 | −4.19% |

Blood Bath p95 falls from 1,171,673 to 1,163,385 cycles. All three
Outpost window peaks fall, by 55,888 cycles on Terran, 69,859 on Z10,
and 46,233 on P10. Their busy windows remain above the 30 fps cycle
budget. Physical frame pacing and an ordinary N64 static-defense
acquisition and impact capture remain open.

The instruction observer attributes all 33 sampled updates and matches
all 31 world checkpoints on the current Z10 ROM. The defense pass
averages about 23,000–24,500 cycles across the four sampled bands.
Orders still spend 2.44 million cycles on the worst sampled update.

| Evidence | SHA-256 |
| --- | --- |
| Z10 Blood Bath | `155359480c48f463947956bd6108aa9bceb446c7d2e184c51df215569dc4bacf` |
| Z10 Outpost Run | `5a5e3016fd5fe1662b5ad344208acf258835fdb9693aa253de31b39ea260b6f1` |
| Terran Outpost Run | `5341d5d75efc3041cb69802a82c79617979b00d0a9deb7b534e2a7a97b7a619f` |
| P10 Outpost Run | `8ddc8b5bca1caffb3ef2fbfd8e4ca8079a3dcc19529e0eec8879f7ffe193d8d1` |
| Custom Game save matrix | `5a8a24baa08b9cb2c7226d724d3569956a8cd4ab8a5b3f2b06284f0a51a8272c` |
| Cross-version journal bytes | `9927a866e67ad60544daddf487988d704a34281634f71cde22822815ab58a2c3` |
| Blood Bath system stack | `2a643f59885ac779748a48652b0d4d2d5f4e912e4d93d307ef36a0d5031170cb` |
| Current instruction attribution | `e417d4e2226815acefb47c1c561080febae030edd8a86cce9e3d6582d1684d58` |
