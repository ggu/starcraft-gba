# Visible drawing and campaign replays on 18e550d7

## Visible Carrier fight and Protoss army

Eight paired updates of Showcase's Carrier scene, frames 41–48,
match world state, both framebuffer pages, palette and guest clocks
with instruction profiling on and off. The frame costs range from
1,132,393 to 1,253,604 cycles. The generic byte sprite blitter takes
1,706,299 self cycles in 731 calls, or 18.1% of total guest cycles.
The packed sprite blitter takes 380,475 self cycles in 89 calls.

Read-only breakpoints count 47,903 scalar pixel pairs over 4,725
clipped rows. The original aligned-word path could cover 10,014
pairs because it requires matching source and destination phases.
The longest row contains 21 pairs. Unit-image drawing submits
38,960 pairs. Weapon effects submit 8,943 pairs, including 557
one-row calls. A one-row submission can come from a reversed image
or a fog or map fragment.

An ordinary P7 continuation from the retained frame-8,845 pause
reaches frame 38,745 and creates a new normal pause save there.
An ordinary Down hold moves the camera to a visible army. Frames
38,898–38,905 match the unprofiled replay at the same state, pages,
palette and cycles. The mean is 2,434,790 cycles. Unit-image drawing
takes 704,368 inclusive cycles per update. The generic and packed
blitters take 191,039 and 68,267 self cycles per update. The
selected-unit ellipse renderer takes 143,802 self cycles per update.

The Arbiter tick contributes about 182,242 self cycles per update
in this visible sample. Of the 1,457,935 self cycles across eight
updates, roughly 799k fall in tracked-actor detection and 532k in
aura traversal and geometry. The detector branch includes about
148k for repeated source binding and mask computation. Preserve
refresh scheduling and source-owner fallback when testing reuse
of those calculations.

The first direct-mapped memoization trial reduces Arbiter self
cost by 24,001 cycles across the eight visible-army updates.
Total loop cost rises by 4,203 cycles there and by 4,371 in the
fixed-camera view. Both pages, palette and sampled gameplay match.
An isolated sanitizer check compares 11,760 cache decisions using
a binding stub. The prepared game fixture needs its generated
header. Any later trial should measure the net loop cost and the
Arbiter tick's inclusive cost.

| Local receipt | SHA-256 |
| --- | --- |
| Showcase profiled frames | `8022f8b29ee3f032ade42c0c0dc452f600ebe24aa97cc13d9d74b74ceb9576fd` |
| Showcase clipped-row census | `3529cd4addd563dd3e98c5d56e9388f8f7994a6e370c3c0faa9fe528fcd0abe5` |
| P7 visible-army profile | `0a23fdce57ee3376355199bc2cd79f846dc26bce2379df5d22b8da67b0df9d2a` |
| P7 normal pause journal at frame 38,745 | `616a5ebe268c0135776585d38c2a5f5d1a01bf138e57581213913be0c92ee743` |
| P7 visible-army owner-mask memoization | `a5213fc5aa51991b0a5de9c418a32e52a331f4d753f75c7390a8d049367a7a03` |

## Byte sprite row trials

Enabling the existing aligned-word path in Showcase preserves the
world and framebuffer pages over 120 updates in each of three
scenes. Mean frame costs rise by 0.32–0.60%. A compact C row loop
adds a two-halfword load for source rows offset by two bytes. This
version improves the three Showcase scenes by 0.68–2.43%, the fixed
P10 Outpost pair by 0.82% and the visible P7 army by 1.06%. Its Z10
link exceeds the IWRAM guard by 88 bytes.

An ARM quad loop passes the guard when the blitter is compiled for
size. Z10 ends at `0x03004fa8`, 56 bytes above the release. The
paired worlds, both framebuffer pages, palette and sampled SRAM
match. Isolated display-page timing differences occur.

| Workload | Mean cycle reduction with the size-compiled ARM quad loop |
| --- | ---: |
| Showcase Carrier scene | 0.93% |
| Showcase scene 5 | 0.12% |
| Showcase scene 7 | −0.28% |
| P10 fixed Outpost | 1.17% |
| P10 Blood Bath | 0.14% |
| P7 visible army | 0.60% |
| Z9 road assault | −0.02% |
| Z9 opening technology | 0.16% |
| Z10 panned Outpost | 0.007% |

Z10 panned Outpost's maximum rises from 1,495,093 to 1,520,273
cycles in the 96-update window. Visible-army peak cost falls from
2,594,227 to 2,570,316.

Moving the remaining two-pixel loop into ARM keeps all four normal
links inside their memory guards at the usual optimization level.
Z10 ends at `0x03004fd0`, 96 bytes above release and 48 bytes below
the code boundary. The candidate applies this path to all three
race builds and Showcase. It loads one aligned word or two aligned
halfwords per quad and preserves transparent bytes in their original
positions. Remaining pairs and clipped edges use halfword writes.
The Z10 blitter prologue allocates 20 more stack bytes. The paired
tests cover the retained workloads and the existing static boundary.

| Workload | Mean cycle reduction with both ARM loops |
| --- | ---: |
| Showcase Carrier scene | 3.95% |
| Showcase scene 5 | 2.79% |
| Showcase scene 7 | 1.91% |
| P10 fixed Outpost | 1.52% |
| P10 Blood Bath | 0.82% |
| Terran fixed Outpost | 1.80% |
| Terran camera sweep | 0.57% |
| P7 visible army | 1.78% |
| Z9 road assault | 0.36% |
| Z9 opening technology | 0.56% |
| Z10 panned Outpost | approximately flat |

