# Blood Bath opening and steady income

The owned USA N64 ROM ran the matched Blood Bath opening again in
ares. It resumed the authenticated step-zero match with northwest
Terran at (128, 240) and southwest Zerg at (288, 1808). The human
gives no gameplay orders. The pause-and-snapshot observations use the
native game update counter.

The Terran wallet has 50 minerals at native update 169 and 58 at
update 183. The first credit therefore arrives in (169, 183], or
roughly (211, 229] at the nominal GBA update ratio. The native wallet
grows from 98 to 266 across updates 362–1,268. The Terran still has
the same four SCVs at both endpoints. This interval compares income
after first delivery.

Four normal-format Terran ROMs repeat the same seat positions through
ordinary Custom Game controls and 1,600 game updates. All link with
38 identical prepared input hashes and the same IWRAM and EWRAM
endpoints. Both flags off reproduces the `e92caada` Terran ROM byte
for byte.

| Trial | First eight-mineral credit in GBA updates | Wallet at the matched endpoints | Gain |
| --- | ---: | --- | ---: |
| USA N64 | approximately (211, 229] at nominal scale | 98 → 266 | 168 |
| Regular GBA | 75 | 170 → 482 | 312 |
| GBA 102-update wait | 131 | 122 → 338 | 216 |
| GBA Terran opening formation | 141 | 154 → 458 | 304 |
| GBA formation and wait | 198 | 114 → 314 | 200 |

The matched GBA endpoints are the floor and ceiling of native update
362 × 5/4 and update 1,268 × 5/4. The wallet is the same at both ends
of each GBA bracket. This compares game ticks in the two emulators.

The formation shifts first delivery by 66 updates in isolation and
changes the later gain by eight minerals. The wait accounts for most
of the steady gap. The combined trial saves at frame 121 during an
active gather wait, reloads from HOME in a fresh mGBA process, and
matches the following 240 updates and re-exported SRAM.

The combined opening still reaches the first deposit before the observed
native bracket and gains 32 more minerals over the later interval.
Keep both experiments opt-in. Next compare patch occupancy and
the return-to-depot contact points, including the SCV that reassigns
from the temporarily shared patch. Repeat with Protoss and Zerg and
check AI spending and campaign timings before normal activation.

| Local receipt | SHA-256 |
| --- | --- |
| Native step-zero seed | `ed9225feb4caa29de200475af24c435c85acddabdc2c9e352857bc8732f30fcd` |
| Native worker and wallet samples | `a49e4f5196c26896916b5e1646eb4286c147a2fe528c11a9f8eea8c45fc43eb3` |
| Four-variant comparison and active-wait continuation | `bc2d3493e9c87bbc62a7ea3f2186b76c1b2caea3156ffcfff98490978eb68dd4` |

See the prior [native worker opening and contact audit](2026-11-21-blood-bath-worker-opening.md)
and [mineral wait experiment](https://github.com/ggu/starcraft-gba/pull/643)
for the observed worker order and probe switches.
