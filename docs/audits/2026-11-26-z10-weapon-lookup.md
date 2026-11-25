# Indexed weapon art on Zerg10

Terran10, Protoss10 and Showcase compile the indexed weapon image and
animation lookup. Zerg10 already depended on the same generated header but
compiled the binary-search fallback. Selecting the shared build setting in
the Zerg10 flag vector also reaches its matching HOST match author.
`CANONICAL_LOOKUP_INDEX=0` restores the old resolver for comparisons.

The normal candidate build and an isolated compile that added the index to
the previous release command produced identical 32 MiB ROMs (SHA-256
`84bb873c12a5e09a7ad115f94835a1d17528765e7d8106960375dccf7ccfd2e0`).
IWRAM and EWRAM endpoints were unchanged. The stack checker reports 448
bytes beyond the 512-byte margin, compared with 536 bytes on the control.

## Measured battles

A fresh mGBA process resumed each prior release battle save and replayed
512 original input updates after the resume checkpoint. Each candidate
passed every trace checkpoint and matched the control's endpoint fingerprint.
At every boundary, including the start (513 per case), both bitmap pages and
the palette matched byte for byte. The active page sometimes differed at
the input boundary because the VBlank interrupt flips asynchronously. Of
the 11 differing samples, the indexed build displayed the newer page in
seven and the control displayed it in four. Each pair was one update apart.

| Map | CPU seats | Control flips/s | Indexed flips/s | Cycles saved | Different displayed bitmaps |
| --- | --- | ---: | ---: | ---: | ---: |
| Outpost | P | 21.61 | 21.64 | 0.14% | 1 |
| Outpost | PZ | 19.49 | 19.54 | 0.26% | 1 |
| Outpost | TZP | 12.05 | 12.08 | 0.32% | 4 |
| Blood Bath | P | 26.06 | 26.23 | 0.68% | 0 |
| Blood Bath | TZ | 16.65 | 16.80 | 0.88% | 3 |
| Blood Bath | ZZZ | 11.69 | 11.91 | 1.85% | 2 |

Flip rates use the cartridge's VBlank page-flip counters and the mGBA guest
clock. Each run presented 512 pages. The counters also record how many
VBlanks each page remained visible, with a final bucket for seven or more.
These rates measure emulated presentation, independent of whether successive
pages contain identical art. Physical GBA SP timing remains open.

All ten retained Zerg chapter victory saves loaded, advanced or retried the
finale on the normal candidate, and reloaded the resulting pause save in
a fresh emulator with matching state and save bytes.

## Checks outside the battle window

For every 16-bit image ID and every 8-bit animation ID, the generated
indexed resolver returned the same row or null as the original resolver on
the actual prepared bank (151 images, 425 animations). All 14 original
arrays in the two headers had identical definition hashes.

In the 4,901-input Zerg 7 campaign cut, cycles fell from 9,327,831,341 to
9,325,062,555. At all 41 samples the mission state, depleted-site counters,
both framebuffer pages, palette and displayed selection matched.

At the end of each 512-input skirmish window, pressing START and saving
produced the same SRAM on both builds. Every such save restored its battle
fingerprint. In another 36 matched trials, a read-only emulator observer
timed the interval from sampling START until the VBlank handler selected
the first completed pause page. The six offsets per lineup were 16, 64,
128, 256, 384 and 512 updates past the resume checkpoint, followed by one
neutral update before START. Mean latency was 72.04 ms on the control and
70.86 ms with the index. Maximum observed latency was 97.03 and 96.35 ms,
respectively. Individual pairs ranged from 14.36 ms faster to 9.91 ms
slower, depending on VBlank phase. This measures guest time at a sampled
button edge; it excludes physical button scanning and LCD response.