The 96-update visible-army mean changes from 2,398,249 to 2,355,460
cycles, and peak cost from 2,594,227 to 2,540,310. World, both pages,
and palette agree across these comparisons. The skirmish pause
journals and fresh continuations agree. A few sampled display-page
register states differ. All tested busy windows exceed the
561,792-cycle budget.

The exact candidate repeats Wasteland and Choosing Sides from
HOME through a fresh pause load and victory readback. Both pause
and victory journals match the release. Zerg 2 repeats its full
alternate HOME route, winning at frame 10,209 with identical
terminal and re-exported SRAM. The candidate also passes all 54
Custom Game opening, pause and fresh-load cases.

| Local receipt | SHA-256 |
| --- | --- |
| Showcase Carrier control / size-compiled ARM quad | `77fc98432315ef092f6fe1fbc3068a9860167013027e3839ebc0f693be277e22` / `957f640fd8d0e0a580d7b92e32dc66a8a67331ee99ac0d26b04718f76ebdb2cb` |
| P10 fixed Outpost size-compiled ARM quad | `0d72120cb8c6f5302e907ff8cf364076184fb63b8a1d6e21fcf7240888d9964f` |
| Z10 panned Outpost size-compiled ARM quad | `354cf2fd5cc2e058931239f5eee825b833c9b81b3f557355c8342eef81bbd205` |
| P7 visible army control / size-compiled ARM quad | `820b844769143c361efcee23f9b0fe2559285eb495f38e2d6c808bdbf3996e89` / `d0fd2800785c2011cf7ed38937e10053cb9a9106241f1f6199bb11da114a5abc` |
| ARM Mode 4 differential, 1,024 cases on each path | `30f47cdd0c997453b215563f3a4d3eb33db2ed2619c1e79c50cccc80c54fdaca` |
| Candidate 54-case Custom Game matrix | `da0f9b1065be96a29cba4a9126d37cf719c4449ab0a1bebd8d845446f5511dfe` |
| Terran fixed and panned Outpost pairs | `ba0c628a8b179cad3af9f5abe573a66ca63f7871003f5e1e8a06aec976488801` |
| ARM quad and pair Showcase Carrier and P7 | `33ac68765678ffcbcd5bcf4651f6081cf1d87db05a149407d5c843ec32e182c3` / `2c405d68357a1d2a7eba64c933387359c244eddc0264e554a13b9b7ca27271d3` |
| Candidate Terran 1, Protoss 5 and Zerg 2 campaign receipts | `47792336a40107244b5b0c2c4379bd0f00c14a0acb6e02196a120b86bb438d82` / `3320b5ad15a4b72f357c80cb84c53130bd8a9f23948c1d7c3e105b750e4d4848` / `51a49fd1945480ec071b907c31fe69dd67b9b3b8797507232dc4e457966f033f` |

## Exact campaign checks

The [previous eight complete checks](2026-11-25-fogged-tracer-viewport.md)
and the following routes use the ROMs from the same `18e550d7` source
tree. The Terran ROM SHA-256 is
`2b1faf0b1871c1b1bd058a2370b11191a3de26fe6daa05b2e11ce30ae5544a9f`.
The following recorded input replays match their checkpoints and pass
both the pause reload and fresh victory readback.

| Chapter | Pause frame | Victory frame | Local receipt SHA-256 |
| --- | ---: | ---: | --- |
| T6 | 19,466 | 23,536 | `35fcbe61d2a284fc6c94a4beee715a0c39dc8b6df4ddf21e7ed7617eb1db1116` |
| T7 | 2,289 | 3,931 | `d55bfc0fb6b48ffbb0d7a116bc6a53c3df77682c9b3cc35e811008057e70f67d` |
| T8 | 2,568 | 139,216 | `16f94ef6707351f8218b7685ed9a24cc0e6077cf019f98601839f2c32d0f01bb` |
| T9 | 35,363 | 47,266 | `ac1890b9f26815998c0b0eb52b0950cae1570e1197a47293b6639e97ead3568d` |
| T10 | 2,276 | 3,361 | `5ebdc2faeb87c007396281e70211af584a71fde6572e5addf128d5306fefa0d3` |

The archived T5 route opens pause with Select at tick 20,401 when
the selection is empty. R+Down+Right triggers the pause-menu
restart shortcut at tick 20,403 and resets frame 20,073 to zero.
A current ordinary-input route must carry the mission through
victory. Reset receipt SHA-256
`4375e63fb25ced0f086f200ce43c3f8d33f751d0a2203681d51172cb51b481c6`.

The P7 suffix from frame 8,845 matches the old world-state fingerprint
through sampled call 40,000, with framebuffer differences in earlier
cuts. By call 50,000 the world fingerprints differ. At call 56,967,
the current game is still active at frame 55,258 with all three heroes
alive and the source-owner 0 Nexus at full HP. The original tape expects
a stopped frame 55,257. A current route through the remaining
objectives and a fresh result readback remain open. Diagnostic
receipt SHA-256
`d137b7183e3b8e254ef0b99f7f23cc18375333ed3634c7fdd6df775ef4f412f4`.
