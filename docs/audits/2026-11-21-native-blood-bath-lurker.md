# Native Zerg CPU Lurker morph in Blood Bath

A retail N64 Blood Bath continuation in ares uses neutral controller
input from authenticated frame 9,791. The read-only observer samples
100 native states through frame 12,511. An ordinary Zerg CPU morph
appears in three distinct actor slots. Each keeps its map anchor
through the observed transition.

| Native frame | Owner and slots | Type | HP | Order and completion |
| --- | --- | --- | ---: | --- |
| 10,577 | CPU 1, 118–120 | Hydralisk | — | Completed |
| 10,610 | CPU 1, 118–120 | Lurker Egg | 200 | 42 |
| 11,174 | CPU 1, 118–120 | Lurker Egg | 200 | 42 |
| 11,200 | CPU 1, 118–120 | Lurker | 125 | 41, completion bit clear |
| 11,225 | CPU 1, 118–120 | Lurker | 125 | 163, completion bit set |

The three anchors are (325, 1754), (548, 1993) and (379, 1839).
The egg interval is bounded by these cuts to 564–623 native simulation
frames. The observer also finds twelve Larva-to-Egg and nine Egg-to-
Hydralisk same-slot transitions. Sampling leaves the exact morph
frames, research trigger and planner conditions open.

The source catalog represents the Hydralisk, Lurker Egg and Lurker
roles. The [GBA skirmish planner](../SKIRMISH-AI.md) still lacks
these ordinary AI unit morphs. Use a paid per-owner research and morph
fixture to capture eligibility, affordability, supply, egg behavior
and continuation through victory or save/load. The reference cuts
provide the actor lifetime and ordering expectations for that fixture.

| Local evidence | SHA-256 |
| --- | --- |
| Native observer receipt | `8a177519c73bab426494ce8adef115c31bc17027848cde17636f23f74fa2c485` |
| Native sample rows | `7d909dac6827b442d5a7309abfa6ace0071344cd9f6af7f133b3a245a930749a` |
| Authenticated frame 9,791 checkpoint | `45b5955e9ec299048cd246008986f11595248ae9048ad8a68a719f4704ed02e0` |

## Follow-on Egg observation

A fresh isolated ares continuation of the frame-12,511 Blood Bath
checkpoint samples the neutral game through native update 12,920. At
12,824, CPU owner 1 has an unfinished raw-36 Egg in slot 130 with 200
HP at (356, 1798). At 12,857, the same slot holds a completed raw-37
Zergling with 35 HP at (385, 1775). A second completed raw-37 Zergling
appears in slot 135 with 35 HP at (333, 1817). This is consistent with
a paired hatch. Its exact update, the source of the second Zergling,
and the purchase cost remain unobserved in these samples.

The released GBA Custom Game uses a Hatchery queue. In an ordinary
Z10 match between a human Terran and a Zerg CPU, the queue held a
Zergling order at frame 6,101. By frame 6,126 the order had cleared and
two 35-HP Zerglings occupied new slots 24 and 25. A separate fresh
run sampled every update around this AI's first Zergling order.
Between frames 4,175 and 4,176 the queue appended a Zergling after a
Drone and the owner's bank went from 1,228 to 1,178 minerals with
zero gas throughout. Saving at frame
6,101 and loading from fresh HOME produced the same world as
uninterrupted play at frame 6,161.

An independent Egg actor is available in an experimental Custom
Game profile. Qualify its payment, second-slot reservation and
save continuation near the actor limit before promoting it.

| Follow-on receipt | SHA-256 |
| --- | --- |
| Frame-12,511 input checkpoint | `11b71a1ff0dde5be696861df064cc93bcd33ee9a5ed571844186569a03aa74f0` |
| Live samples | `4d91d7452672d8289401d44da2caa677e5ad052b70379d29eb41ba22a993eac5` |
| Final native state | `4da3a3139006c693a2db4cd35ec215a5d8f6cf42ec80f0651128e89e3c8e7c11` |
| Observer receipt | `1d2a07d377c445214f5a7fd1da90ab7c9c42d8fdcc2ea836a4aed13d40689ec8` |
| Normal Z10 Hatchery queue samples | `d0da57f66d205e101da06a6f7812360bdc9b932f48065cc593c0cff3aec2642c` |
| Normal Z10 first queue debit | `8fd153dda9abe36f97783a294899d15dc83cf4a0e917c5c132cd30c8e4ecc87c` |
| Normal Z10 pause and fresh continuation | `56fc61d79df01c534c25eb96488cca605440605348c9781fa8906cbf6cdac2e7` |

## CPU Drone production and fresh HOME continuation

A further neutral ares continuation begins at the authenticated
native update 12,920. The read-only actor sampler follows CPU Zerg
slot 133 through four states.

