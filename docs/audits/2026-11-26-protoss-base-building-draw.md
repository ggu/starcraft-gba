# Packed Pylon, Gateway and Forge art

The opt-in build renders the same three source frames using the existing
word-packed sprite drawing code. The Zerg cartridge already uses packed
versions of all three. In a prepared checkout, build the normal P10 ROM with
`make CANONICAL_PROTOSS_BASE_BUILDING_WORDS=1 build/StarCraft-Protoss-P10-Transport1-GBA.gba`.
The same setting applies to the normal Terran and Showcase targets. Omit the
option to use the default bank.

The three-AI Protoss Blood Bath profile motivated
the experiment. Across 16 sampled updates on the FIFO trial, calls from
the completed-building renderer into the byte sprite renderer averaged
78,989 emulated cycles per sample (5.5 calls).

## Current release economy, Protoss cartridge

The baseline rebuild matched the retained finite-gas release ROM byte for byte.
Each row resumes a winning battle pause, applies 512 original controller
inputs and measures elapsed guest cycles. Opponents appear in lobby order.
Each input boundary records both raw bitmap pages, the palette and the
visible bitmap. Displayed differences count samples where the two builds
presented different pixels. These measurements are from mGBA.

| Map / CPUs | Checks | Fewer cycles | Updates/s, baseline → packed | p95 cycles, baseline → packed | Displayed differences / 513 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Outpost / PTZ | 3 | 2.14% | 15.55 → 15.89 | 1,513,467 → 1,499,563 | 0 |
| Outpost / T | 4 | 3.29% | 16.41 → 16.97 | 1,606,261 → 1,573,374 | 0 |
| Outpost / TZ | 2 | 0.64% | 16.60 → 16.70 | 1,517,263 → 1,512,628 | 2 |
| Blood Bath / T | 6 | 2.00% | 10.08 → 10.29 | 2,525,428 → 2,477,815 | 3 |
| Blood Bath / TZ | 5 | 1.77% | 13.01 → 13.25 | 1,921,801 → 1,901,498 | 2 |
| Blood Bath / TZP | 6 | 1.94% | 15.54 → 15.85 | 1,448,918 → 1,427,902 | 2 |

Every paired checkpoint and ending world fingerprint matched. Both raw
bitmap pages and the palette matched at all 3,078 boundaries. The visible
page can change at a different sampling boundary when a build reaches
VBlank earlier. Hardware pacing remains unverified.

On the separate FIFO trial, the ordinary-input P10 Outpost and Blood Bath
victories also replayed in full. Outpost matched 259 checkpoints and 172
after a fresh pause load. Blood Bath matched 499 and 345. Both pause and
victory SRAM files matched the original trial byte for byte, including
fresh victory reloads.

Terran's two 512-call FIFO battle cuts changed by less than 0.001% of
elapsed cycles. All 513 sampled raw page pairs, palettes, actor arrays
and camera positions matched in each case. A light Showcase comparison
also matched but did not reach a busy battle. Both cartridges made
byte-identical pause saves and matched queue and world state after a fresh
load and 128 further updates. All three tested products fit in 32 MiB.
The P10 and Terran stack guards retained 816 and 240 bytes respectively
beyond their required margins.
