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
Eight P7 visible-army frames contain 178 byte blitter calls on each
ROM. The lowest sampled entry stack pointer is `0x03006b20` on both.
After the prologue the observed minimum is `0x03006afc` on the
release and `0x03006ae8` on the candidate. These leave 7,252 and
7,136 bytes, respectively, above each ROM's IWRAM code end at that
call site. Other call paths and the whole-program minimum remain
unmeasured.

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
| Sampled P7 byte blitter stack pointers | `83c599adea0696f391cb854ed9f740b253dd10d65199828683ca4d48df40e151` |
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
An ordinary-input trial skips five empty-selection Select presses
and continues without restarting. At tick 22,000 it has 27 friendly
units and 200 minerals, compared with 35 and 512 in the recorded
route. Unit slot 3 is selected at tick 20,200 in the unmodified run
and the selection is empty by 20,300. Capture a clean resume point
at or before the last matched unit order at tick 19,907 and
re-establish group selection there. Reset receipt SHA-256
`4375e63fb25ced0f086f200ce43c3f8d33f751d0a2203681d51172cb51b481c6`.
A new pause save at tick 19,731, frame 19,419, restores the world,
selected unit, camera and both living heroes through fresh HOME.
Checkpoint receipt SHA-256
`ab9ce693c304bbba8c69b9f4698df82ed0ad138dbd758e99c2ee88b054006cce`.
Ordinary cursor orders return both SCVs to mineral gathering.
Three paid workers complete, and a fresh HOME load at frame 24,067
restores five SCVs, 954 minerals and 300 gas with both heroes alive.
The fresh-load receipt SHA-256 is
`f838b7a1a2194ff33c6263174e033af3d7939550f7fe7fb20121ab81ac337830`.
The existing Starport adds a Control Tower for 50 minerals and
50 gas, then completes two Wraith orders at 150 minerals and
100 gas each. Fresh HOME restores three Wraiths, both heroes,
and the mining economy at frame 28,666. Receipt SHA-256
`91556e279e65127da4469c6a811711cae80347b1f66531516c882f9c0c84fb45`.
Three workers gather gas and seven Wraiths are available in a
fresh-load checkpoint at frame 40,567, with both heroes alive.
Receipt SHA-256
`bde27d5b3b6ba27ec8f3f38e91246df30a4852069f308d884ce7e8dc18030aa3`.

The P7 suffix from frame 8,845 matches the old world-state fingerprint
through sampled call 40,000, with framebuffer differences in earlier
cuts. By call 50,000 the world fingerprints differ. At call 56,967,
the current game is still active at frame 55,258 with all three heroes
alive and the source-owner 0 Nexus at full HP. The original tape expects
a stopped frame 55,257. A current route through the remaining
objectives and a fresh result readback remain open. Diagnostic
receipt SHA-256
`d137b7183e3b8e254ef0b99f7f23cc18375333ed3634c7fdd6df775ef4f412f4`.
A new ordinary pause at call 56,961, frame 55,252, restores the
units, buildings and mission state through fresh HOME with a
byte-identical SRAM re-export. Checkpoint receipt SHA-256
`e7437658a274cbb544437567381ad22794f6c51fbd2c594b3de1693baef13b28`.
Ordinary Gateway production rebuilds the army. A 45-Dragoon
assault fails against the enemy defenses. The next attempt starts
from a fresh ordinary save with 64 Dragoons, destroys the enemy
Nexus at frame 80,193, and reaches victory at frame 80,828 after
advancing the dialogue. All three required heroes survive and
the mission fault flag stays clear. A fresh HOME result load
preserves the named mission state through eight neutral updates
and re-exports byte-identical SRAM. Victory receipt SHA-256
`600890be9e06d27a14d1f2c9f020ffa27c356923157390204f36ab6aab3352bf`;
winning SRAM SHA-256
`ad66dc354fd3db879d9d7cce788ee01e96e3dbbbeda2ae6ed6f60ae66f8cfa78`.
This qualifies the recovered suffix from the ordinary frame-8,845
seed through victory on the release.
The candidate separately replays the 64-Dragoon assault from
the same pre-assault normal save. Its recorded inputs and observations,
terminal journal, and fresh HOME re-export match byte for byte.

## Backwater Marine affordability

