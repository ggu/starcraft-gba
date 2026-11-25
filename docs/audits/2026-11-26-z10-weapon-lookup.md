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
the 11 different displayed bitmaps, seven showed the page most recently
written one update earlier on the indexed build and four on the control.

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
