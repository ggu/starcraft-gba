# Nearest depot selection on normal P10

The normal `c906058` P10 cartridge and the same source with only
`SC64_DEPOT_FIRST_NEAREST_PROBE` both win the ordinary HOME tape at
frame 46,057 after 47,194 calls. Facility FIFO, direct census, typed
reader and word packing are off on both. The candidate uses the exact
staged P10 compiler command with the probe define inserted immediately
after the compiler executable. Both verify as 32 MiB. IWRAM ends at
`0x03004aa0` and BSS at `0x0203c0e0` on both; text grows 208 bytes.

All 20 gameplay checkpoints, ten briefing pages, 14 authored dialogue
acknowledgments and terminal campaign fields match. Thirteen ordered
observations also match both rendered pages, palette, fog, cursor,
units, buildings and minerals. Only derived route cache metadata,
active fields, and build/hit counts differ in those observations.
Nearest-first removes route requests for farther depots that cannot win
the selection; it preserves the ascending slot tie rule.

| Call | Whole update, control → nearest first | Facility lookup, control → nearest first |
| --- | ---: | ---: |
| 9,674 | 42,479,785 → 2,634,827 | 30,139,497 → 765,167 |
| 11,045 | 45,264,642 → 2,284,400 | 37,000,313 → 646,741 |
| 11,046 | 1,691,495 → 1,701,673 | 3,332 → 3,422 |
| 32,644 | 43,182,881 → 8,495,611 | 30,375,391 → 1,088,815 |

Explicit display wait takes 25 cycles on both in all four samples.
Full mean drops 2.05%, from 1,905,975 to 1,866,904 guest cycles
(8.80 to 8.99 emulated updates per second). The result update still
dominates the full maximum, 76.49M versus 76.54M cycles. These measurements
do not establish hardware FPS.

The same pair matches all 101 ordered world and video observations on
an ordinary Outpost custom tape through call 18,771. In 58 samples one
owner has multiple completed resource depots (up to four). In 44
samples a worker is carrying resources with a mine or gas order and
a valid home among that owner's multiple depots. This tape ends in
gameplay on these P10 products. Its prefix mean is 824,667 → 823,334
cycles and its maximum 19,947,980 → 19,948,078.

Both cartridges load the same retained normal pause journal through
the physical Load button, at frame 36,761 with 213 units and 81
buildings. The starting world matches the earlier normal load. All
five ordered observations match raw world, both rendered pages, palette,
and fog through the same victory at frame 46,057. The exported journals
are byte-identical. This is a fresh process for each cartridge and the
same physical suffix, with no changes to saved input.

- Control ROM: `b24184ee2b10b8c6f64153672a25d79a39ef6dc098b8c4cc7582ec8016538474`.
- Candidate ROM: `64a2bb9e43840c737b30934831f689fa948c43e640198f0473dadce7c9bfd525`.
- Control / candidate campaign: `e88bf80bdbad31ba3bd35a52424e35498085277160dca46a458640b96f32794d` /
  `e54098873a31777e8a3d8b905f88b1017ab341c539c57127aff017ed3776e696`.
- Ordered campaign difference receipt: `a30a3dd7b34f8dfccd348327a4265e327a8840e2ef760e7606f27c362ba64886`.
- Outpost tape: `aa5c552c2c81153497c384ac1632a3316051989b380d30f30779ab519b733431`.
- Control / candidate Outpost: `e70f0cc7ed1d05e5189cd69cddeb76c29ffed55004a0db53eda2798e997634e4` /
  `cdd72e65bce2197df2a886db2788acb2f6ec009591a3bc13e963319fe2ebb97e`.
- Retained input journal: `db0a7afa244e97912d0e06fdf3b80600b02750e03ab34b4b9511af378b7f8569`.
- Paired fresh-load receipt: `43fdef64b82568beac497da7bae9f4477eae4fe6b831656234809d602ed0377b`.
- Both exported journals: `c69ff8edddd990e94f3f9418e5b2198cb47e649a555afb36aca5d6f54de58cbf`.
