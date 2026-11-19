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

## Limits

Emulator cycles, not hardware; one dense window plus one full run. The census
scratch hooks still cost about 45k cycles per update (about 9.5k per-actor gate
and 13k for the force-0 rectangle path); no further exact saving was obvious.
Not yet run on this branch: `__iwram_end`/`__bss_end` on the other Zerg ROMs,
the retired `verify-*` links, `./dev build`, `./dev test`, unified and the full
`test-sc64-zerg-z10` suite (disk fell below the agreed floor). A trial merge with
`origin/codex/sc64-z7-negative-route-20261119` has no textual conflicts.
