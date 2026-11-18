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
changes. The refund needed a second run with the economy held fixed.

The GBA can remove a single active entry, reset its progress and refund
its mineral and gas costs. The first N64 run verifies
active-job cancellation and supply release. [GBA queue code](../../starcraft/game.c)

The native observer exited normally at step 4,408 after two requested
visual samples. The original ROM and Flash hashes stayed unchanged.
The private manifest SHA-256 is
`2ae945070cc0d83b8201073fc3a03ab2b43691ae162bf1de37d516449a5f1092`.

## Full refund with the mineral line idle

A second fresh Ares Blood Bath run selected the four human SCVs with R
and ordered them onto open ground via the field Move command. All four
were visible there after cancellation. No further economy commands ran.

| Observer step | Settled HUD and job |
| ---: | --- |
| 4,485 | 50 minerals, 4/10 supply, empty queue |
| 4,660 | Zero minerals, 5/10 supply, one 50-mineral SCV in production |
| 4,857 | 50 minerals, 4/10 supply, empty queue |

The input at observer step 4,536 ordered the SCV. The input at 4,683
cancelled it. The samples followed neutral holds of 115 and 165 steps
respectively, allowing the wallet display to settle. This establishes a
full 50-mineral refund for this active SCV order from the visible HUD.
The original ROM and Flash hashes stayed unchanged. The private
manifest SHA-256 is
`d745a9bacc8f8ce485b351b6057973d0759b7d8214a2c77617f8c0f8948809f4`.

## GBA Blood Bath comparison

The shipping Z10 cartridge entered Blood Bath from fresh HOME with
Terran and three individually selected opponents. Ordinary A inputs
selected the Command Center and ordered an SCV. After twelve neutral
calls, the job was still at progress 14 and the wallet was zero.
Tapping L removed the active entry and restored 50 minerals. The HUD's
reserved supply was 4/10, then 5/10, then 4/10 at the three cuts. This
matches the one active SCV transaction observed on N64. The GBA ROM
uses its regular Blood Bath custom mode.

The read-only physical replay sampled calls 50, 64 and 66. Its receipt
SHA-256 is `5abbd6bc2135f86abc1819eece551fbb52cb7a3e650600fb99e76468011f4dda`.

## Cancel Last with two paid orders

In a fresh N64 Blood Bath run, the workers collected 122 minerals
before being parked away from the patches. The player paid for two
SCVs at the same Command Center, then selected the native Cancel Last
command.

| Native observer step | Minerals | Supply | Production |
| ---: | ---: | ---: | --- |
| 6,185 | 122 | 4/10 | Empty |
| 6,444 | 22 | 5/10 | One active SCV and a second SCV in slot 2 |
| 6,581 | 72 | 5/10 | The first SCV remains active and its bar advances |

The GBA physical comparison used ordinary buttons from HOME in three
fresh emulator processes. The shipping Z10 and tail-cancel build
received identical inputs. A neutral run replaced the final L tap
with two idle input calls to account for an eight-mineral deposit
at that boundary. Both cancellation runs refunded 50 above the
neutral wallet. The candidate matched the neutral head progress.

| GBA sample | Queue before | Head progress before → after | Final wallet |
| --- | ---: | ---: | ---: |
| Idle control | 2 | 20 → 22 | 22 |
| Shipping L | 2 | 20 → 1 | 72 |
| Cancel Last | 2 | 20 → 22 | 72 |

The shared GBA control now removes the last paid queue entry and
keeps the head index and progress while another order remains. A
wrapped full mixed Marine/Firebat queue passed under both host
control variants, including the refund and slot order. The candidate
ARM build passes the existing memory guards. The same two-job
result repeats on a separate build based directly on shipping
source `2655cb22`. A single-item cancellation on the candidate
also clears the queue and refunds 50. The P10-feature host fixture
passes the same L-release gesture and the Gateway refund checks.
The HUD still displays
6/10 with the two paid SCVs, then 5/10 after cancel. Observe native
admission at cap and the handoff to a blocked new head before changing
when waiting jobs count against supply.

- Native manifest SHA-256 `60cbcdb014c1bf55aaee2433f5a856046256a39db8fbe1f14d78a94b83275fb0`.
- GBA shipping cancellation receipt SHA-256 `bb3754af6b0a9edd8498f8414fbe18d94bce25e9d0603592a4ce0f6e0369b0d7`.
- GBA idle control receipt SHA-256 `87566d0361055fa101aaf96ae9ecc85f8a73adf266c871d6786f1ba498999817`.
- GBA tail cancellation receipt SHA-256 `eff219e30425e349352e74a513e0bd9ea8cceb0b28f7446a766a360ff750ab28`.
- Release-based tail cancellation receipt SHA-256 `1c77e70dbc197d08804cdc6322bc28bb94f5e892a1add3060fbb70bb72012431`.
- Candidate single-item cancellation receipt SHA-256 `f9f537a0994264632efbe5e8c5d8a927dae1e55dca6318baeff494028ea23f0b`.
