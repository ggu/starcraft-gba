# Carried Z9 on the normal Z10 cartridge

The earlier winning Z9 HOST tape no longer matches the normal Z10 cartridge
from `6e843d814da131950c92da79901985fc2e05ff78`. At call 2,118, game frame
2,000, the cartridge had 120 enemy units and the tape expected 123. The
other named fields matched. A fresh ASan/UBSan HOST run using the matching
Z10 flag vector also had 120 enemy units there. The fresh tape's first
seven checkpoints then matched the cartridge through frame 10,090, including
the ordinary pause save and return to play.

The old controller stalled before it started the source countdown. It kept
sending the air sweep toward a Cannon on the flank and never advanced past
the same leg. Limiting the sweep to threats near the road alone still stalled:
the controller failed to select a Mutalisk group for repeated attacks on a
Zealot. The updated test controller also retries ordinary selection outside
the initial search near the air group's average position. It buys and orders
units through the normal controls. Mission scripts and the engine are unchanged.

With those two changes the matching HOST configuration wins at frame
72,628, after 74,386 calls, with no fault. Two Z10-flag runs and one
standalone Z9-flag run produced byte-identical tapes. A local test hook
hashed the covered route state after each game update and before the next.
The controller did not alter that route state. Its Drone reaches the
formation, waits for the source countdown to finish, and brings the new
crystal home.

```sh
make CLANG='clang -D_FORTIFY_SOURCE=0' build/test-sc64-z10-carried-z9-run
./build/test-sc64-z10-carried-z9-run .local/sc64/z9-current.jsonl
```

The unchanged normal cartridge won from fresh HOME in mGBA at the same
frame. All 32 checkpoints matched across 15 named fields, including the
countdown and three active AI towns. The controller's private waypoint index
is not a guest field. The tape contains 10,044 held-key transitions and an
ordinary pause save. Six A presses and releases dismissed authored pages
without advancing the simulation, for 74,398 physical updates in total.
The replay runner made no guest-memory writes and did not load SRAM.

The live scheduler reported 14 trigger rows and no faults. The observed
countdown passed 582, 417, 228 and 96 seconds before reaching zero. At
frame 59,894 a live human Drone acquired the new crystal at (896, 1456).
The crystal followed that same Drone to (269, 240) inside source location 1.
The human source outcome became victory, and the primary objective and
campaign completion bits were set. This checks the port's authenticated
trigger program and its crystal carry, with no claim of native timing
or AI parity.

The full replay averaged 1,762,501 guest cycles per main loop (9.52 emulated
updates per second), including rendering, UI and presentation waits. It
does not meet the near-30-fps target or measure physical GBA frame rate.
This covers one complete route from HOME; it does not test Z10 SRAM reload.

- ROM SHA-256: `6224e1821edacff7a88908d2a9b5bc64b496094a567213153a1796c3d36715d3`.
- ELF SHA-256: `9bbe176b7d2916f945aa75691feccc2d1743e8b9e18b6e40807c99e05883c307`.
- Earlier tape divergence receipt SHA-256: `1024db764073c02eaed9be4747d8161d9c2333a777906018c1d3ef93c21a18f2`.
- Fresh baseline prefix receipt SHA-256: `2dd14cd77770ea7a05f27b7bfd522199f80bc2dd86e58e9bec1006ae9fa72221`.
- Winning HOST tape SHA-256: `26aecd7758ac368b9f8e34cb1764f47bf12c0bf6d7f5e8607aec9e096ae610bf`.
- Full physical win receipt SHA-256: `9309bbf77599bf779bb45443fe5f52d7766800731c21466b3d6d6a7362503281`.

## Exact November 13 KODAK cartridge

The same tape won from fresh HOME on the exact `cbba1c5` Z10 cartridge.
All 32 checkpoints matched after 74,386 tape calls and 74,398 physical
calls, with the six authored acknowledgments and no loaded save or guest
writes. The source countdown expired, and the same live Drone carried
its crystal from (896, 1456) to (269, 240), inside source location 1.
Victory at frame 72,628 set the objective and completion bits with zero
trigger faults. Mean cost was 1,748,268 cycles per loop, or 9.60 emulated
updates/s including the UI and presentation waits.

