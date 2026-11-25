# Waiting training orders at the local supply cap

## Stock Nintendo 64

The US N64 Wasteland replay uses the normal Command Center buttons at
18/18 supply. An idle Command Center with 145 minerals refuses its
first SCV, retains the bank and shows the supply warning. With one
SCV training, the same control charges for a waiting SCV. When the
first SCV completes, the waiting SCV becomes the head and holds an
empty progress bar at 18/18. Further ordinary input charges for an
additional order while that head is stalled.

A separate fresh load restores the blocked SCV at 18/18. A worker
places a Depot with the ordinary N64 build menu. Supply remains
18/18 during construction. After it completes, the HUD shows 19/26
and the previously blocked SCV has a progressing training bar.
With neutral inputs the SCV finishes and the queue empties.

| Native input tape | SHA-256 |
| --- | --- |
| Idle refusal | `49c0b71c7de872017c266b844062c323956e3937dd916a94adeb995b848078ab` |
| Paid waiting, stalled head and additional order | `a7ffb21f14d8ff29af91e4468ab0ac0cd9c0647d47bf287164b1d591f2b019f6` |
| Fresh load, Depot and resumed order | `ce96bcb316f09f53ecb11ec8545b3d622ff4cabc91863e3b4a260da20b0810a9` |

## GBA comparison

The previous Terran build counts all paid orders against the local
admission limit. At 14/18 live supply, an active SCV and three
waiting SCVs reserve the remaining four supply. With 361 minerals
and one open slot, the fifth order is refused. The new build takes
payment from the same ordinary pause save, then stalls the paid head
at 18/18. A sixth order is accepted behind that head. The two paid
orders, zero progress, and world state survive a fresh HOME load and
byte-identical SRAM re-export. An SCV builds a second Depot through
the field menu. When capacity rises to 26 at frame 10,305, the head
advances to progress 1. Ten updates later the progress is 11.

| Local receipt | SHA-256 |
| --- | --- |
| Prior GBA refusal | `e1fefa247565a565427140d9f846c48b44ae509111b0bdd0c5979988aae11e50` |
| Candidate admission and fresh blocked head | `36773cdf34d5b71d49aeb7575d76be83124d141e7512e7df533dab5ea9a4fee4` |
| Candidate SCV and Depot completion | `408b7cafe3e9419e1ca144bb6e4be1fc5e89e081002df5ef17c5f68c3407c2ea` |

Terran and Protoss facilities use the same rule in campaign, Custom
Game and Showcase. At idle admission, reserve the first job before
checking another facility in the update. The building tick consumes
that reservation and advances frame one, or releases it when the
facility has lost power before starting. Running heads stay paid and
retain progress through supply loss. Paid heads with zero progress,
including those loaded from older journals, remain queued and retry
at the start gate. The normal save layout is unchanged.

The host checks cover both facility races in campaign and Custom
Game. They check idle refusal, competing first orders, paid waiting,
cancellation, capacity restoration, and continued progress of a
running head after capacity is lost. The global 200 reservation
ceiling, existing AI planning and Zerg larva rules stay in place.
Native comparison of those rules, and multi-facility priority when
several heads can resume, remain open.

All four normal ARM links pass their existing memory guards.
The three race builds pass all 54 ordinary Custom Game openings,
pause saves and fresh HOME continuations across both maps, each
human race and one to three opponents. Matrix receipt SHA-256
`ba26d4c025bf52e1a98e8aa1ac5fbf96971611415ef584d2efcf47b3cc7b5669`.

On the first qualified supply build, Wasteland replays from
the campaign menu and wins at frame 10,120, with an ordinary
pause and load during the route. Zerg 2 wins at frame 10,209
and restores its result from fresh HOME. The retained Protoss 7
assault repeats its victory at frame 80,828 and fresh result load.

| Campaign receipt | SHA-256 |
| --- | --- |
| Wasteland | `21e6e26c309bcde56f99d9cd78378b35f71f3b9ee99644b255b7eda8e563a30c` |
| Zerg 2 | `1de85ded0493783be910cb36dc31fc71e831ffdc975da4d207bef0701429bd29` |
| Protoss 7 assault | `600890be9e06d27a14d1f2c9f020ffa27c356923157390204f36ab6aab3352bf` |

## Final combined build

The supply implementation combined with the selection rendering
release passes the source boundary, toolchain doctor, default
ARM build, and complete asset-free suite. The Python batch runs
1,183 cases with 35 skips. All four production ROMs link within
the same memory guards. The four 96-update P10 and Z10 busy
comparisons add 0.008–0.039% to mean cycles versus the selection
release. Every paired world, both framebuffer pages, palette,
display selection and fresh save continuation match.

| Combined-build check | Receipt SHA-256 |
| --- | --- |
| 54 ordinary Custom Game openings and fresh loads | `ba93e9c141ffe48c78d04f98b7cf553e898a66a26ba1211c1a0d8451303450a2` |
| Ordinary paid waiting and fresh blocked head | `3d7460bc56fd7cd577d93fc4bfdd4ca58c93a2f7f833a56fb5e515479748a754` |
| Wasteland through victory | `c5800bfd69336926e6a540665fc8d4fb564a4419d35214a3e8845a2f21aaef29` |
| Zerg 2 through fresh HOME result | `4aac302030605d3d7de2d47d6d4f34aa3aef3f19ea792c7f83b2e9887e311a9e` |
| P7 repeated assault, victory and fresh HOME result | `600890be9e06d27a14d1f2c9f020ffa27c356923157390204f36ab6aab3352bf` |
