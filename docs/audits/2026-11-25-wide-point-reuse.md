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

The earlier trials establish the storage choice and record the first
Z7 victory, Z9 continuation and per-call movement profiles.
[Original #886 trial and evidence](https://github.com/ggu/starcraft-gba/pull/886)

## Z10 release qualification

The normal Z10 builds on `758cd54d` and `9661bceb` match their respective
control and candidate ROMs byte for byte. The control also matches the
packaged `77179474` Z10. The release Z10 recipe enables
wide-point reuse. Other route shapes dispatch directly to the existing
linear flood. Z5–Z9 use 128×128 maps, Z10 uses 96×128, and the two Custom Game
maps use 64×64.

The aligned admission scan skips equal four-byte blocks and preserves the
byte comparisons for mixed words and unaligned fields. The scalar BFS
ASan/UBSan oracle matches discovery counts and packed directions. It
checks all four destination alignments, packet bytes, packet wrap,
edited grids, shape changes and buffer canaries.

The 4,901 physical input calls from the archived Z7 battle SRAM use
9,239,954,696 → 8,371,236,926 guest cycles. The 9.40% reduction raises
measured emulated throughput from 8.90 to 9.82 updates/s.

| Timing | Release cycles | Cached cycles |
| --- | ---: | ---: |
| Mean whole-loop interval | 1,885,320 | 1,708,067 |
| p95 whole-loop interval | 3,852,238 | 3,548,716 |
| Maximum whole-loop interval | 20,594,660 | 20,172,878 |
| Mean orders | 1,194,136 | 1,014,043 |
| p95 orders | 3,130,096 | 2,834,871 |

The two builds match all 72 sampled world and actor records, camera,
menus, both indexed bitmap pages, palette, and display selector. Five
additional ordinary save replays cover Z1, Z5, Z6, Z8 and Z9. All 325
world, actor, fog, page and register samples match, and all five
save/reload comparisons are byte-exact. The largest mean cost increase
in those 256-input holdouts is 0.042% in Z6. These checks use emulated
guest cycles. [Campaign map sizes and holdouts](2026-11-26-campaign-route-map-sizes.md)

From the same battle save, both builds follow the full late route to
victory at world frame 267585. All 45 checkpoint summaries and semantic
worlds match. The gas balance first differs from the older authored tape
at input 175203 in both current builds, and their subsequent balances
match each other. Victory and fresh HOME reload export the same SRAM
(`4e8266b1564444a72c820568931c730875b82f29ae5b75cd3ba8f8951dd72722`).
Full late-route cycles fall from 150,670,690,880 to 147,257,735,147 (2.27%).

The final build also loads and advances from all ten archived Zerg chapter
results and passes 27 fixed lobbies and 18 sparse or random rosters.
Fresh-process Custom Game saves and continuations match the resolved
rosters. Ten cold-input Blood Bath samples choose a different native
start order from the control, consistent with the elapsed-time lobby
seed. The qualification archive SHA-256 is
`5c45a68190cb91c0b3950252ffbd7eedcbca9167a390f040f99eb80f205c963e`.

The normal 32 MiB Z10 ROM passes verification. The cache adds 7,756 bytes
of storage, and the linked EWRAM end advances 7,760 bytes from
`0x02039a68` to `0x0203b8b8`, leaving 10,056 bytes below the existing
`0x0203e000` guard. The hard IWRAM guard leaves 432 bytes beyond the
required margin with 5,172 hot bytes. A fresh Z10 ROM build with
`Z10_WIDE_POINT_MEMO=0` uses the previous linear flood path.

| Input or current artifact | SHA-256 |
| --- | --- |
| Release Z10 control | `1ff91aa97d0a3dcabdd76f9b52ce718880ab9be10fb935c9174154710c7e6a0c` |
| Z10 with wide cache and aligned comparisons | `aadcf119ebc4840a0c3a807b7ea815dcdf8dd5473961eb9f1ac572a2948baabf` |
| Archived Z7 battle SRAM | `2879841d6fbe26a27c35ea1316302419d0271abbcc395177393c8f1723be9bf5` |
| Archived Z7 ordinary-input trace | `022d0f08d4020ab86b3ac69fcb8dfe430b7837c3817d37ae785e241a62cd3ad1` |

## Copying cached fields

The four individually profiled Z7 inputs spent 98,418–492,056 guest cycles
in bytewise memcpy on release `6b01df0e`. Completed movement fields live in
ordinary RAM. The 6,144-byte copy can use aligned words when both ends allow
it. Unaligned slots use the existing byte copy. The scalar BFS oracle checks
all four slot alignments, packet contents, packed directions and canaries
under ASan and UBSan.

| 4,901-input Z7 replay | Total guest cycles | 95th percentile | Maximum |
| --- | ---: | ---: | ---: |
| Release | 8,371,236,926 | 3,548,716 | 20,172,878 |
| Word copy | 8,264,447,987 | 3,550,138 | 20,024,545 |
| Word copy unrolled four times | 8,255,076,504 | 3,544,214 | 20,001,582 |

All 72 sampled game states, actor arrays, both indexed bitmap pages, the
selected page, palette, camera and menus matched the release in the unrolled
trial. Five additional 256-input mission windows (Z1, Z5, Z6, Z8 and Z9)
matched all 325 state and video samples and re-exported identical SRAM after
a fresh load. The hard IWRAM guard leaves 432 bytes beyond the required
margin. These are mGBA measurements. The dense battle still advances at
about 10 updates/s. On the release build, the slowest 49 updates account for
3.85% of all cycles in the window.

Two systematic profiles of the release sampled every 67th input, starting
at offsets 1 and 34 in the 4,901-input window. All 147 individual clock
totals matched the normal runner. Of 264,994,911 sampled cycles, facility
routing accounted for 14.13% directly (10.28% and 17.75% per sample) and
linear point expansion for 8.68%. These were the top two functions in each
sample. Contact queries accounted for another 5.32%. The facility FIFO in
#896 is a higher-priority trial than further tuning the copy loop. Recheck
its effect on 64×64 custom maps as well as its 128×128 campaign gains.

The updated Zerg ROM also restored all ten archived chapter victories with
matching handoffs. Both custom maps passed 27 fixed-lobby and 18 sparse or
random seat tests, including fresh SRAM loads and continued gameplay.
