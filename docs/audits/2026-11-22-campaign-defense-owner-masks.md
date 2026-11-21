# Campaign defense cell owner masks

The campaign defense pass already links every live actor into a
spatial cell. Each cell now also records which owners have linked
units. A defense skips a cell when none of those owners are hostile.
Queries still check live visibility, range, air and ground eligibility,
and the existing slot tie order. An attack attempt may mutate the
unit pool, so the rest of that defense pass uses the original lists.
Invalid owner or coordinate rows still use the full-scan fallback.
The mask uses 64 bytes of local scratch and adds no save fields.

Set `CANONICAL_ZP_DEFENSE_OWNER_MASK=0` to use the original spatial
query. The target configuration stamp tracks the setting. Terran's
ROM matches release `fe4e1502` byte for byte. The Protoss, Zerg and
Showcase links use the same toggle wherever the campaign defense
provider is compiled.

## Local qualification

The exact paired P10 replay wins with matching SRAM, 68 world,
video, palette and fog cuts, and 9,489 matching frame and route
counter rows. Full replay cycles fall from 14,986,065,403 to
14,865,866,738 (0.80%).

| P10 cycles per advancing gameplay update | Control | Candidate |
| --- | ---: | ---: |
| Mean | 1,566,617 | 1,553,638 |
| p95 | 2,091,101 | 2,080,145 |
| Peak | 3,938,013 | 3,906,141 |

Z10 Blood Bath matches 31 world and video cuts, 16 fresh
continuation cuts, per-update route counters and all SRAM bytes.
Mean cost is effectively flat. Its p95 increases from 1,114,663 to
1,115,886. P10 and Z10 Outpost comparisons each match 31 video and
state cuts, 3,600 route-counter rows, eight continuation cuts and
SRAM. Their mean cycle costs match to the reported precision.

The existing randomized Z/P defense test compares the spatial
pass to a scalar full-tick reference. It passes with ASan/UBSan on
both the prepared P10 and Z10 vectors, including the Z10 Carrier
forced include. The exact four openings, 41 camera cuts per product,
and 54 ordinary Custom Game launches and fresh HOME continuations
also pass. A separate 1,134-launch sweep verifies both maps,
each human race, all nonempty named CPU seat arrangements, and
the actual town hall starts on each of the three race ROMs.

The full Z7 tape retains the original win at frame 228,128.
It matches 254 world observations, ten display cuts, six
route-cache cuts and the pause, victory and fresh HOME journals.
The mean cycle cost of the two late windows increases by 0.41%
and 0.44%. The earlier dense window changes by less than 0.01%.

| Product | End of IWRAM | Change |
| --- | --- | ---: |
| Terran | `0x03004b38` | 0 B |
| Zerg 10 | `0x03004f60` | +112 B |
| Protoss 10 | `0x03004ea0` | +144 B |
| Showcase | `0x03004f00` | +152 B |

All four links stay within their memory guards and retain the
previous EWRAM endpoints. The instrumented P10 replay retains
6,504 untouched stack bytes through victory. Both Z8 canaries
retain 7,216 bytes over 1,200 fresh-loaded updates. Paired Showcase
canaries match world and video cuts and preserve 7,568 untouched
bytes in Outpost and 6,832 in Siege Line.

| Local evidence | SHA-256 |
| --- | --- |
| Exact P10 replay comparison | `8169c61cad54d6b464e896b125eaad86ab5c68299f893c1c5c3fecc728657af7` |
| Exact Z10 Blood Bath and save comparison | `5913dd2c9e71d4a3e91c551d06f57029eec9f03d92b3ac8e27a4fc9c6d1d13e9` |
| Exact P10 and Z10 Outpost comparison | `ee156a8010414c2a4ac91e9ab48eed34c1afe5ff25d2880edd6c7d144b9ca976` |
| P10 stack canary | `07754587c0dda34f3ee2a9ab4c939bd782f17cdb0d81868bc9285a6ae0a3fb97` |
| Paired Z8 and Showcase canaries | `2ad6499c4a99a288ecdc395d39ab9a3ac0cf28cae95022f3f85403f9514f9f37` |
| Custom Game and fresh HOME matrix | `43cbd010ca1a24adb2bbe3040ee19ff8aff46ebff800787292acc49463c38264` |
| Exhaustive race-cart launch matrix | `22b91c681126d922a356af81789e5f017118dd5418063e96c1ebd0e3a7c2c82f` |
| Exact Z7 ordinary win and readback | `5411c084063e9e0a0577fe8a2a2300f313e4be709a44d976cd6cc793b825c052` |

## Refreshed normal Zerg 10 route

The previous tape diverges at call 19,860 and first restarts
at call 20,384 on both the released cartridge and the candidate.
Both runs follow the same state cuts and end unresolved.

A fresh ordinary controller built from current source generates
a winning tape at frame 51,588. The normal control from HOME
matches all 23 HOST checkpoints. Both exact ARM builds save
at frame 2,526 and reach victory after fresh HOME loading.
All 21 continuation checkpoints, dialogue observations, objective
state and Crystal transfers match. A third HOME process reads
the victory on each cartridge and exports identical 32 KiB SRAM.
The refreshed paired replays have no checkpoint drift or restart.
The candidate uses 67,781,661,859 cycles versus 68,333,957,223
for the control, a 0.81% reduction in the continuation.

| Local evidence | SHA-256 |
| --- | --- |
| Current controller tape | `c9c4b76362e3bb389a16068b02fc48c52ebb6b82c5477cb6ba71e396434353b9` |
| Control from ordinary HOME | `b288186e999fb5d7cac577bc5f0a2c500d34c9579e802fc420b3d7dcd4e49710` |
| Candidate fresh HOME and victory | `171ba2e5fa3aa4858b816989fc5ea90892b7a57c48b1dc66ce12060bf5242c2d` |
| Both continuations and result readbacks | `8f3ae010054af42654c1fb919784cc45dc492fa4dedf357963f672beab3b24b8` |

## Remaining order cost

Sixteen current P10 updates with no new route build average
1,547,717 cycles. Target search averages 64,512 cycles and
the target index rebuild 61,656. Their combined cost is 8.15%
of that sample. Cell insertion accounts for 37,237 rebuild
cycles, the coordinate check and link setup for 10,741, and
head and owner-axis clearing for 3,072. The current clear already
tracks dirty heads. Rebuilding the cells also preserves
live ownership, coordinates and the target tie order.

Four additional cuts with new route builds average 167,535
cycles in depot work. Depot work averages 10,820 in the other
cuts. Retain exact FIFO completion counts while attributing
remaining stalls, since completing a queued field can cost
cycles without incrementing the new-build counter. Earlier
index-retention trials increased sample costs. The current
instruction and counter validation digest is
`22450953252a468e4b303ba40a1cc52979dbcf1d8ee2facad7b3a1b574326a7a`.

Three measured late Z7 route bursts on `fe4e1502` average
roughly 1.37 million cycles in point fills and 490,000 in
target search. Attack-policy checks account for about 283,000
of the target-search cycles. One sampled late update without a
fill still exceeds two million guest cycles. The six-cut
instruction report digest is
`d9e210530fbf2fede720a168cc0476540a0dca74005780e848e3564476121d8b`.
