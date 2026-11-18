# Active SCV cancellation in StarCraft 64

An isolated Ares run used a fresh Flash copy and ordinary controller
inputs to start a Terran Blood Bath custom game. The human Command
Center was selected with 50 displayed minerals and 4/10 supply.

| Observer step | Input or visible state |
| ---: | --- |
| 4,234 | C-Right opens the Command Center Build grid. |
| 4,262 | Stick Right moves from the center to the SCV tile. |
| 4,284 | A orders one SCV. |
| 4,325 | The job has a progress bar, supply reads 5/10, and Cancel appears below the SCV. |
| 4,326 | Stick Down selects Cancel. |
| 4,348 | A cancels the job. |
| 4,407 | The job is gone, the SCV remains available, and supply reads 4/10. |

The display shows 14 minerals in the active-job sample and 46 after
cancellation. Workers were still mining and the HUD animates mineral
changes. The exact integer balance and refund remain open. To isolate
the refund, hold the economy fixed and
observe the wallet before admission, after admission and after cancel.

The GBA `cancel_queue()` removes the active entry, resets its progress
and refunds its mineral and gas costs. The N64 observation verifies
active-job cancellation and supply release. It leaves the amount of the
GBA refund unqualified against N64 play. [GBA queue code](../../starcraft/game.c)

The native observer exited normally at step 4,408 after two requested
visual samples. The original ROM and Flash hashes stayed unchanged.
The private manifest SHA-256 is
`2ae945070cc0d83b8201073fc3a03ab2b43691ae162bf1de37d516449a5f1092`.
