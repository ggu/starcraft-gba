# Z7 trigger counts collected during live-actor validation

The prevalidated Zerg 7 tick walks the actor pools once to prove every
live actor's source provenance (`race_z7_validate_live_world`) and then,
for the triggers that have not started, walks them again to count the
five source predicates (`sc64_z7_count_snapshot_begin_masked`). Both
walks see the same actors with no writes between them.

## Measurement on the base Z10 ROM

The carried Z7 controller trace is regenerated from `origin/codex/source-release`
`2655cb22`. The ARM replay on the unmodified Z10 product ROM matches all
131 historical checkpoints through call 48,237, so the audited dense window
(calls 47,758-48,237, 476 advancing updates) is reproduced. An instruction-level
profile with a shadow call stack gives, per advancing update:

| Scope | Guest cycles | Share of mean update (2,482,596) |
| --- | ---: | ---: |
| Trigger-count pass (`begin_masked`), inclusive | 118,176 | 4.76% |
| of which its own instructions | 105,184 | 4.24% |
| Provenance validation | 24,789 | 1.00% |
| Paid-product reconcile | 9,719 | 0.39% |
| `race_campaign_tick`, inclusive | 166,368 | 6.70% |

The count pass runs once on every update and scans 240 unit and 160 building
slots; 236.8 units and 58.0 buildings are alive, and about 57 actors need a
rectangle test. In the whole window `started` is 0x3a, so the needed mask is 27
(loss check, hostile buildings, and the two force-0 region probes). The existing
default-off `SC64_Z7_VALIDATED_CENSUS` only handled mask 3, so it did nothing here.
Over the whole carried run the needed masks are 27 on 42.6% of updates, 11 on
42.6%, 3 on 9.8% and 31 on 5%.

The base mean (2.48M cycles) is higher than the 2.0M of earlier audits because
those ROMs carried extra routing options this ROM does not.

## Change

`starcraft/sc64_z7_census.h` arms a per-tick table before validation. Each
validated actor adds a table lookup; only actors that can contribute (or can make
the separate pass fail) are examined further. The same row, region, raw-id,
unit-type and rectangle checks apply. Whenever the separate pass would return 0,
the fused pass publishes nothing and also returns 0, so the caller falls back as
before. Masks 0 and 31 keep their existing paths. Mask 3 keeps its existing
two-counter accumulation, now on by default. `SC64_Z7_SEPARATE_CENSUS` selects
the old separate pass; `SC64_Z7_CENSUS_VERIFY` (HOST) runs both on every
update and aborts on any difference. Save layout and `RaceSave` (16,296 bytes on
both ROMs) are unchanged.

The extra 64 bytes of `.bss` (`__bss_end` 0x02038900 to 0x02038940 on Z10) are
the 62-byte `Sc64Z7Census` scratch (flags, counts and region bounds). It is
rebuilt by `sc64_z7_census_arm` every tick, consumed by one snapshot and never
saved.

## Evidence

Paired ARM replay (same inputs, base and candidate Z10 ROMs), dense window:

| Cycles per advancing update | Base | Fused | Change |
| --- | ---: | ---: | ---: |
| Mean | 2,483,053 | 2,412,859 | -2.83% |
| p95 | 3,637,726 | 3,567,014 | -1.94% |
| Slowest | 4,841,711 | 4,760,825 | -1.67% |

Validation grows from 24.8k to 70.1k cycles; the old count pass (118k) is gone;
`race_campaign_tick` falls from 166.4k to 99.4k. Frame, screen, `started`, live
actor counts, route counters, all 131 checkpoints and 21 sampled hashes (actor
banks, mission state, palette, both display pages, DISPCNT) are identical.

Whole mission (197,455 updates, 253 checkpoints): every update's Z7 persistent
state bytes (all trigger started bits, cursors, timers, outcome) is identical on
base and candidate, so every trigger changes on the same update through the win;
395 sampled world/display hashes are identical. Mean update cycles 1,454,754 to
1,406,754 (-3.30%).

HOST: the verify build compared both ways on every update of the carried Z10
(194,998 updates; 166,158 geometry checks, 19,164 two-counter), Z9-vector and
standalone Z7 controllers with no difference. Fused (non-verify) controllers give
byte-identical traces to base for Z7 on Z10, Z7 on Z9, standalone Z7, Z8 and Z9
(Z7 won at frame 194,998/184,248; Z8 frame 45,468; Z9 frame 72,628).
`make test-sc64-z7-validated-census test-sc64-z7-prune-started
test-sc64-z7-count-snapshot` pass (fused, verify and separate fixtures; 1,160
fused comparisons including 23 refusal cases).

Save: a save exported at update 47,757 (dense window start) is byte-identical
(SHA-256 `98d496bd...c5fd`) from the base and candidate ROMs, so a base-written
save is the same file the candidate writes. Fresh-process resume to update 48,237
passes on both ROMs with identical restored state and identical sampled model
fields. The replay tool binds each saved summary to its ROM hash, so a
cross-ROM resume is refused by design; the identical bytes make it equivalent.

## Memory, links and suites

`__iwram_end` / `__bss_end` after the change (all link within the guards
`__iwram_end < 0x03005000`, `__bss_end < 0x0203e000`): Z7 0x030048d8 /
0x02039874, Z8 0x03004c88 / 0x02039d34, Z9 0x03004c88 / 0x02038938, Z10
0x03004ce8 / 0x02038940. Each ROM that compiles the adapter holds the 62-byte
`sc64_z7_census` (Z10 IWRAM end is unchanged; Z10 `.bss` end +64). Retired Z7
and Z8 take it with room to spare, so no ROM needed the separate pass. Z6,
P7, P8, P9, P10, P3, Terran, Z3 and Z4 do not compile the adapter and link
(`verify-protoss-p6` cannot build here: a private input is absent).

`./dev build` passes. `./dev test` has two failures in
`test_sc64_photon_cannon_renderer` ("missing sprite: unit raw 85") and
`make test-unified` aborts at the gas-worker carry assertion in
`race_engine.c:255`; both also fail in a separate checkout's earlier base logs
and touch no Z7 code (not rerun at this exact SHA). `make unified` and
`make test-sc64-zerg-z10` pass (Z7 won at frame 194,998, Z8 45,468, Z9 72,628).

The base then moved to `2d319e76` (Makefile and race_skirmish_ai.h, not the Z7
adapters). It merged with no conflicts. After the merge `check_source_tree`
passes, the Z10 ROM links with the same memory ends, and the HOST verify build
on the carried Z10 controller repeats its 166,158 geometry and 19,164
two-counter checks with no difference and a trace identical to base's.

## Limits

Emulator cycles, not hardware; one dense window plus one full run. A
base-written save was not loaded on the candidate: the replay tool binds each
save summary to its ROM hash and refuses it, so the evidence is that the two
saves are byte-identical. The fused hooks still cost about 45k cycles per
update (about 9.5k per-actor gate and 13k for the force-0 rectangle path); no
further exact saving was obvious. The trial merge with
`origin/codex/sc64-z7-negative-route-20261119` has no textual conflicts but was
not built or run.
