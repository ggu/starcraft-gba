# Remove the first-use weapon bounds scan

The imported weapon renderer can skip an effect whose source art cannot
reach the viewport. The old bank calculated the fallback bounds on the
first eligible trace by scanning every crop, overlay and LO offset. The
generator now emits the same whole-bank bounds. Banks without the constant
keep the original scan. Unknown overlay opcodes and unrepresentable
radii still disable the whole-bank cull.

In the [mixed Z10 skirmish](../CARRIER-PROJECTILES.md#mixed-blood-bath-battle-from-blank-sram),
the instrumented first update after title L spent 1,206,617 cycles in
tracers, including 1,204,784 in cull checks. It spent only 28,549 cycles
drawing unit bodies. The scan covered 1,218 frames, 165 overlays and
1,169 LO offsets in this authenticated bank. The generated radius of
66 × 61 matches the original C scan, including the mirrored crops and
signed child offsets. Comparing the original bank and the regenerated
one shows identical data apart from the generated constant. The indexed
variant preserves the same constant, with its other content unchanged.

The measured pair uses ordinary title L and follows 1,000 neutral game
updates on the combined Z10 preview with three AIs in Blood Bath. The
candidate actually includes the regenerated bank; the first experiment
accidentally linked the older bank and retained the spike. The corrected
pair matches 43 gameplay checkpoints, the 240 × 160 indexed page and
palette at 58 rendered cuts, and the complete 32 KiB SRAM after an
ordinary pause save.

| Emulated cycles | Old bank | Generated bounds |
| --- | ---: | ---: |
| First complete game update and draw | 2,999,527 | 1,795,375 |
| Unit and effect draw portion, first update | 1,240,613 | 36,337 |
| Worst complete update in the window | 2,999,527 | 2,311,989 |
| Mean complete update | 1,006,460 | 1,006,922 |
| p95 complete update | 1,365,093 | 1,371,412 |

This removes roughly 40% of the first-update cost. The mean rises 0.05%
and the p95 rises 0.46% in this window. The battle remains well above the
561,792-cycle 30 fps budget. Neither save behavior nor weapon graphics
change. Native GBA SP timing remains open.

| Local evidence | SHA-256 |
| --- | --- |
| Render-pass breakdown | `06a879d2e8484f2fdf4061d0430e77891c4968cc4ba53d9c320055629174f9ce` |
| Corrected timing and visual pair | `da9119b53e5ce77160dac703f30e6fc6e10e2da26390c2c1860ea0cd52314137` |
| Compared indexed-page and palette hashes | `bf993d1be51143dfbbd0ce0ac0d3359bd1255d917376e9be1cbc51ff4316b0b4` |
| Candidate cartridge used for the pair | `0aeddf0115cde90799301bd6f1df14f868fa63093946d5570dfc168cc95c5fd7` |
