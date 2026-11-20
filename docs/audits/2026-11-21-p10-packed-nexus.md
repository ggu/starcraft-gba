# Packed Nexus draw in P10

The normal P10 cartridge selects the prepared Nexus frame alongside the
Hatchery and shelled Overmind. Run `make P10_NEXUS_WORDS=0 verify-protoss-p10`
for the earlier two-frame bank. The three selections can also be toggled
independently. The shared Mode 4 renderer matches the exact source frame
pointer and uses the existing clipped word blitter.

The Nexus adds 41,888 packed bytes across four alignment phases. The
linked text grows by 41,928 bytes within the existing 32 MiB cartridge.
Both builds use the same RAM endpoints, `0x03004e20` IWRAM and
`0x0203b52c` EWRAM. Run `make test-sc64-p10-nexus-words-pixels` with
prepared inputs. The sanitizer test compares all 38,400 indexed pixels
in 360 draws covering the three frames, their four alignments, and
clipping along both viewport axes.

## Blood Bath

The current P10 cartridge on `bf846b88` and the packed candidate both
start ordinary Custom Game with a Protoss player against Terran, Zerg
and Protoss CPUs. Controller input selects the lobby, then the camera
stays at the player's base through game frame 4,999. The 21 selected
gameplay checkpoints match the actor, resource, fog and route-state
hashes, palette, active page and both Mode 4 pages. The 2,000 timing rows
match frame numbers, camera coordinates and route counters. Both builds
preserve the 256-byte system stack guard and reach the same observed
depths of 3,828 system and 296 IRQ bytes.

| Game frames | Physical mean cycles | Physical p95 cycles | Physical peak cycles | Mean actor and effect draw cycles |
| --- | ---: | ---: | ---: | ---: |
| 120–1,119 | 565,035 → 562,089 | 639,828 → 627,595 | 895,885 → 849,625 | 141,721 → 92,573 |
| 4,000–4,999 | 643,995 → 596,272 | 738,504 → 690,070 | 1,135,048 → 1,088,325 | 144,033 → 96,180 |

Mean physical cycles fall by 0.52% and 7.41% across the two windows.
Their peaks fall by 5.16% and 4.12%. These are emulator cycle measurements.
The earlier whole-update mean improves less than the rendering cost.
The profiled updates at game frames 120 and 180 individually take more
physical cycles even though both draw the Nexus faster.

| Game frame | Indexed Nexus draw | Packed Nexus draw |
| --- | ---: | ---: |
| 120 | 83,645 | 36,932 |
| 180 | 90,162 | 36,932 |
| 240 | 89,954 | 36,933 |

The three cuts use hardware breakpoints immediately around the draw
calls. The control observer checks the Nexus pixel pointer and live
return addresses on the guest stack. The candidate observer checks
the dedicated branch loading the Nexus word frame.

| Retained private receipt | SHA-256 |
| --- | --- |
| Candidate link | `dd8b399488f9c351d00b71e4140da7a8bb09de74837e81da1e9351d89dcf4153` |
| Ordinary Blood Bath comparison | `251d282b228a2b657b981b6385788a261825e0f39805dae78809e4b9138c82a3` |
| Nexus call attribution | `ade9423e0d903323ed305ba7c170295cb4cee7b06b9e84c532e5a07f4093e55d` |
