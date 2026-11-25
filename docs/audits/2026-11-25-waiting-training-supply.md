# Waiting training orders at the local supply cap

## Stock Nintendo 64

The US N64 Wasteland replay uses the normal Command Center buttons at
18/18 supply. An idle Command Center with 145 minerals refuses its
first SCV, retains the bank and shows the supply warning. With one
SCV training, the same control charges for a waiting SCV. When the
first SCV completes, the waiting SCV becomes the head and holds an
empty progress bar at 18/18. Further ordinary input charges for an
additional order while that head is stalled.

| Native input tape | SHA-256 |
| --- | --- |
| Idle refusal | `49c0b71c7de872017c266b844062c323956e3937dd916a94adeb995b848078ab` |
| Paid waiting, stalled head and additional order | `a7ffb21f14d8ff29af91e4468ab0ac0cd9c0647d47bf287164b1d591f2b019f6` |

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
