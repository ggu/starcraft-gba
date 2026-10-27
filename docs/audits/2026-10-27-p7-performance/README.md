# P7 development performance investigation

This is a measurement of the unpromoted P7 development profile, not a new
qualified campaign mission. Canonical progress remains 22/30.

## Workload and limits

The engine source is the main-branch game.c SHA-256
`3d0a26b9da4a5efd8abb2897c7ba993d7e9dbb8fcbf210fafd951779ff96b4b8`.
The profile includes P7 projectiles, Carrier training and Interceptor flight,
plus prevalidated triggers, count snapshots, typed targeting, Arbiter cloak
cache, geometry hoisting, collision class cache and steering list. Merely
enabling Carrier features does not establish that this replay exercises them:
the authenticated human opening has no Carrier or Stargate.

A fixed ordinary-input replay enters P7 from the menus, advances 600 opening
updates, selects a ground attack-move destination at (584,216), then advances
1,400 attack updates. It ends at frame 2,099 in PLAY with 14 human units,
21 human buildings and no mission fault. The replay makes no synthetic
world-state writes; orders change only through ordinary input. Twenty
explicit actor/runtime state snapshots plus the terminal state match host and ARM in each measured
variant. These checks are bounded; they are not a full campaign win.

The private staged CHK matches map 008/024, SHA-256
`7c566d8b5e9d851be407dc1ac5438bebf32a8ff1d66a3d1b784cffd66f19e3a7`.
Historical extracted sprite banks were reused read-only; their original
regeneration recipe was not newly qualified by these measurements. ROMs,
input tapes, extracted data, captures and generated headers remain private.

## Uninstrumented baseline and candidates

| Variant | Opening mean full cycles | Attack mean full cycles | Decision |
| --- | ---: | ---: | --- |
| Baseline | 2,224,608 | 2,698,016 | Reference |
| Packed collision hit cache | 2,214,309 | 2,670,730 | Defer P7 activation; about 0.46% / 1.01% gain |
| Fact tables | 2,211,500 | 2,681,765 | Defer activation; about 0.59% / 0.60% gain |
| Frequency-based route eviction | 2,224,609 | 2,698,418 | Reject activation; no useful gain |
| Lazy cannon-power preparation | 2,225,077 | 2,701,227 | Reject activation; slightly slower |

The candidates were tested separately, not as a combined optimization. Fact
tables additionally save 5,132 linked ROM-load-image bytes and 464 IWRAM bytes in this profile.
The baseline linked ROM load image is 20,141,832 bytes; the finalized
ROM files remain padded to 32 MiB. BSS ends at `0x0203d6e4`, and IWRAM at
`0x03004ae8`. The baseline and route-eviction pair retain 9,572 bytes of
measured stack margin. Every measured variant passes the 32 MiB ROM,
`0x0203e000` BSS and `0x03005000` IWRAM limits.

At the GBA clock, the baseline cycle counts correspond to roughly 7.5 and
6.2 updates with rendering per second. This is cycle-budget arithmetic,
not a physical-device frame-rate measurement. The workload remains far
outside the roughly 559,241-cycle budget for 30 updates per second.

## Attribution

A broad-stage probe identifies orders as the largest update stage: about
1,167,060 opening and 1,558,083 attack cycles per update. The parent stage
also includes mission-specific post-order work, so it must not be equated
with update_orders alone.

A second private probe uses identical instrumented code in control and
sampled builds, with a runtime sampling switch. Both keep move_toward and
nearest_enemy_typed in IWRAM at the same addresses, and both match the
20 snapshots and terminal state. Common instrumentation adds about 0.79%
opening / 0.55% attack full-cycle cost; turning on sparse timers adds about
0.083% / 0.052% relative to the instrumented control.

| Sampled component | Opening (17 updates) | Attack (42 updates) |
| --- | ---: | ---: |
| update_orders inclusive | 1,201,167 | 1,654,873 |
| Setup | 528,509 | 782,574 |
| nearest_enemy_typed | 96,739 | 91,070 |
| Movement, excluding nested typed queries | 412,150 | 626,962 |
| Residual | 163,768 | 154,265 |

The sample selector uses frame modulo 67 equal to 0 or 37. Periodic work
can correlate with this deterministic selector. These are instrumented
sample means, not all-frame component averages. The query row covers only
nearest_enemy_typed, not every nearest-target or resource helper. Partitions
sum to the inclusive time within rounding error. Both diagnostic images end
BSS at `0x0203d77c`, IWRAM at `0x03004c68`, and retain 9,492 bytes of measured
stack margin.

A follow-up probe splits setup on the same sampled updates. It retains
identical host/ARM snapshots and terminal state in control and sample builds,
with unchanged hot-function placement. The mean setup components are:

