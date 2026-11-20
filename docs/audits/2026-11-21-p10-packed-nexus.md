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

The original P10 control on `bf846b88` and the packed candidate both
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

The three cuts use debugger breakpoints immediately around the draw
calls. The control observer checks the Nexus pixel pointer and live
return addresses on the guest stack. The candidate observer checks
the dedicated branch loading the Nexus word frame.

## Default build

The default recipe on `bf846b88` produces a ROM byte-for-byte equal to
the measured candidate. The ROM SHA-256 is
`4c834e2ae4880f640b53a2570e8befa968ad697ea724b3f8664c87d0194487ee`.
Both ARM links verify the same 40 inputs, cartridge header and ELF
payload. The flag-off recipe selects the exact Hatchery-and-Overmind
header from the control. All eight switch combinations resolve to their
corresponding bank paths.

| Retained private receipt | SHA-256 |
| --- | --- |
| Candidate link | `dd8b399488f9c351d00b71e4140da7a8bb09de74837e81da1e9351d89dcf4153` |
| Default link | `15bcf5ce84138db66ba368b2299e3f2fc054fb95c872633c2ae1d3442dbf12cb` |
| Ordinary Blood Bath comparison | `251d282b228a2b657b981b6385788a261825e0f39805dae78809e4b9138c82a3` |
| Nexus call attribution | `ade9423e0d903323ed305ba7c170295cb4cee7b06b9e84c532e5a07f4093e55d` |

## Shared world-depth release

The follow-up control at `0034ebd4` includes the shared world-depth
changes. The normal packed Nexus build changes only its selected bank
relative to this control. The cartridge header, ELF payload and all 40
prepared input digests verify. IWRAM and EWRAM endpoints are unchanged.

Both ROMs replay the same three-AI Blood Bath lobby through frame 4,999.
All 21 world, palette and both-page pixel checkpoints match. Camera and
route counters match at all 2,000 timed updates. The 256-byte stack guard
stays intact and both builds reach 3,844 system and 296 IRQ bytes.

| Game frames | Physical mean cycles | Physical p95 cycles | Physical peak cycles | Mean actor and effect draw cycles |
| --- | ---: | ---: | ---: | ---: |
| 120–1,119 | 563,290 → 562,089 | 638,390 → 626,174 | 890,854 → 844,296 | 135,830 → 87,246 |
| 4,000–4,999 | 638,762 → 591,273 | 731,171 → 681,424 | 1,130,424 → 1,083,814 | 138,878 → 91,072 |

The late window mean falls by 7.43% and the peak by 4.12%. The early
window mean falls by 0.21%. These are emulated cycles from the release
cartridges.

| Retained private receipt or ROM | SHA-256 |
| --- | --- |
| Control ROM | `7ee5bf4f3f34eff3827cd326c748fd0c6e2869e837fd466bd774b245fcd69532` |
| Normal packed Nexus ROM | `c8d88776f1ac2356b8a69c51e584ec38c79fd44cedc65d96e375db0075df6df9` |
| Normal packed Nexus link | `63ba62f05baf28dddc97a70be77e0d5a3ed1ffad9cc7f007c73feff3fd402150` |
| Ordinary Blood Bath comparison | `5e6c7c2fd5ba23f820dfa9b3ba48f44ec82aec4234c45f393bf8aa81f41c1a35` |

### Fresh P10 campaign victory on the merged cartridge

The #645 control and #646 release `1a74f8e6` both load the same
authenticated P10 pause journal from HOME at frame 36,761. The next
9,489 physical calls reach victory at frame 46,057 with the Overmind
destroyed and Zeratul, the Gantrithor and the Hyperion alive. All six
named gameplay checks, 21 world and two-page video cuts, and 198 route
field digest pairs match. Both 32 KiB result journal exports are
byte-identical.

| Retained private receipt | SHA-256 |
| --- | --- |
| Physical continuation comparison | `4481cd8c280255dbdbcd59944c53777a06faced94226d5e76c5c0eafab5d472f` |
| Both result SRAM exports | `25b0b97c72ffb92721aaf597f15719c7d05d9682a499c32b7f10cb304e00ae58` |
