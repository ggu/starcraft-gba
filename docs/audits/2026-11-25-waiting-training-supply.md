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

## Native Protoss follow-up

An ordinary N64 Challenger Custom Game starts Protoss at 4/9 supply.
The initial observer ends with a generic snapshot failure. The interactive
observer's error also covered its bounded input log. Raising that limit
in the isolated v148 build permits the paired runs to finish at step
13,370, with nine frame captures and GPU ready in both runs.

One schedule reaches 9/9 with an active production panel, a blank
progress bar, and empty visible waiting positions. An A pulse at
step 13,000 places a Probe in queue position 2 by step 13,020. A
second A pulse fills queue position 3 by step 13,170. Supply stays
at 9/9 and the progress bar stays blank. The mineral counter
animates between captures. In the matched comparison, the idle and
blocked banks are 267 and 217 at step 13,000, 276 and 176 at step
13,130, and 312 and 162 at step 13,370. The difference grows by 50
for each of the two late waiting orders.

Omit the earlier Probe pulse at step 11,530 to leave the Nexus idle
at 9/9. The same two late A pulses now display `NOT ENOUGH PSI`
without opening the production panel or filling a queue position.
The visible minerals rise from 267 at step 13,000 to 272 at step
13,040 and 288 at step 13,170. Native Pylon completion and resumption
remain open. The native Custom Game save command is disabled.

The packaged GBA Protoss ROM repeats the idle and blocked cases
through its ordinary controls. An idle 9/9 Nexus denies the first
order and keeps its 1,416 minerals. With a blocked order at the
head, the Nexus takes 50 minerals and adds a second waiting Probe.
Both paid orders retain zero progress after fresh HOME and an
identical pause journal re-export. A Probe places a Pylon at
(216, 168). Supply stays at 9/9 during warp-in, becomes 9/17 at
frame 3,697 and starts the first Probe on the following update.
That Probe completes at frame 4,072. The internal supply fields
store twice the displayed units.

| Local receipt | SHA-256 |
| --- | --- |
| Native interactive observer executable | `abcc1ac5876de2310391dbef90aae9b3f5cded7554c1028ecdd6986d506d7697` |
| Blocked-head route command log | `0e4b7ac02ea243c6c389d830177eea27bae89fa9099e5d8d36a3959d52738bd9` |
| Blocked-head route second waiting icon | `4ed07b08daa85d60ae29f69c67bc815f2fe66888246727cea40ee1db13d21f43` |
| Idle route command log | `a396a01eaa7b8eec6fc8970edb91687f101a1858e02797b5e9fe4b75378cb3f8` |
| Idle Nexus warning after first attempted order | `8a6731ebd878d5068df06186dd420c3de6442546599e7ba7652a2a219bd4170f` |
| GBA idle, waiting, and fresh-load receipt | `adf2b815ebcc0722aa87288c6741fd422b2c3a4329d9ee12feeab334b2f4b9c9` |
| GBA Pylon construction and Probe resumption | `573ecd0fc0fdf6680c681fc42c65ea1e807a3bac12a44fc369ba8a377d9741d5` |

### Close the native Nexus command card at the supply limit

A paired native Challenger run starts from the same queued Nexus at
9/9. The interactive observer exposes B as mask 512, using the mapping
already shared with the actor observer. One run holds B for ten steps
after step 13,370, then releases it. The control uses neutral input
throughout that interval. Both observers reach step 13,470 with GPU
ready and four captured frames. The pre-press images match byte for
byte. Five B polls are pressed in the intervention, zero in the control.

In the final control image the command card is open with Probe selected.
In the B image it has closed and the field BUILD hint is visible. The
selected Nexus, two waiting Probe icons, 162 minerals and 9/9 supply
remain visible in both. Use an explicit cancel command for a future
native refund comparison. The final raw image hashes are
`75063a640617bb1d63068dd772b58a7f5bed9766eeedf79dc7043d6110384b2e`
(control) and `ee672ffc29051f9c421b5fe0f26bc97f4a8ff7bcb58fc95b2fd96b8620f70661`
(B).
