# Combined routing, Command census and save-reader options

The November 13 KODAK runtime is the control. The composed ROMs enable
only the [bounded facility FIFO](2026-11-13-bounded-facility-fifo.md),
[direct Command census](2026-11-13-p10-command-census.md) and
[typed save byte reader](2026-11-13-typed-save-reader.md). The wide point
FIFO, nearest-first P10 depot policy and packed mineral probe remain
off. This is a qualification build, separate from the files copied to
KODAK.

All four 32 MiB ROMs pass the existing linker RAM guards and ROM header
verification. IWRAM ends at `0x03004908` (Terran), `0x03004da0` (Z10),
`0x03004c70` (P10), and `0x03004ec0` (Showcase). The census adds eight
EWRAM bytes to Z10 and P10. The Showcase IWRAM end remains only 320 bytes
below its guard.

On the normal P10 cartridge, the complete ordinary HOME replay wins
at frame 46,057. All 20 gameplay observations, ten briefing pages,
14 dialogue acknowledgments and the terminal state match. Over 47,194
calls the average falls from 1,907,674 to 1,743,571 guest cycles
(−8.60%, 8.79 to 9.62 emulated updates/s). The maximum measured loop
falls from 76,499,788 to 36,633,430 cycles. These are emulated loop
measurements including waits; they do not establish GBA hardware FPS.

Four paired physical custom tests match the control's resolved seats,
120-update game state, SRAM bytes, and fresh-process 60-update
continuation. The two full Z10 three-CPU replays match every state
checkpoint and final defeat:

| Map | Matching checkpoints | Final frame | 1,000-loop mean cycles | Window maximum cycles |
| --- | ---: | ---: | ---: | ---: |
| Outpost | 76 | 18,621 | 1,320,506 → 1,256,742 | 3,816,683 → 2,253,203 |
| Blood Bath | 77 | 18,986 | 1,186,105 → 1,194,986 | 2,278,463 → 2,447,406 |

Blood Bath regresses in this window: +0.75% mean and +7.41% maximum.
A separate replay reproduces the new maximum at call 11,539:
2,235,129 → 2,447,406 cycles on the same closed-menu update. Unit
orders rise by 215,833 cycles, accounting for the increase; map and
HUD work do not. The old maximum at call 11,092 rises by 8,166 cycles.
A paired instruction trace of call 11,539 puts 361,686 → 570,727 cycles
in the facility lookup, with matching route builds (2,336) and hits
(29,673). The new fill takes 470,415 cycles including only 36,148 for
FIFO expansion. Grid initialization and packing are part of the
remaining cost. A cheap scalar path for small, early-stopping fills is
the next comparison. Keep the composition optional and measure the
build on hardware before enabling these options by default.

The same composition wins Z7 from HOME on Z10 at frame 194,998, matching
all 253 named observations from the retained tape. It averages 1,800,367
guest cycles, or 9.32 emulated updates/s. The previous qualified Z10
control averaged 1,794,976 cycles on the same tape (9.35); this is a
comparison with that older ROM rather than an exact KODAK control.
The result supports compatibility for this route and no substantial
steady Z7 gain.

A paired normal Z10 Full Circle continuation starts with the same
cartridge-format SRAM at frame 2,527 and wins at frame 59,997. All 61
raw world checkpoints, 24 drawn page/palette/fog samples and nine
authored acknowledgments match. The two sampled 1,000-update mean
costs rise from 1,479,357 to 1,485,626 (+0.42%) and from 1,507,022
to 1,508,665 (+0.11%). The latter window's maximum rises from
3,418,866 to 4,055,372. The composition remains optional.

- Z10 Full Circle paired receipts (control / composition):
  `f6122c66becab2b3fb62655c959ba49c88bd75745d71bdd65151ab029f6a2499` /
  `70d103064302b68240f7dad759169e18a50d1e6afa808dc5d70cecc597c15411`.
- Z7 composed replay receipt: `3be0f473121bae6d3f58f21c13ed10ba2f21fb38fd117a8b90f6cf42652ea5ee`.
- P10 composed ROM SHA-256: `6ac056503bf28b8ce7af09b3313b90b78f764cfd9a3fd805e75ae12d5bd08c97`.
- P10 composed replay receipt: `51f06c06340b5ddd7ebbff37809093d111b098f6f14a53868287521ff6b06f61`.
- Z10 custom receipts (Outpost / Blood Bath):
  `daaccc740563a00da2243aaf7a5e82938f1949d97cd7c86c0f814d1b003e4d2f` /
  `50a473c89b9c46f1e53c02d5c35fee03a4a3f4350deca52810c0c46e6f682cb0`.
