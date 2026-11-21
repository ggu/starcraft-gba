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