- KODAK ROM: `e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907`.
- Matching ELF: `74c4b2394c4aee619bf2ea0549c31de7987827a915c32c5c50e7a4fec58ad6c1`.
- Physical observer: `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549`.
- Replay harness: `5c57f0a65c1af8f9f216cd664050125aaea602dc42a8cdd1b06e0dc3ad03bba6`.
- Full KODAK receipt: `556cf4dd17b91d6705c101b4b6920712b70ec21fd632fe274144baf6bd394e89`.

## November 24 installed cartridge opening and air production

On the November 24 Z10 image, ordinary HOME controls open Z9,
acknowledge the opening and save at frame 11. Fresh HOME matches
the initial world and SRAM. The army starts with six Zerglings
and two Hydralisks, four Drones, three Overlords and a 750/750 bank.

Two Drones begin mining. The Hatchery queues three paid Drones,
and a worker builds an Extractor at (416,240) that finishes by
frame 1,112. The next orders assign a gas worker and finish the
Hydralisk Den and a full-health Lair by frame 4,058. The Spire
finishes by frame 6,554. Three paid Mutalisks hatch at full health
by frame 8,925. The Lair has 1,548 HP and the gas worker has 8 HP
after pressure at the base. The bank is 1,020 minerals and 398 gas.
All three stages replay independently with matching state and
SRAM. Fresh HOME restores the force, buildings and bank. Build
more air and secure the gas line before attempting the crystal.

Two more paid Mutalisks hatch and two new Drones work the gas
line. An ordinary move order brings all five full-health air
units to the first rally of the earlier winning route. Fresh
HOME at frame 12,531 restores them near (560,250), three gas
Drones and the 822/486 bank with identical SRAM. The objective
countdown is still at zero. The earlier winning tape gathers
fourteen Mutalisks before clearing the road.

The next paid orders bring the group to eight full-health
Mutalisks and fill available supply. Fresh HOME at frame
19,211 keeps the three gas workers, a mineral worker and
an 866/874 bank with identical SRAM. Three paid Overlords
in a later branch allow further Mutalisk production. In
that branch, an approaching Reaver reaches the gas line
before the air group stops it, and the Extractor and gas
workers are gone by frame 24,041. Continue from frame
19,211 and target that Reaver before committing the air
group to the crystal road.

On a separate branch from frame 19,211, the eight Mutalisks
attack-move into the Reaver's approach. The Reaver is gone
from the base area in later observations. Fresh HOME at
frame 23,282 gives nine full-health Mutalisks, two more
queued, three surviving gas workers and a full-health Extractor.
The next paid orders raise the group to fourteen full-health
Mutalisks by game frame 26,691. Fresh HOME keeps both groups
and the 442/1,018 bank with identical SRAM. The eight
forward fighters and six near the Hatchery still need to
assemble for the crystal route.

The combined group destroys the first Photon Cannon on the
road at (992,304). Fresh HOME at frame 27,561 restores all
fourteen Mutalisks. Twelve have 120 HP and two have 100 HP.
The gas line survives and the Extractor retains 750 HP.
The bank is 490/1,106 and the countdown remains at zero.

| Local receipt | SHA-256 |
| --- | --- |
| Paid air units and gas workers | `90cb4bc3dbdc2b12b2c28c54108be3cd401d849377ab954e2139cbf21b1447fd` |
| Air rally and fresh HOME | `679241e68bc02b3d997abdec94b2ac6a9d04392764ec0c4e951a7ba0ca4e33b1` |
| Rally SRAM | `4e499a8d708913fdc1c9d3755925ed56e0c6610fda6e68565d9faf4a9405999c` |
| Eight Mutalisks with gas workers and fresh HOME | `6da811f52de1caab0785e426fcea3c0b8aa42339e50b16d2ae58115a3d46e4d3` |
| Reinforced SRAM | `c4d4911f41e0b30d8f5093489f423bc8cd10c155501021c1d24ba1e82600ae27` |
| Reaver interception and protected gas | `5c9a4eabe96c3291d1229999cda87ae4bc36c8308bfb35bc6c6087ea2e008c7f` |
| Fourteen paid Mutalisks and fresh HOME | `2594f3ed328f1f2bbf4034423acfab02f959e8bde74a37f184e484a963a505d3` |
| Fourteen-Mutalisk SRAM | `92bc86d2702d6c8969a4ac80190c1b6aedad85dc2303f2e1950d5171d912c1ae` |
| First road Cannon attack | `53394856447841e82b979826b04771490834e05732f8f94ef58f020daf3084f6` |
| Road attack fresh HOME and matching SRAM | `c53b63e53bada5cd317a0c9aaa1419527d4dda58b55d8a0e1663ebe3f1c4326e` |