In a fresh native N64 replay, Backwater's Barracks has one active
Marine, three waiting and a free slot after its first completion.
The HUD shows 7/26 supply and 22 minerals. An ordinary A press
leaves the queue intact and displays `NOT ENOUGH MINERALS...`.
Mineral income raises the bank to 30 during the sample. The
before/after frame hashes are
`44b88fcb26a0fd09142943d581b9a2c192ebea98379376577343c6911cd05b38`
and `2fc2f5eac7b4727e59be7b1a3f9eae34a63511b7aea8654fb7a148351fd4e3db`.

The candidate Terran GBA ROM starts the comparison at 200 minerals.
Four paid ordinary A presses reduce the balance by 50 each. A fifth
press at frame 88 keeps all four orders and the same active queue
head with 6/26 supply, displays `NEED MORE MINERALS`, and the active
Marine continues training. The same run on `18e550d7` returns
identical queue and framebuffer results. This pairs the affordability branch with
a free queue slot and supply in both games. Local-cap and global-cap
admission remain separate checks. The GBA candidate and release
receipt SHA-256 values are
`f31757950cd78dd71de3fc9862e77724a48cf23bfb3674a940eeb56cd39d1b22`
and `6ceed541ff39308b089c4e286dfdf66c3314123229f5ae3486560f3ae6cac794`.

## Native waiting order at the local supply limit

An ordinary Wasteland continuation starts with nine Marines.
At native step 12,830 it shows 17/18 supply, 139 minerals and an
empty Command Center queue. A fourth SCV order fills the active
slot. At step 12,940 it shows 18/18 supply and 97 minerals. Another
ordinary A press adds a waiting SCV, charges 50 minerals and
leaves the display at 18/18.
Input tape SHA-256
`1de47b5e4a163ee86686864d145bce50322768ab28a2de6c4be6496140eab205`.
This observes payment and waiting admission. Check waiting-order
activation at the local limit separately from the global 200 limit.

## Drawing matching one-pixel selection edges

The P7 profile attributes 143,802 cycles per update to the
row rasterizer and 25,277 to ellipse setup and other selection
work. A trial that looks up common ellipse widths saves just
0.15% of P7 frame cycles. Keep the existing width calculation.
When both spans on an outline row are one pixel wide and on
screen, they occupy the same byte lane in their two halfwords.
Resolve the lane mask once and preserve the other byte in
both writes. The clipped and wider rows use the existing spans.

Against the exact row-blitter release, the ARM candidate saves
0.91% mean cycles over 96 P7 army updates and 0.96% over 96 Z9
road-assault updates. Three 120-update Showcase scenes with
ordinary selection improve 0.33%, 0.51%, and 0.48%. The other
Z9 sample and panned Z10, both P10 Custom Game frames, and the
fixed and panned Terran samples match the prior timings.
The paired worlds, both framebuffer pages and palettes match.
One P7 sample shows a different display-page selection. The
tested ordinary Custom Game pause journals and fresh
continuations match. All four production links retain identical
IWRAM/EWRAM endpoints; the host field UI and tile/bitmap shape
oracles pass. Dense scenes still exceed the two-VBlank budget.

| Local receipt | SHA-256 |
| --- | --- |
| P7 visible army | `d8d3e4a3f7a744d9e9bd5dead44d78c279f7f1677d11ebc8e41c440ecd80456b` |
| Z9 road assault | `a2d01f9b9b83e9ebd3ce651e16d58ea8fa130a56093403b6685070e6e036b63b` |
| Showcase scenes 0, 5 and 7 | `af2207672ca2ed2fa57e16c32f61c36b7f033b503f0530debafaa8f7e7d54021` / `3c550689a971a284c8e74bc2b8a3fcce8ad1c81da8916171b7ebd243ce9f7de8` / `1448676dc4179d99b1c8c71c9475775afee3226068b4b67a8cfe1d132e05773a` |

## Paired rows in visible selection outlines

With the full ellipse inside the active clip, the center half
width bounds all rows. Read the inner and outer widths once for
each pair of rows, preserve the edge lanes, and keep the flat
top and bottom spans. The clipped path remains active at the
screen and HUD edges.

Against the thin-edge release, combined frame cycles fall
1.55% in the P7 visible army and 1.68% in the Z9 road assault
over 96 updates each. Three Showcase scenes with ordinary
selection save 0.63–0.86% over 120 updates each. The worlds,
both framebuffer pages and palettes match. One Showcase sample
selects the other display page. The tested Z9 save and fresh
continuation match. The other Z9 sample and panned Z10 Custom
Game are flat. All four ARM builds keep their memory endpoints.
The sampled busy scenes remain over the two-VBlank deadline.

