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

## P4 campaign and normal cartridge builds

The normal release at `b7e69893` served as the control. The paired build
changes the canonical pack selection for Pylon, Gateway and Forge. The
existing generated sidecar and packed compositor preserve the source art,
clip and painter order.

The P4 replay loads an earned P3 result through HOME and uses the ordinary
linked input tape. At tape call 17275, a guest call-stack profile of 16
updates assigns 55,275 cycles per update to byte-sprite blits from the
completed-building renderer. A Forge and Gateway are in view. At call 40127,
the same compositor costs 20,564 cycles, and the mapped construction image
costs another 60,542 cycles. The P4 checkpoint at call 40127 yields an
ordinary pause journal at world frame 39600. After loading it anew, a
56-position camera sweep keeps visible actors and surrounding fog in view.

| Normal product and window | Calls | Mean cycles, control → packed | p95 cycles, control → packed | Max cycles, control → packed |
| --- | ---: | ---: | ---: | ---: |
| P10, P4 from call 17275 | 128 | 802,420 → 773,269 | 885,365 → 856,210 | 939,764 → 906,838 |
| P10, P4 from call 40127 | 128 | 1,022,558 → 1,014,581 | 1,058,829 → 1,052,214 | 1,161,591 → 1,155,002 |
| P10, P4 from fresh SRAM with camera sweep | 128 | 1,071,479 → 1,060,520 | 1,194,635 → 1,184,508 | 1,269,194 → 1,264,670 |
| P10, scrolled calls from that sweep | 56 | 1,135,527 → 1,121,187 | 1,214,353 → 1,203,717 | 1,269,194 → 1,264,670 |
| T10, earned T5 assault journal and camera sweep | 128 | 592,943 → 592,943 | 712,859 → 712,859 | 767,194 → 767,194 |
| Showcase, Blood Bath Melee against Protoss | 384 | 815,697 → 815,697 | 1,547,471 → 1,547,372 | 1,873,592 → 1,874,008 |

The P4 cuts at tape calls 17275 and 40127 gain 3.63% and 0.78% respectively.
Whole-loop cycles in the scrolled portion fall 1.26%. The Showcase pair
plays the same physical Blood Bath tape with the same two native start
anchors. It ends with 57 units and 12 buildings. In nine boundaries from
calls 9013 through 9021, the camera brings an enemy Pylon across the right
clip. Actor draw cost on that segment falls from 254,985 to 248,996 cycles
per call. The complete 384-call mean stays level.

Both indexed bitmap pages, palette, world state, camera and sampled fog
match all applicable checks. At one late P4 input boundary the display bit
selects the adjacent completed frame, and both builds present the same page again at the
next boundary. Hardware pacing is unmeasured.

The three profile-specific ASan/UBSan packed-word oracles passed 720 Terran,
720 Showcase and 1,080 P10 pixel cases with clipped and phase-shifted source
frames. The four normal builds passed the ROM verifiers. The Z10 ROM
matches the control byte for byte because it already packs these buildings.
The three linked stack guards pass with unchanged hot-byte counts and spare
space. ROM contents grow by 52,920 bytes on T10, 52,912 on P10 and 57,336 on
Showcase. The four products fit their normal 32 MiB cartridges.

Two alternative mapped-sprite implementations were also tested from the P4
pause. The generic ARM pair loop costs 0.98% more cycles. A WarpAnchor lookup
with an identity row saves 0.70% and matches indexed pixels, but the current
change is limited to the established packed-building path.
