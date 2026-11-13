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
prepared checkout to build the Z10 probe.

## Normal cartridge qualification

The physical mGBA control is the pinned icon build from `cbba1c5`
(Z10 SHA-256 `e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907`).
The candidate applies `1e76370` to its source and enables only
`SC64_STATIC_MINERAL_WORDS` (Z10 SHA-256
`5cb62ac50f45649abb2a43ee0e4f380975a3ee1106ae0432879510b3d3e66972`).
The prior product compile vectors, prepared assets and RAM guards were
reused. Terran, Z10, P10 and Showcase each link and verify as 32 MiB
ROMs, with unchanged `.data`, BSS and IWRAM end addresses. Linked text
and read-only data grow by 23,640–23,744 bytes across the four builds.
The source boundary, doctor and all three focused host targets above
also pass.

The Z7 tape starts at HOME and matches 39 named replay checkpoints.
All 67 ordered samples match in both 38,400-byte indexed pages, palette,
fog, exploration, menu state and raw world fingerprints. The 63 samples
with the command menu closed average 1,318,076 → 1,292,833 whole-loop
guest cycles (−1.92%); 13 rise, by at most 29,554 cycles.

An instruction profile on 12 of those physical calls also matches the
39 replay checkpoints. Excluding the menu call, the static resource pass
averages 124,933 → 101,426 inclusive cycles (−18.82%), and the whole
profiled update averages 1,194,978 → 1,173,756 cycles (−1.78%). At call
15,356, the static pass falls from 233,160 to 178,427 cycles and the
whole update falls by 53,798. At menu call 15,623, the loop falls by
210,911 cycles, but the static resource pass is absent and background
cost falls by only eight cycles. The menu outlier is not evidence of
mineral drawing savings.

Both full three-CPU custom tapes match the original outcomes and all
76 Outpost and 77 Blood Bath ordered checkpoints, including both pages
and the palette at every checkpoint. Their same 1,000-update window
(calls 10,860–11,859) gives these whole-loop results:

| Map | Mean cycles | p95 cycles | Maximum cycles |
| --- | ---: | ---: | ---: |
| Outpost | 1,320,506 → 1,319,811 | 2,142,033 → 2,132,879 | 3,816,683 → 3,819,134 |
| Blood Bath | 1,186,105 → 1,185,589 | 1,534,488 → 1,529,560 | 2,278,463 → 2,321,315 |

Blood Bath's candidate maximum occurs at menu opening, call 11,847.
The same call rises from 2,080,790 to 2,321,315 cycles (+11.56%), and a
separate physical replay reproduces both values. Background and map cost
rise by only 7 and 20 cycles respectively, HUD by 1,120 and update by 22.
Those scopes do not explain the 240,525-cycle increase. The previous
maximum, call 11,092, is nearly unchanged (2,278,463 → 2,278,136).
These are bounded guest measurements; keep the probe optional while
investigating the timing tail and qualifying on hardware.

Paired replay receipt SHA-256 values (control, then candidate):

- Z7: `bfa290e04683e65b2203a2ef8aa427b5a5875e01c19498e228e1958a65ed449f`, `f750fadb46471d001a1798c6306af7f79d7684109b945101c93c20974ce1b70f`.
- Outpost: `61ceeddf9cd439ccca18ee1f45b25ed0c1fd5336e284dc27f2f5777c58c43051`, `4264b563bd6f290fdc085f09f72e8ab8ba33ad13359d2fff08a7b1d549b3c4c7`.
- Blood Bath: `7d226c1d93b82cd36fcab3651a272b5e6ef4907c83e53dbea0cd6c758e15e69e`, `f353993b1ad8f99dba14f94862125e26523288c659e33ffa8741039e6e6eafc1`.
