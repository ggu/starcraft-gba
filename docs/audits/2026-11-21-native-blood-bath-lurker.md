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

The GBA paid pair trainer reserves a second actor slot at admission.
For comparison, capture an ordinary Custom Game Egg before completion,
then both newborn actors and owner resources after completion. Pause
mid-egg and repeat from fresh HOME. Repeat with actor slots nearly
full to verify that the pair reservation and wallet agree.

| Follow-on receipt | SHA-256 |
| --- | --- |
| Frame-12,511 input checkpoint | `11b71a1ff0dde5be696861df064cc93bcd33ee9a5ed571844186569a03aa74f0` |
| Live samples | `4d91d7452672d8289401d44da2caa677e5ad052b70379d29eb41ba22a993eac5` |
| Final native state | `4da3a3139006c693a2db4cd35ec215a5d8f6cf42ec80f0651128e89e3c8e7c11` |
| Observer receipt | `1d2a07d377c445214f5a7fd1da90ab7c9c42d8fdcc2ea836a4aed13d40689ec8` |

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