The independent host oracle compares 4,836 ellipses against
individual pixel draws, including full rings, clipped corners,
flat caps, both destination phases, zero-color aliasing, and the
tile renderer's dirty, draw and art masks.

| Local receipt | SHA-256 |
| --- | --- |
| P7 visible army | `4c9f8bfbfe6731ddaedc0a806ab4026c4bf103b1b47a4c7b680a14bfcb7d58a1` |
| Z9 road assault | `fb5c457ebe4db01e8d620d50abf3e6c075056b0577e7ec5cc8f35a0698fd298c` |
| Showcase scenes 0, 5 and 7 | `e1b088b7422890a95255154865f0c0c805ce70efcf816bb02edf7742e443a3cf` / `9b214756c3f406643c11dee947f33708581f7153c00543e54484c79d0ec879a2` / `f4e3ee896231ece5bd72ba84f36215a8f0554b42b2609a379df41edcc6a7a3ee` |

## Combined supply and paired-outline qualification

The control is the released supply build at `2ea65bcd`. Recovered
and retained build inputs reproduce all four control ROMs byte for
byte. All four candidate links retain their IWRAM and EWRAM endpoints
and pass the existing memory guards. The three race candidates pass
144 ordinary Custom Game opening, pause and fresh continuation
cases across both maps, each human race and one to three opponents.
The additional 90 cases cover the four occupied seat patterns absent
from the 54-case matrix, two Protoss opponents, and three Zerg opponents.

| Workload | Updates | Control mean cycles | Candidate mean cycles | Reduction |
| --- | ---: | ---: | ---: | ---: |
| Protoss 7 visible army | 96 | 2,334,416 | 2,298,367 | 1.54% |
| Zerg 9 road assault | 96 | 1,611,413 | 1,587,077 | 1.51% |
| Protoss 10 fixed three-CPU Outpost | 96 | 1,063,343 | 1,063,343 | 0.00% |
| Zerg 10 panned three-CPU Outpost | 96 | 950,723 | 950,723 | 0.00% |
| Showcase scene 0 | 120 | 1,284,174 | 1,276,185 | 0.62% |
| Showcase scene 5 | 120 | 1,476,984 | 1,464,197 | 0.87% |
| Showcase scene 7 | 120 | 1,221,954 | 1,211,712 | 0.84% |

The sampled worlds, both framebuffer pages and palettes match. Two
Z9 and two Showcase samples select the other display page. The Z9
and Custom Game pause exports and fresh continuations match. Every
update in the four race workloads exceeds the 561,792-cycle budget.

On the released supply build, the Terran 5 ordinary-input continuation
from frame 88,779 replenishes the Wraith wing and clears the 11 remaining
enemy buildings. The result at frame 143,281 restores via fresh HOME
with Raynor at 45 HP and Kerrigan at 250 HP. Its victory journal
re-exports byte for byte. Pressing A advances to Terran 6, where an
ordinary pause save and a fresh HOME load agree at frame 101. The
T5 receipt covers the late checkpoint through victory. Replaying
its physical buttons in the 13 recorded fresh-HOME stages on the
paired-outline build produces all 13 identical journals and the
same fresh victory readback. The older October T3 tape first differs
in enemy minerals at input tick 993 on the released supply build.
On the paired-outline build, replay its unchanged ordinary inputs
from HOME, save and load at frame 26,993, and win at frame 54,000.
Mission and objective checkpoints agree, and the victory journal
re-exports byte for byte. Enemy minerals, actor counts and raid
counts differ from the October observations at 53 checkpoints.

On the paired-outline build, the full Z8 HOME replay wins at frame
86,969. The late Z10 route from the retained frame-60,123 pause wins
at frame 63,523. The recovered P7 assault wins at frame 80,828 with
all three required heroes alive. The three fresh HOME loads restore
their terminal state and re-export identical victory SRAM.