| Setup component | Opening (17 updates) | Attack (42 updates) |
| --- | ---: | ---: |
| Buckets, excluding cache clear | 37,517 | 34,492 |
| Collision class-cache clear | 3,465 | 3,460 |
| Target lists | 21,839 | 21,810 |
| Target index end/begin maintenance | 85,501 | 83,278 |
| Arbiter processing | 379,308 | 638,609 |
| Hostility snapshot | 2,800 | 2,803 |
| Setup timing-boundary residual | 679 | 679 |
| Setup inclusive | 531,112 | 785,134 |

Arbiter processing performed 13 actual refreshes in the opening sample and
34 in the attack sample. It dominates sampled setup; cache clearing does
not. The next candidate should reduce repeated work within this refresh
while preserving every visibility, ownership, detection and timer rule.
These remain sparse instrumented samples, with the same periodic-correlation
caveat. This probe ends BSS at `0x0203d7c0`, IWRAM at `0x03004c68`, and retains
9,468 bytes of measured stack margin. Reducing simulation cadence or changing
gameplay semantics is not justified by these measurements.

## Optional sight-distance cull

`SC64_P7_SIGHT_AXIS_CULL` skips a sight-radius lookup only when an actor is
outside the conservative maximum-radius square. The current roster and
fallbacks are bounded by 176 GBA pixels for units and 160 for buildings.
Actors on the boundary still reach the original inclusive circle test.
Alive, owner, loaded-unit, hangar and construction-sight rules are unchanged.
The bounds fixture enumerates the actual radius helper over every runtime
type and invalid/extreme type IDs, checks radius/boundary/corner geometry,
and exercises the actual owner-visibility query on synthetic single actors.
Positions in this differential fixture are valid map coordinates.

After preparing private inputs, the opt-in checks and builds are:

```sh
make test-sc64-p7-sight-axis-bounds
make test-sc64-p7-sight-axis-normal-input
make build/sc64-p7-normal-input-replay.elf
make build/sc64-p7-sight-axis-normal-input.elf
make protoss-p7-sight-axis-probe
```

The last command builds an internal gameplay probe. Existing default and
canonical profiles do not enable the new gate. The disabled branches retain
the original evaluation order. The gate requires P7 actor support.

The final public-source ARM benchmark matches all 20 snapshots and terminal
state. Its ROM is byte-identical to the reviewed candidate. Compared against
the hardened ordinary-input reference harness:

| Full mean cycles | Reference | Sight cull | Reduction |
| --- | ---: | ---: | ---: |
| Opening | 2,224,609 | 2,168,075 | 2.54% |
| Attack | 2,698,019 | 2,614,352 | 3.10% |

Both retain 9,572 bytes of measured stack margin, BSS end `0x0203d6e4` and
IWRAM end `0x03004ae8`. The separate gameplay probe links a 20,128,124-byte
load image, with BSS end `0x0203deec` and IWRAM end `0x03004ae8`; its final
ROM is padded to 32 MiB. Its gameplay stack is not qualified by the benchmark
stack measurement. The smaller cycle count remains far outside the 30-fps
budget; there is no new campaign-win, Carrier-flight or physical-GBA claim.

## Evidence retention

This audit records a frozen local investigation. An opt-in authored replay
is now available after preparing the authenticated private inputs described
in `docs/DEVELOPMENT.md`:

```sh
make test-sc64-p7-normal-input-replay
make build/sc64-p7-normal-input-replay.elf
```

The first command runs the ordinary-input replay under host sanitizers and
prints 20 semantic snapshots and a terminal state. Host cycle counters are
zero; use the ARM ELF for timing. It defines its own main and excludes the
normal game entry point. It adds no default build or test cost. The probe
uses the full P7 Interceptor-flight flags; enabling them does not mean the
opening exercises Carrier production or flight. Compare identical workloads
and inputs when evaluating an optimization. This is a bounded development
probe, not a mission-win or performance qualification target.

The newly published source harness does not establish regeneration parity
for the historical asset pack or replace the frozen evidence below.
The private evidence directories below contain explicit compiler argv,
postlink guards, host goldens, owned-process launch records and ARM results:

- `.local/build/p7-gen43-packed-hit-20261027/root-arm-reviewed-v2/`
- `.local/build/p7-gen43-packed-hit-20261027/root-stage-profile-v3/`
- `.local/build/p7-gen43-fact-tables-20261027/root-arm-reviewed/`
- `.local/build/p7-gen43-route-frequency-20261027/`
- `.local/build/z7-live-20261027/p7-arbiter-lazy-powered-v3/root-reviewed/`
- `.local/build/p7-gen43-orders-sampled-20261027-v2/`
- `.local/build/p7-gen43-orders-sampled-20261027-v3/root-reviewed/`
- `.local/build/p7-sight-axis-root-reviewed-20261027/`
- `.local/build/p7-sight-axis-public-root-final-20261027/`

These are ignored local paths, not downloadable artifacts. Do not regenerate
shared banks to reproduce this audit. Another machine must first prepare
its own authenticated private inputs through the documented importer flow;
these numbers alone do not establish parity for a newly generated pack.

All compilation, ASan/UBSan checks, emulation and review ran locally. No
GitHub Actions workflow was enabled or dispatched.