### Opening frame cost

Three independent 120-update windows use neutral buttons after
each fresh HOME load. All 360 measured updates exceed the
561,792-cycle two-VBlank budget. The totals are mGBA guest cycles.

| Starting save | Mean loop cycles | Mean simulation | Mean drawing | Updates/s |
| --- | ---: | ---: | ---: | ---: |
| Opening, frame 11 | 820,131 | 479,915 | 339,830 | 20.46 |
| Lair, frame 4,058 | 865,060 | 498,827 | 365,846 | 19.39 |
| Three Mutalisks, frame 8,925 | 875,885 | 492,239 | 383,260 | 19.15 |

Eight instruction-attributed updates from each window match
ordinary controls on both framebuffer pages, palette, camera,
world, phase counters and elapsed cycles. The static defense pass
uses 97–98k inclusive cycles per update. Thirty power queries
together spend roughly 44k exclusive cycles, including the lazy
Pylon roster setup. The target index's owner-list setup uses
60–61k exclusive cycles in the same cuts. These measurements
identify present costs in this dense campaign opening.

Read-only breakpoints separate the lazy roster setup and
lookup in one update per window. Each sees 69 completed enemy
Pylons and checks 30 Cannons. The first call reaches the
completed roster after roughly 7.5k cycles. The lookup portions
use roughly 38.6k cycles across 902 actual candidates. An
offline model grouping the same owner list by 128-pixel Y
bands reproduces all 90 power results and visits 172 candidates
per update. Its visit reduction excludes indexing and probing
cost.

An authored ARM fixture tests the eight-bin layout with exact
power masks and the same ROM wait setting in each pair. The
dense single-owner scenario uses 69 Pylons and 30 queries.
Its `-O2` pass drops from 80,203 to 39,349 cycles. Its
`-Os` pass drops from 106,011 to 48,997 cycles. The sparse
three-Pylon/two-query case grows from 6,658 to 7,158 cycles
at `-O2`. The four-query case with no Pylons grows from 6,404
to 7,414. The fixture reports an extra 40 bytes across the
caller and lookup stack frames. Seven sparse, dense, mixed
owner and negative-coordinate holdouts plus the main dense
case return identical masks.

A second trial keeps owner lists below 24 completed Pylons
and groups them by Y once the count reaches 24. The main
dense case takes 41,838 cycles at `-O2` and 52,561 at `-Os`.
The three-Pylon case takes 5,360 and 9,601 cycles respectively.
The no-Pylon case takes 5,272 and 9,359. The nine-Pylon
holdout uses 858 additional cycles at `-Os`. All eight
authored guest masks match the scalar reference. Separately,
ASan and UBSan compare 153,600 queries across counts around
the transition and across negative coordinates, incomplete
Pylons and unsupported owners. This is a local source
experiment. Verify stack, full-frame cost and saves across
low- and high-Pylon production workloads before adoption.

| Local receipt | SHA-256 |
| --- | --- |
| Opening launch and fresh HOME | `6e76e7e22d0a4f5cdcfb0070d750df11b5973a92a6487bfa37ea3ff4aabbc4be` |
| Independent paid economy, tech and air route | `5f2113b311a2e507f84f93729155a3a7d3e2249a12b8f92bbde81e6d81c1349a` |
| Opening and Lair phase windows | `a5d834a4b77b3b07de6dda0563ae2831ae4ba03e4d22cea4bf4bebab7e9a9b4a` |
| Paid air phase window | `d83c652f165e193d228b4dfefdbf202caa244c070057247d359449d151db274d` |
| Paired instruction attribution | `b7bffd3d4d7bb8fdaa1a6906fc7749c18d6c38aa4eb6860e6d8c2ae7987fe714` |
| Power scope split and modeled candidate bands | `6d30464f2c62388a31c064c18abec3ac7106cec1a4e6fe4bce39afed1dac3f48` |
| Authored paired power-bucket timing | `65374ac4e3f78acdf8c979d3aa21751ed43751d648ba4a929d7c0844b794a971` |
| Authored paired adaptive power-bucket timing | `c08867d3ba536ce6bf1f02422f9248be4076e726e1ade947270e6e34801885a1` |
| Adaptive power-bucket sanitizer result | `e054e695d0dbe8d87f4d5d60bbac30309a6507a1607714154162e04ba2c070b2` |
