# Packed static mineral drawing

In eleven sampled non-menu updates of the normal Z10 cartridge playing
Z7, the static art pass averaged 124,926 guest cycles. Its own scan
averaged only 7,774 cycles. At call 15,356, the call-edge trace attributes
132,261 cycles to eleven generic byte blits directly from the static
resource pass, separate from the 90,063-cycle building art pass. It does
not distinguish minerals from geysers on that edge.

This probe derives the three existing mineral frames in the format
already used by the Mode-4 packed sprite compositor. The phases keep
word alignment at any camera position; the stored masks preserve
transparency. Only the static mineral call changes, using the original
canvas placement, visibility, order and clip. Geysers, other art,
procedural fallbacks and the optional tile backend keep their paths.

Each 32×28 frame requires 7,840 bytes of packed phase data. The three
frames add 23,520 bytes of data plus small frame records; no persistent
working memory or new IWRAM function is needed. The generated header is
about 74 KiB of text and remains local. Tightening the packed row spans
would skip only about 11–22% of groups on these frames, so this trial
uses the existing compositor rather than a new row format.

| Tiny ARM workload, 12 drawn sprites | Byte path | Packed path |
| --- | ---: | ---: |
| Authored synthetic mineral shapes | 153,777 | 82,905 |
| Three prepared mineral frames | 147,559 | 80,821 |

Each pair matches all 38,400 indexed page bytes on five updates and
uses the normal Mode-4 clipping and compositing functions. The prepared
sprite batch is 45.2% faster in guest cycles. This does not measure
whole-game savings or hardware fps.

`make test-sc64-static-mineral-words test-gfx-sprite-packed` exercises
the synthetic bank and the existing compositor with and without scene
clipping. With prepared local inputs,
`make test-sc64-static-mineral-words-prepared` verifies 400 full pages
through the resource wrapper, including all phases, clipped edges and
an unpacked fallback.

The four normal cartridge targets share the opt-in. It remains off by
default. Use `make SC64_STATIC_MINERAL_WORDS=1 verify-zerg-z10` on a
prepared checkout to build the Z10 probe. Next, compare complete pages
and timings on the Z7 movement window and both custom maps, then check
the other cartridge links and hardware before promoting it.
