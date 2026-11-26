# Reusing completed 128×128 movement fields

The Z7 battle window spends 22.45% of its cycles building full direction
fields for 32 destinations. A second read-only trace found an existing
packed field within 16 byte differences of the requested result on
1,374 of 3,024 calls. This counts actual bytes at function entry. A live
logical route slot can also contain a partially constructed field.

The optional `SC64_WIDE_POINT_MEMO_PROBE` keeps a private full field and
up to eight exact differences from it. A normal full flood supplies
each admitted result and its discovery count. A difference packet
records a byte offset, length and replacement bytes for each run.
Each packet is at most 384 bytes. The eight descriptors share a 1,536-byte
store. A new packet invalidates older descriptors only where their byte
ranges overlap, or when it replaces their descriptor. Failed admission leaves existing entries
intact. Rebuilding the walkability grid or resetting routes discards the
entries. The private cache leaves logical slot selection and eviction
in the existing route code.

## Measured on the release after #885

The paired ARM builds use the same inputs from release `d496f1a2`.
The sole configuration difference is the probe, enabled on candidate
`9f4e9d75`. Measurements include presentation waits and use mGBA's
guest clock. The private Z7 battle SRAM SHA-256 is
`2879841d6fbe26a27c35ea1316302419d0271abbcc395177393c8f1723be9bf5`.

| Workload | Updates | Release cycles/update | Candidate cycles/update |
| --- | ---: | ---: | ---: |
| Z7 replay with recorded commands | 4,901 | 2,026,958 | 1,868,756 |
| Z7 with neutral input from the same save | 1,024 | 1,410,676 | 1,341,712 |
| Z9 crystal-spawn save | 1,024 | 845,821 | 845,822 |
| Zerg custom, three CPUs | 512 | 980,940 | 980,937 |
| Terran custom, three CPUs | 512 | 984,033 | 984,032 |
| Protoss custom, three CPUs | 512 | 797,001 | 797,000 |
| Showcase scene 7 | 256 | 667,676 | 667,676 |

Both recorded Z7 endpoints matched the authored replay. All 72 sampled
world, display-page, menu, palette and framebuffer observations agreed
on the recorded window. The shorter pairs checked both framebuffer
pages, displayed pixels, palette, camera and full actor-bank bytes every
16 updates. Z7 and Z9 also saved during play, reloaded in a fresh
process and matched the control for 256 more updates, including identical
SRAM. Z9 used the ordinary northern flank save for this check.

The same paired products also continued Z7 from the ordinary battle SRAM
at replay tick 136,697 to victory at tick 270,585 using physical-button
inputs. All 45 subsequent host observations and the paired semantic-world
fingerprints matched. Both ended at frame 267,585 with victory, no loss or
scenario fault, and no enemy buildings. The terminal SRAM images were
identical (SHA-256
`66fec8e5ac2dbd922de486cea21bd45784c92e3b22b741917640a3079d8fce0e`).
Fresh processes loaded those images through the title-screen L input and
reproduced the victory state and semantic-world fingerprint, with unchanged
SRAM on export. These fingerprints cover selected persisted world fields,
not every transient byte; this is one Z7 continuation, not every campaign.

Three more paired campaign saves ran for 2,048 neutral-input updates each.
The late T5 save started with 80 units and 30 buildings, the P9 opening
with 162 and 114, and the P10 battle with 232 and 80. In each pair all
129 samples matched for the game-state fingerprint, actor arrays, camera,
both framebuffer pages, currently displayed pixels, and palette. Mean cost
changed by under three guest cycles per update in each scene. These runs
extend cross-cartridge regression coverage; the measured speedup remains
specific to the tested Z7 battle.

An earlier build of the probe kept a walkability snapshot. Its
neutral-input Z7 comparison caught a different display-page bit at update
496. Both pages, palette and visible pixels matched at that update, and
the bit agreed again on the next update. The rendering code queues a page
and the VBlank interrupt presents it. Keep checking both page contents and
visible pixels when comparing execution times.

The initial fixed-packet probe added 9,272 EWRAM bytes on Z10. All four normal products
linked with the option enabled. P10 had the smallest remaining space below
the existing EWRAM safety boundary, 2,672 bytes. The branch does not enable
the probe in release builds. Run `make test-sc64-wide-point-memo` for
the scalar direction, discovery count, buffer boundary and grid-edit checks.
Before promotion, qualify additional ordinary-input wide-map campaigns and
the remaining memory budget. Both current Custom Game maps are 64×64,
outside this cache. Keep their ordinary game and save paths in the product
checks.
Physical GBA pacing remains unmeasured.


## Shared packet storage

Nine smaller fixed layouts preserved the 72 sampled world states and both
framebuffer contents in the 4,901-update Z7 replay. The fastest of them
used six 256-byte packets and averaged 1,883,507 guest cycles per update.
The five-by-320 layout presented the opposite completed page at update 34,
so its visible pixels differed at that sample.

