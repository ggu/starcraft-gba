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
compares subsequent requests to scalar BFS. It also checks that a change
of grid dimensions and invalid dimensions each clear a populated cache.
