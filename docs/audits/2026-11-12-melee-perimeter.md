# Melee reach on source-bounded structures

The center approach could stall because a building's footprint covered every
seed tile. In [PR #386](https://github.com/ggu/starcraft-gba/pull/386), short-range ground attacks can connect from the edge
of the blocked tiles. A failed center route falls back to the walkable sides.
The field shares the existing route pool and topology invalidation; its key
cannot alias a worker depot field. This is a correction to the GBA collision
grid, without a claim of exact N64 weapon reach.

- The original mirrored Outpost match (Terran vs three Zerg AIs) on the Z7
  canonical HOST fixture is unresolved at update 216,001 on base `6028da68`
  with the Machine Shop still standing. The fallback runtime `ecaa865` ends
  with a Terran defeat at 34,091 and no Terran structures remaining.
- On the fallback runtime, all 640 Z7 sanitizer approaches pass: two melee
  unit types, five structure types, four sides, and all 16 tile alignments.
- The old ordinary-input Z8 controller wins on base at frame 44,150 and loses
  on the candidate at 34,840. In the losing run a cloaked raider had 16 HP
  just before the southern defeat trigger. The updated controller intercepts
  farther up the road, resumes clearing the southern mineral fields when
  interrupted, and keeps a cohesive group on the western route. No campaign
  script, game rule, mission state or save format changes were needed.
- Controller `4fc3195` wins at frame 45,468 on the carried Z9 build, after
  46,487 calls; both Nexuses destroyed, Kerrigan at 400 HP, zero faults.
  The retained tape begins at the title, saves through the pause menu at
  frame 16,660, and resumes at call 17,123. The fresh HOST process agrees
  with the uninterrupted run at all 30 continuation checkpoints and ends
  with digest 2533489598.

## ARM replay and ROM checks

The carried Z9 replay probes use the production Z9 configuration, with the
ordinary-input tape embedded in the test entry point. In mGBA the full run
wins at frame 45,468, at the same checkpoint digest as HOST. The prefix
writes a 32 KiB save at call 17,123. A fresh mGBA process loads that save and
reaches the same win: all 30 comparable continuation checkpoints match the
uninterrupted ARM run, and all 31 (including the resume observation) match
the fresh HOST run. The strict comparator used the actual cutoff, 17,123.

All nine normal ARM links pass the ROM and memory reserve checks at
`6ba59b1`: default, unified, Terran, Zerg Z7/Z8/Z9/Z10, Protoss P10, and
Showcase. The replay evidence is for one campaign path and save point;
it does not establish arbitrary saves or timing on physical hardware.

On the same source, the carried Z1–Z8 controller suite, Z9/Z10 objective
and save fixtures, full Z10 (58,795) and P10 (52,969) HOST wins, P10
Gantrithor lifecycle and continuation, and the default depot return test
pass. The old Terran and Zerg depot skirmish fixtures both abort before
depot logic, at the Gateway placement assertion, on the candidate and
the same configured `6028da68` control. This is a separate fixture issue;
those optional depot tests are not reported as passes.

## Paired Z9 ARM battle window

We replayed the same pinned physical-button tape on verified Z9 ROMs at
`6028da68` and `ecaa865`. The 1,000-update window is ticks 10,860–11,859
of the prior busy-combat Zerg workload. Both runs have zero sampled workload
mismatches against the reference. Samples are counts and frame/screen state;
they are not a full per-update equivalence proof.

| Loop cycles | Base | Fallback | Change |
| --- | ---: | ---: | ---: |
| Mean | 1,414,608 | 1,416,948 | +0.17% |
| p95 | 1,913,978 | 1,916,907 | +0.15% |
| Peak | 3,803,516 | 3,834,506 | +0.81% |

The base ROM SHA-256 is
`2f8641c1ef07705ce86d3d16d44acffa91f895417a829a633f44661123e7f6e2`;
the fallback ROM SHA-256 is
`939e7d09ee2da3a8c81c72ada84a5545d04c629959b8072ce11bf65e7fc69ad7`.
Both use tape SHA-256
`13019d6073bbf365cae1833f6e9e161ba96d61f660e38f3bc948254073e792d2`.
The mean remains about 2.5 times the 561,792-cycle budget. This window says
nothing about cycle costs on real hardware or in every campaign fight.
