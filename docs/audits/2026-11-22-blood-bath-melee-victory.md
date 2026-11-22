# Blood Bath Melee victory

An opt-in Terran ROM built from `b2d323c4` wins Blood Bath Melee
against one Zerg CPU from a fresh HOME launch with ordinary buttons.
The Terran controller trains and uses Marines, Medics and Firebats
and finishes at frame 38,258, after 38,982 controller calls. The 234
recorded action checks and both start and result checks match on ARM.
The result has 26 Terran units, 16 Terran structures and no Zerg actors.
The average is 740,171 emulated cycles per call, or 22.67 emulated
main-loop calls per second.

The first 28 calls open Blood Bath and select Melee through the lobby.
The launch resolves to packed base order `11`, with GBA base IDs 3
and 2 for the Terran and Zerg respectively. The HOST author reproduces
that order with `SC64_SKIRMISH_START_ORDER=11` and
`SC64_SKIRMISH_GAME_TYPE=melee`. The author starts from a measured ARM
launch and sets only the HOST clock used by the existing lobby seed
function. The ARM replay uses physical button inputs and reads the
resulting mode and order from the ROM. Supply those values in the
`start` event together. The verifier requires the same match type and
order at launch and result. Older records still use the existing
launch fields.

Start then B commits a pause save on physical call 8,157, frame 7,910.
The uninterrupted and fresh HOME paths match 218 continuation cuts
through victory. A third fresh HOME process opens the victory and
re-exports identical SRAM at frame 38,258.

## Native three-CPU alliances

The [matched native starts](2026-11-12-n64-blood-bath-opening.md)
use the same start timing, seed, races and placements in USA
Blood Bath. The six directed CPU-to-CPU relations read 2 in Melee
and 0 in Free for All. Native enemy scans skip allied relations.

A fresh isolated ares continuation starts the native Blood Bath
Melee lobby with human Terran and Zerg, Terran and Protoss CPUs.
Read-only captures at steps 313, 1,020, 2,199 and 3,454 have identical
relations among the four active owners. The human remains hostile
to each CPU and the CPUs remain mutually allied. All four owners
still have actors after the last sample. The lobby screenshot verifies
the selected mode. The match states establish alliance persistence
through the opening and leave post-elimination persistence open.

## Remaining release checks

The normal race and Showcase releases still use their existing
Custom Game mode. The Melee flag remains opt-in while survivor transfer
and active multi-CPU wins qualify. The existing native ally relation
and result rules are the comparison target for those runs.

All four Melee-enabled ARM links pass ROM and static RAM checks.
All 54 existing Free for All launch and fresh-load cases pass on
these builds, covering both maps, every human race and one to three
CPUs. Each of the four profiles also repeats the same three-CPU
Melee defeat through uninterrupted play and fresh HOME.
The P10 campaign tail still reaches victory with the same 68 gameplay,
video and fog cuts and the same victory SRAM. The compare includes
the per-call frame and route counters and compares the common state
fields across the two profiles. The optional Melee state word is
sampled separately in skirmish. P10 mean cycles per controller call
change from 1,536,740 to 1,536,018.

A private composition of the Melee preview and the
[neutral survivor transfer draft](https://github.com/ggu/starcraft-gba/pull/566)
also passes all four ARM links. The Z10 IWRAM endpoint is
`0x03004ff0`, 16 bytes below the current guard.
An ordinary three-CPU Melee launch reaches human defeat at frame
11,580 with two surviving SCVs, both transferred to neutral.
A separate staged lethal hit clears the final CPU building on
frame 3 and transfers a worker at 20 HP. A fresh Home load of the
pause continues through victory on frame 4 and a further fresh
process reads the terminal result.

The dual-flag active Terran tape matches all actions through the
last CPU building's destruction at frame 38,258. One more neutral
update reaches victory at frame 38,259, with the same armies and
resources at result. The original tape and extended proof both
capture the ordinary physical lobby and battle inputs. An ordinary
win with a surviving enemy worker remains a separate case.

| Local receipt | SHA-256 |
| --- | --- |
| Native Melee lobby screenshot | `5af262d6f37f2a9fe04b874d7a5d3aa95a2481fcbe632fc7c9cfbc19eb03efba` |
| Native Melee step 313 | `a6e1aac50850f49beee157a84ba236feb83219ed262386cce1653a143503a34e` |
| Native Melee step 1,020 | `918a1a4bd0b9bf4e5fcfb553aed5f55355a7c8af16d007055bccc7577a4386a1` |
| Native Melee step 2,199 | `b9d1bfdedcce9cf0b77a744ac2d3edc763c602f20f798a210ca29cd55a68394e` |
| Native Melee step 3,454 | `6fe2e4b8005b917bb3ab500ba82f26b0116268323684602143e7112cdf0b6ddc` |
| Terran Melee launch | `9a72dd67bea2d6c50b95983f0599f578e2295bf563f64d6bca66fba45adfaf3e` |
| Terran Melee controller tape | `2a31bff6ce8dd8a663f5cf7804984bf3ea841f2dceed48f2dbe9bc93e9a40cca` |
| Terran Melee ARM victory | `94dba6fccaa691693927b538b20e4eda020a5935b31a76eec9610013ae40505b` |
| Terran pause, continuation and fresh victory | `b86d177079c305e4135a6ffee86c14ea60ca04caa6951db2d4bbc90d136aafec` |
| Existing Custom Game save matrix | `d35daeff613e4f9e905811b7097bac419d9bf24e68fca5b74488e3ef3056c9dc` |
| Three-CPU Melee result and continuation index | `3092602df7b5b219c0ca1dfc5db2f8bd859bbda29334a7500771e85c18e8de88` |
| P10 campaign tail | `aada20ce563e9985f134f2a5d5be93bfb414dbf352c7aaab5c1c173cff83e4bc` |
| Dual-flag ordinary defeat and survivor transfer | `4cd6524a6c9e6d4ba8794cfd74f5549ce7050fc54b00edab442273acbb28ab62` |
| Dual-flag staged enemy survivor and fresh victory | `5004d70f682a115d3e7b227cce7c19a9104d466031c095ae166a25481f8de6f6` |
| Dual-flag ordinary Melee victory | `90b7b8fb28e8a873567581cb60486b3cc6b6c238bb6bb2c9a1d4a0f37d768a61` |