Keeping eight descriptors and sharing 1,536 bytes across their variable
length packets used 7,756 bytes for the cache and averaged 1,867,971 cycles
per update. All 72 sampled worlds, both framebuffer pages, selected visible
pixels, palettes and menus matched. A 1,024-byte store averaged 1,958,789.
A 2,048-byte store averaged 1,867,956, with a different visible page at two
samples. The host oracle forces a packet wrap over a live descriptor and
compares subsequent requests to scalar BFS. The four final ARM builds all
linked. They leave 4,192 bytes on Protoss, 6,844 on Terran, 10,528 on Zerg
and 14,324 on Showcase below the existing EWRAM guard. The final Z7 ARM
build repeated the 1,867,971-cycle mean and 72 matched samples. The
fixture also checks that a change
of grid dimensions and invalid dimensions each clear a populated cache.

The final shared-store ARM builds also matched all 633 sampled states and
screens across nine additional campaign, skirmish and Showcase windows.
In the recorded Z7 continuation, all 45 remaining checkpoints matched
through victory at frame 267,585. The terminal SRAM and fresh reload were
byte-identical to the release control (SHA-256
`66fec8e5ac2dbd922de486cea21bd45784c92e3b22b741917640a3079d8fce0e`).

## Current release Z10 trial

The shared-store cache was applied to release `81e76943` with
`SC64_WIDE_POINT_MEMO_PROBE` enabled only in a private Z10 build. Both
builds used the same release Z10 compiler and hot-IWRAM flags, prepared
audio bank and 56-mineral Blood Bath header. The rebuilt control ROM
matched the existing audio-hot release byte for byte. No release flag was
changed.

The same ordinary-input Z7 battle save and 4,901-update tape were replayed
on both ROMs. The control used 9,325,541,168 guest cycles (1,902,783 per
update); the cache used 8,554,350,089 (1,745,430 per update), an 8.27%
reduction. These are 8.82 and 9.61 emulated main-loop updates per second,
including presentation waits and UI, not physical-device frame rates.
An independent per-update counter breakdown over all 4,901 updates
attributes most of the reduction to movement orders: their mean fell from
1,212,133 to 1,052,743 cycles and p95 from 3,158,061 to 2,918,886.
Drawing averaged 426,205 versus 426,193 cycles. Whole-loop p95 fell from
3,877,437 to 3,624,023 cycles.
An instruction-level profile of the single slowest orders tick in each
of six equal intervals attributes 37.75% of all guest cycles on those six
ticks to point BFS expansion (37 calls), 19.80% to facility route fill
(5 calls), and 8.30% to point-direction packing. Facility fill had no
calls on the six midpoint ticks; these figures characterize spikes, not
the full-run cost mix.

Both matched the recorded start and end observations. At 72 sampled cuts,
semantic summaries, both framebuffer page hashes, palette, display-page
selection and menu state matched. These samples do not prove all transient
state or full-campaign equivalence on this release.

The candidate's EWRAM end was `0x0203b7e8`, 10,264 bytes below the
`0x0203e000` guard; the control ended at `0x020399a0`, so the increase
was 7,752 bytes. The candidate IWRAM stack checker passed: code ended at
`0x030062d0`, with a 5,856-byte main stack bound, 512-byte margin and
592 bytes spare; the IRQ bound was 264 of 416 bytes. These figures cover
Z10 only. Other cartridges and ordinary-input wide-map campaigns still
need current-release qualification before promotion.

| Input | SHA-256 |
| --- | --- |
| Control Z10 ROM | `82bcc90a835449abd4ac636085830b132bf9926714f6757b3ff2611b09c3b6da` |
| Candidate Z10 ROM | `1d3f8f098900371404523bc4d591f7ac568dddf4325cf9a4190e8c76526c38f6` |
| Z7 battle save | `2879841d6fbe26a27c35ea1316302419d0271abbcc395177393c8f1723be9bf5` |
| Recorded input trace | `022d0f08d4020ab86b3ac69fcb8dfe430b7837c3817d37ae785e241a62cd3ad1` |

## Z9 ordinary-input flank continuation on the current trial

From a retained Z9 crystal-spawn save, the same ordinary-button tape moved
the camera and units toward the northern flank for 8,851 input updates,
then paused and saved. Control and cache matched at 70 cuts during that
route and 16 further cuts over 256 updates after a fresh-process reload:
semantic world summaries, actor and building array hashes, camera, both
framebuffer pages, palette, visible page and display control all matched.
Both exports were byte-identical after reload. The pause SRAM was identical
between builds (SHA-256
`28395f85eb499cdb4311ae4b7df7946a35dbe586364015a851d0da4af306c5cb`).
The control took 7,322,034,084 guest cycles and the candidate 7,322,236,536
(+0.0028%); this route supplies a regression check, not a Z9 speedup.
The starting SRAM SHA-256 was
`d612c753633572eaef6e16f1bf4d6155164ceb8c87fe260d7ba3f43d0895a1b2`,
and the input tape SHA-256 was
`9a3144f9e9ba9863d9306546c2d6ec18813f6d6016f290653a8bea02550f0c9e`.
These periodic observations do not assert equality at every update.