| Local receipt | SHA-256 |
| --- | --- |
| Candidate 54-case Custom Game matrix | `bd7203166664b38dfc15c9b66f24f744fec1f37745b5e7e27199fd9461c5ded6` |
| Candidate 90 additional CPU-seat cases | `23ae317befef5cdd0ca2d65104900ac0cf5d9881be840e37643e974991bacb67` |
| P7 paired-outline sample | `b6713d4193aac41cc5c9a40af04576c8768b3310488dc82ef6172c4d1aca0e30` |
| Z9 paired-outline sample | `b5f52fc0fb5cb7e33f1a37f1c971b93210ff604833c2a483034d75b4d7744f55` |
| Showcase paired comparisons | `a589ade01d5af32323944fd9cd6c187dacd9bae68cfb587bdc5c79c7328c0542` |
| T5 continuation input and readback manifest | `e39669f66e90d0cde3e0462e157790e25b2ed64772af3f13ccefe442c78fae69` |
| T5 victory journal | `88888faa6e886d23d31c777584c591c06f0c99a71ef32c43bf446eb489a792b1` |
| Candidate T5 13-stage ordinary replay and fresh readback | `6576785904adf125f8971166fe3214b38caf6f66f38d23dad250b97304d47fb2` |
| T6 pause and fresh HOME journal | `adc89cbe03df68f46c6dd7b33e9c7d4c10690b8f7ad02b479f71ae3ae0b288cc` |
| Candidate T3 HOME replay, pause and victory readback | `8d64b25b17863fa83800e7f5ed0573562e573bf743d73a570a56883728c94ef1` |
| T3 historical battle-stat differences | `7c233fa3de3e4ed65071760e55755cfe1e565232cb8124eeba27eafb6720d412` |
| Z8 victory journal | `f0946a81d56cf77f0f034d735544cc130217051b4531eabf6714bf3d5882f2f9` |
| Z10 victory input receipt | `9e1d9486d993f9276b40946d8d0a7de0488820b136b1d6ba0f3e6c21ea2fc2c0` |
| P7 victory input receipt | `600890be9e06d27a14d1f2c9f020ffa27c356923157390204f36ab6aab3352bf` |

### Visible Terran three-CPU combat on the supply release

A normal Blood Bath save with Terran, Zerg and Protoss opponents
reaches a visible attack at the human base. A worker falls from
36 to 28 HP during the 32 measured updates. The two runs match
full/update/orders/draw cycles, world fingerprint, both video
pages, palette, display and camera. Across frames 11,183–11,214,
mean total cost is 807,142 cycles, including 389,193 in orders and
290,045 in draw. Every update exceeds the 561,792-cycle budget.

Separate read-only entry/return watches for the first eight updates
match the same uninstrumented clocks and outputs. `move_toward`
costs 235,488 inclusive cycles per update across 98.25 calls.
`sc64_target_nearest` costs 9,885 across 42 calls, and
`draw_tracer_parts` costs 12,406 across 6.125 calls. Weapon-range
movement is not reached in this cut. The uncached contact mask
accounts for 76,356 cycles per update across 18.25 calls, and the
monotone route step for 33,646 across 4.125 calls. Costs are inclusive
and the figures from independently watched functions can overlap.

The mover-origin and map-dimension reuse from [#828](https://github.com/ggu/starcraft-gba/pull/828)
keeps all 32 sampled worlds and video outputs. On this cut, the
uncached mask costs 320 more cycles per update and the whole frame
costs 327 more. Leave the source change out of the released builds.

Adding the draft [monotone route witness](https://github.com/ggu/starcraft-gba/pull/805)
to the same release increases linked BSS by 352 bytes. The 32 visible
updates match their sampled world and video outputs. Mean frame
cost falls from 807,142 to 788,972 cycles (2.25%). The largest
individual cost increase is 30,062 cycles. All 32 still exceed the
budget. A read-only replay counts 63 witness hits in 124 monotone
queries. Keep this result scoped to the visible Terran workload.

| Local receipt | SHA-256 |
| --- | --- |
| 32 matched visible combat updates | `25ad0aa1996e75ae4c7f1fc33db9cc3371884e3e560ca0b27d09163e400f7777` |
| Movement attribution | `4ab32781df8179ab0790e0584cd9b7f40c41e0c019b4bdcf26dab0d3b3592cdb` |
| Automatic target attribution | `3362602767464d449c42d1c33e2f341e9038a0c979fe122e7250509d105c08b0` |
| Tracer attribution | `0f957a5c39827e07536c3ba2228011e6d23aacceca0162ab612dc41c3bbfb7f9` |
| Uncached contact mask attribution | `b5f73869b67b6ba3fe6b89aab81d3a7279373131efadfcf022849a4649a7d021` |
| Monotone route attribution | `29c69f0f452168193bf40b57063a130fcac200b709f1ac907e956cc92b99d6a8` |
| Contact reuse candidate, 32 visible updates | `4091d35d8c47039fbcdfb4249e0eaa7dc80556bfda97bdca55aa585e3f5473d4` |
| Monotone witness candidate, 32 visible updates | `6f97555edf180785c0569ff1ef7921004b663ab90ce672d06f66a951cb59f52d` |
