# Blood Bath: mixed AI lobby and opening economy

The owned StarCraft 64 ROM reached **Melee → Blood Bath** through ordinary
controller input on the pinned ares visual observer. The map card reads
64×64 and four starting slots. The lobby shows human Terran, then computer
Zerg, Terran and Protoss. This checks the same map offered by the GBA port;
its other map, Outpost, is an authored map and was not compared here.

An initial map navigation run captured Meltdown, Blood Bath and Spread Out
on consecutive Right presses. From the established starting map row, 11
Right presses select Blood Bath. A second clean run captured that lobby,
pressed Start, then held neutral throughout both gameplay captures.

| N64 capture | Observation |
| --- | --- |
| Lobby, observer step 3,244 | Blood Bath, Melee, four slots; human Terran, computer Zerg/Terran/Protoss |
| Field, step 3,754 | Terran Command Center, 50 minerals, 4/10 supply |
| Field, step 4,254 | Terran Command Center and workers, 66 minerals, 4/10 supply |

The normal Z10 GBA ROM launched the same map and player choices through
physical buttons. At game frame 1 it reported 50 minerals, zero gas and
four workers with 4/10 supply. It had one base for each of the four owners,
four workers each, and the Zerg AI's starting Overlord. The three AI wallets
each contained 50 minerals. With neutral input throughout, the human's
mineral readings at frames 61, 121, 181 and 241 were 50, 82, 90 and 114;
supply stayed 4/10. The runner used named read-only state. It did not write
guest memory. GBA ROM SHA-256:
`bb286c9c4fe3c70c12b0dec7abb298ba24576f2213dd5dbf8fcb3b692dc70412`.

Both games start Terran at the same displayed minerals and supply and accrue
minerals without a player mining order. N64 observer steps and GBA game
frames are different measurements, so this does not establish matching
mining rates. The N64 captures do not show the three AI bases, their internal
wallets, or a match outcome. In particular, the GBA AI balances are not
evidence for the N64 balances.

The visual-only observer used CPU/RSP recompilers, no RDRAM capture or guest
writes, and the existing disk, output and wall limits. Both runs returned
zero, verified their command ledgers and three requested frames, and retained
unchanged inputs. The second plan SHA-256 is
`1b348844c37c9672d66c67a9e069048c973fa1d73e4f81fb2fa2291fe08ed23c`;
its three raw frame SHA-256 values are:

- `6258ce9e5df4470cab221f2a37417469e457a4f8e8ba43ec0787edee16628c77`
- `f714c0433d55a26d0da9df671344a292f1a222c1876a7b733e7de67143b5316a`
- `ac57b7c8d8612acbd4701e52e7311468a19c051a26892ecadd098c6716c3f58b`

The ROM, input tape, screenshots and runner outputs remain private. See the
[earlier Challenger observation](2026-11-07-n64-visual-only-jit/README.md)
for the same observer's provenance and limits.