| Native update | Type | HP | Observation |
| --- | --- | ---: | --- |
| 12,920 | Larva, raw 35 | 25 | At (608, 1977) |
| 12,950 | Egg, raw 36 | 200 | Unfinished, same anchor |
| 13,266 | Drone, raw 41 | 40 | Completion flag clear, same anchor |
| 13,301 | Drone, raw 41 | 40 | Completed and moving at (669, 1980) |

The normal `fe4e1502` Z10 cartridge uses a Hatchery queue in
the corresponding ordinary Terran-versus-Zerg Blood Bath test.
From frame 47 to 48, the CPU bank goes from 50 to zero minerals
and Hatchery slot 1 queues a Drone. At frame 422 a new 40-HP
Drone appears in slot 9. The GBA role maps to source raw 41.
Its Custom Game runtime raw field is unset. The two observations
use independent clocks and leave the native payment and exact
transition updates open.

Saving the normal GBA game at frame 48 and loading from fresh
HOME restores the queue and named gameplay state. At frame 548
the resumed state matches the uninterrupted run, and the fresh
process exports byte-identical 32 KiB SRAM.

The experimental combined ARM Egg/Carrier build also restores
an actual CPU Egg in an ordinary human-Protoss versus Zerg
match. At frame 6,001, CPU owner 1, slot 30 holds an Egg with
134 updates remaining. Fresh HOME restores it and both runs
match over another 100 updates, including the original Egg's
timer and another CPU Egg.

An ordinary Protoss build order on that experimental cartridge
reaches a Stargate and Fleet Beacon against a Zerg CPU. At
frame 17,294, the paid Carrier order occupies Stargate slot 18
and the CPU has a live Egg in slot 23. Pause and fresh HOME
restore both, and the SRAM re-export matches byte for byte.
The CPU destroys the base before the 2,100-update Carrier
queue finishes in that first build order.

An ordinary Outpost match on the same experimental ROM
(`2e8c4ff5fbc98733c90f6ae81abda4d056a2e813ff7ead88e7a2c34122dcf844`)
reaches a completed Carrier and a live CPU Egg at frame 18,763
with early gas, two Cannons and a reserve Pylon. Fresh HOME restores
both. Paying 25 minerals at frame 18,856 starts training an
Interceptor. The Carrier retreats while training and then attack-moves.
At frame 19,523 the Interceptor in slot 0 is outside Carrier slot 2
and targeting enemy slot 33, while the CPU Egg in slot 5 has 91 updates
remaining. A new HOME process restores both, re-exports byte-identical
32 KiB SRAM, and advances 50 updates while the Interceptor moves and
the Egg timer reaches 41. The combined journal still needs normal
cartridge and predecessor-save qualification.

| Local evidence | SHA-256 |
| --- | --- |
| Native 23-sample observation | `ef063506548d4ed9f99cca89ecae47efb0ec2ed4221bd1c1545796d97d5f1b58` |
| Native continuation receipt | `bb5377638ce012f44b4edd2683a89021a320e46dfae38ce59dc45c0a368bf0eb` |
| Current normal GBA pause and Drone completion | `c7528a65be48c9cdc741e812ec9ff9028b96b8e8663935a8756783f1f0fd5453` |
| Experimental ARM CPU Egg fresh HOME receipt | `218543fe0fee183acbd56de32757aed5b6f2c63655348cadc1963ba24c225204` |
| Experimental ordinary Protoss Stargate journal | `f5b6785c0db2a60b884ae8198d7d91f10706606cc5fd468f7ae14ec007ae1105` |
| Experimental ordinary ARM Carrier queue plus live CPU Egg journal | `69a356babed42f59d69401772664e7106d04854cb150f9ac9ad5f12eb5b715bc` |
| Experimental Outpost live Carrier plus Egg journal | `0d4c0eae4388ee64d8c6b658910bf4137a3efe753d7ea8bcf835b883243f38f1` |
| Experimental Outpost launched Interceptor plus Egg journal and fresh export | `7b813fa0514efca919dfee61e2370310d72954713bc7098fe70514f67e3af310` |
| Interceptor launch and pause observation | `c1a54307248ee894f596a88a8eee9c4437da82290a8c6ff233b7080964a0df12` |
| Interceptor and Egg fresh HOME observation | `0942e81b3484cedd7ca4a0caad38afa5fae2e790074475a65f05a1a5e764e63d` |

## Custom Game implementation dependencies

The released catalog specifies a Hydralisk producer, Lurker Egg and a
Hydralisk Den research requirement. The Custom Game train and research
paths currently have no matching unit-morph command. The optional
`U_SK_LURKER` identity also needs a shared live combat path before
exposing it to players or computers.

Implement separate Den-bound research for each owner, preserving any
concurrent Lair or Hive research. Qualify the same-slot egg transition,
cancel and producer loss, supply and payment, and fresh journal reload
before exposing it to the three independent AI seats. The current Z10
worst-case payload leaves 83 bytes inside its slot. Check the real
maximum across every envelope and add a predecessor-reader test before
changing the format. The next native cut should isolate a completed
Lurker's burrow and spine attack with multiple targets along its path.
