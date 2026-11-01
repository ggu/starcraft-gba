# Current-release range-routing integration

This reconciles PR #134 with release `7181c093`. The branch enables shared
bounded range routing by default starting at `4072d735`, with the qualification
and limits recorded below. Earlier disabled stages remain identified by revision.
The earlier audit in
`../2026-10-29-range-work/README.md` describes historical revisions, not this
integration's performance or campaign qualification.

The current adapter preserves separately tagged range snapshots and in-progress
copies when ordinary point and resource-depot fields are looked up or evicted.
It retires target-keyed work before actor-slot reuse and checks target liveness
before pumping queued work. Successful current-format save publication resets
derived work; failed publication does not. Unit and persistent payloads do not
grow. Packed job metadata keeps the existing actor, map and memory limits.

Before requesting a new range field, the opt-in movement path can read an
already-ready ordinary point field. This lookup neither starts nor resumes a
flood. Partial fields require a discovered origin or a usable adjacent exit;
the next waypoint must respect the weapon's minimum range. Direct/corner
firing-point search and bounded range work handle remaining approaches.

## Revision-specific evidence

- `ac5bf43a`: actual-engine sanitizer corridor, scheduler, reuse, dead-target,
  resource-depot isolation and warm complete/partial point-field fixtures
  passed. The actual-codec save continuation matched 500 game updates.
- `ac5bf43a`: Z7, Z9 and Z10 opt-in ARM links passed their unchanged linker
  guards. Z9/Z10 end BSS at `0x0203dff8`, eight bytes below the strict guard;
  IWRAM ends at `0x03004bf0`. This narrow margin is a constraint on future work.
- The same wider link check exposed a missing `target_alive` forward
  declaration in T10, P7 and Showcase. Revision `977399c9` moves that declaration
  outside the direct-attack flag, and all three then link. Their BSS/IWRAM ends
  are respectively `0x02037150/0x03004b40`, `0x0203838c/0x03004a28` and
  `0x02037080/0x03004cc0`. No guard, stack reserve or actor limit was relaxed.
- `223823bb` restores the documented default-quota engine test target. Its
  runtime source is identical to `977399c9`; only the test Make fragment differs.

All these checks used local tooling, isolated output directories, frozen source
and authenticated existing prepared inputs. No asset generators or hosted
Actions were run for these checks. Native checks were serialized with the local
qualification lock. No game data, binaries, saves or captures are committed.

## Early activation hold (before the following qualification)

ARM link success and host sanitizer checks do not establish whole-frame speed,
acceptable navigation latency, full mission completion, fresh-process SRAM
continuity or hardware behavior. In particular, a work-unit quota is not a
cycle budget. Paired gameplay latency and ARM timing must exercise blocked,
unreachable, moving and competing targets, along with ordinary economy work.
At that early checkpoint the flag remained disabled pending evidence across
campaign, custom-game and Showcase builds. Later qualification follows below.

## Full source checks and latency decision

At `977399c9`, an isolated unprepared `./dev build` passed. `./dev test`
completed its C suites and 951 Python cases (26 optional skips), with one
failure: the flag-print diagnostic was misclassified as an asset consumer.
Revision `b703699e` removes that tracked diagnostic recipe and uses a temporary
Make fixture for flag inspection. All four lookup dependency tests, the flag
inspection and the source-boundary check then pass. The existing dependency
test remains unchanged; no private dependency is generated just to print flags.

At `223823bb`, the default-96-quota actual-engine sanitizer fixture passes.
Both ordinary and active-stride copy oracles pass all 8,320 alignment, length
and offset combinations each. These supplement the earlier small-quota engine
and save checks; they do not replace product-mode or ARM SRAM qualification.

The authored `tests/sc64_range_latency_probe.c` runs the real engine under
host ASan/UBSan with the canonical Z7 vector, once OFF and once ON at quota 96.
It scripts target coordinates, checks gameplay/frame and wall continuity,
verifies point-route availability and blocked direct firing, and clears all
warmed route state before measuring 1,000 updates. On engine `ac5bf43a`:

| Scripted scene | First movement OFF / ON | First hit OFF / ON |
| --- | --- | --- |
| Reachable gap beyond a wall | 1 / 43 | 384 / 431 |
| Sealed wall | no movement / no movement | no hit / no hit |

The reachable scene exercises 4,301 worker units across 45 updates with the
feature ON. That cold-route hesitation blocked activation at this revision; the bounded-corridor follow-up below addresses these measured cases.
These are game-update counts, not measured seconds or hardware FPS. The
separate two-attacker moving-target observation does not prove fairness:
order/target continuity was not independently asserted and one attacker never
hit in either variant. Earlier invalid scene iterations are excluded.

The verified host probe is checked in for further diagnosis. At this checkpoint, ARM harness drafts were local and unvalidated; the follow-up below records the subsequently executed guest harness.
The declaration and test-helper fixes after `ac5bf43a` do not change the
measured Z7 runtime. No release flag is enabled by this integration.

## Bounded cold-corridor follow-up

Production runtime `3c8bee1b` (identical to the `791bbd56` candidate) adds a
shared 160-walkability-inspection budget per game update. The new witness
reuses the existing exact firing-band candidates and proves an axis-aligned
L or two-turn corridor. It returns only its first leg. It does not flood a
whole map, grow an actor or save row, relax memory limits, or refill its
budget on topology/save resets. The common update hook replenishes the budget
once before order processing. Existing candidate enumeration/direct approaches
and the separate 96-unit range worker are outside this new witness budget;
160 is not a bound on total engine work or cycles.

The witness guards the intended first leg against the weapon's inner radius.
This is a first-leg geometry guarantee, not a proof covering every pre-existing
cached shortcut or dynamic contact-steering step. Complex routes, exhausted
budgets and unproven corridors continue to use the bounded range worker.

The host probe now checks original attacker order/target and life through the
first hit. Its separate building contention case issues two attack orders once,
uses stationary targets with natural scout sight, and never repairs orders.
The stronger baseline at `14394b92` measured ON first movement 29/99 and first
hits 305/378 for those building attacks. With the final witness:

| Scene | First movement OFF / ON | First hit OFF / ON |
| --- | --- | --- |
| Original reachable unit target | 1 / 1 | 384 / 412 |
| First building target | 1 / 1 | 262 / 261 |
| Second building target | 1 / 1 | 261 / 261 |
| Sealed wall | no movement / no movement | no hit / no hit |

Both building orders remain intact through 1,000 updates. Their successful
witnesses avoid the queue entirely, so the probe no longer requires those
particular attacks to enqueue jobs. Separate scheduler contract fixtures retain
queue coverage. The original unit-target route still takes 28 more updates to
its first hit with the feature ON; first movement alone is not full latency
parity or a general path-optimality claim.

Test-only revision `7fa1990f` (same changes as `82689553`) adds an independent
pixel oracle: 4,096 seeded blocked-grid trials, 1,557 returned first legs,
both axes, off-center starts and 1,548 nonzero minimum radii. It independently
checks every pixel for bounds, walkability and inner-radius distance, and
compares spent budget with reported inspections. Another aggregate fixture
checks exactly 160 reads across repeated blocked goals and multiple origins.
The actual Transport1 save fixture matches 500 post-save game updates at
worker quota 8. Its attacker starts farther from the persisted map divider so
that the saved scene really contains a partially progressed worker beyond the
witness's reach; it does not disable the witness or inject synthetic terrain.

All six ON production ARM links pass at `3c8bee1b`: T10, Z7, P7, Showcase,
Z9 and Z10. Z9/Z10 still end BSS at `0x0203dff8` (8 bytes below the strict
guard); Z7's IWRAM guard margin is 128 bytes. Other normal profile margins are
larger. These are linker margins, not new reserves or proof of runtime safety.

## Executed ARM probe

`tests/sc64_range_arm_probe.c` is the exact guest-authored source executed with
production Z7 flags. It sets up a stationary building attack and a wall/gap
once, issues one attack order, then checks actor life, order, target, sight,
map continuity and frame counters each update. The emulator supplies zero
physical button input. The retained read-only backend rejects guest-memory
writes; no world injection, forged SRAM, save import or save export is used.

Paired OFF/ON builds use the same guest source and canonical compiler vector,
replacing only `starcraft/game.c` with that wrapper and assigning separate
ELF/map/ROM outputs. Source SHA-256 is
`6579901db49bf8797d3f4732a7b62f5025c5de60ebdbbd8604cde274700e2e27`.
The executed collector records every completed update, including the cold first
update; all 100 frames have zero probe faults and first movement at update 1.

| Input + update + audio + render cycles | OFF | ON |
| --- | ---: | ---: |
| Mean over 100 updates | 581,349.62 | 570,339.89 |
| Peak | 2,340,548 | 1,443,557 |
| 95th percentile | 561,533 | 561,531 |
| Updates above two GBA refresh frames | 2 | 1 |

The peak decreases 38.3% in this scene. Counters include the existing present
wait, so the approximately 561,500-cycle regular cadence is close to the
intended two-refresh-frame cadence (561,792 cycles, about 29.9 Hz), rather than
pure CPU work. The cold update still exceeds that budget. This short authored
scene has no crowded army and does not reach the first hit; it is neither
hardware validation nor a full-match or campaign near-30-fps result.

After building an instrumented pair with already prepared private inputs, the
checked-in read-only collector can inspect each ROM:

```sh
python3 tools/sc64_range_probe_read.py --runner /path/to/runner-read-only \
  --rom /path/to/probe.gba --elf /path/to/probe.elf \
  --output /path/to/new-results --ticks 100
```

Use the existing native qualification lock, frozen source and prepared-input
hash guards for local runs. Keep all game inputs, instrumented binaries and
results ignored. No hosted Actions or SD writes were used. At this checkpoint the shared release flag remained OFF. Subsequent startup, fresh-process and crowded-game qualification, including its remaining performance limits, follows below.

## Current full-source and lineup qualification

The clean, empty-input checks at `7fa1990f` pass: `./dev build`, `./dev test`,
flag wiring and the source boundary. Python discovery runs 951 cases with
26 optional skips; the additional 5- and 8-case suites and C fixtures also
pass. Later changes add the executed probe/reader, documentation and a tested
menu fixture; they do not change production runtime from `3c8bee1b`.

The checked-in reader at `2feec585` independently replays the same OFF/ON
instrumented pair for 100 updates each with physical zero-button input and
reproduces the exact cycle results above. Both fault counters stay zero.

`tests/sc64_range_menu_matrix_probe.c` runs against each actual current T10,
Z7, P7 and Showcase production compiler vector with the opt-in flag ON under
ASan/UBSan. The vectors are derived from those normal ARM build recipes,
retaining their feature/include flags and companion game sources rather than
using stale older-profile host targets. All four pass 234 real-menu cases:

- Both supported maps and all three human races.
- One AI with each of three races, two AIs with all nine independent race
  assignments, and three AIs with all 27 assignments.
- Navigation by ordinary key inputs, then live owner, unit, building and map
  shape checks followed by 60 gameplay updates.
- An ordinary same-process save/load/save, including configuration checks
  and byte-equivalent complete `RaceSave` reserialization.

That totals 936 lineups across the four builds. This is startup and immediate
same-process round-trip evidence. It does not compare another uninterrupted
60-update branch after each load, use a fresh process or prove complete
matches. The separate 500-update continuation fixture and short ARM timing
probe establish their narrower properties, not those broader claims.

## Fresh-process SRAM and crowded-game qualification

At `c44bdba5`, all four normal ON products (T10, Z7, P7 and Showcase) pass an
ordinary physical-button save/load check: Custom Game on map 0, human Terran
against one Zerg AI; pause with START, save with B at game frame 61, and export
the genuine 32 KiB SRAM. A fresh process loads it using HOME L. Named semantic
state, lobby, nine live units and three buildings match the save; 60 more game
updates reach frame 121 and match uninterrupted play. Loading does not change
the exported SRAM. These are selected persisted-field comparisons, not full
machine-state or hardware qualification. No debugger world writes or malformed
records are used.

The reusable `--physical` mode in `tools/sc64_skirmish_sram_continuation.py`
uses the existing read-only K/U backend and the game's normal UI. Its focused
16-case tests and public Z7 60+60 command pass; the four-product results above
were obtained with the preceding equivalent private driver.

The authored `tests/sc64_range_crowd_arm_probe.c` retains normal map terrain,
AI and unit stats, adds combatants once at boot, and never heals, reveals,
retargets or repairs orders per update. Each OFF/ON pair runs 500 physical
zero-input updates with matching initial-world hash and sampled per-update
population/movement/cooldown/health counters. Cooldown rises and aggregate
health decreases are combat proxies, not per-weapon hit proofs. The 48- and
96-actor scenes exercise combat; the 16-actor scene does not.

| Scene / actual production vector | OFF mean / p95 cycles | ON mean / p95 cycles |
| --- | ---: | ---: |
| 16 actors / Z7 | 573,591 / 561,533 | 573,573 / 561,533 |
| 48 actors / Z7 | 754,040 / 917,721 | 761,874 / 1,620,548 |
| 96 actors / Z7 | 1,158,029 / 1,916,355 | 1,151,242 / 1,759,066 |
| 48 actors / T10 | 704,994 / 842,585 | 704,965 / 842,601 |
| 48 actors / P7 | 872,799 / 1,123,159 | 872,769 / 1,123,170 |
| 48 actors / Showcase | 828,016 / 1,122,992 | 827,986 / 1,122,970 |

These `c44bdba5` measurements contradict any claim of near-30-fps crowded
play: both variants often exceed 561,792 cycles. The Z7 48-actor ON scene also
has two approximately million-cycle update regressions. Exact read-only
replays identify additional point-route builds during orders at ticks 323 and
329, with the range worker idle, no queued jobs and the full corridor budget.
Cache-slot key changes establish point fields specifically; no actor/caller is
inferred merely from the generic route-build counter.

## Lazy scratch ownership and incremental activation

`92fd123f` (integrated as `b805929e`) fixes the idle-cache regression. Ordinary
routes can use the entire existing pool while no range worker owns scratch.
Before a worker starts, metadata for every field overlapping the last two
maximum-sized planes is invalidated. A variable-stride field crossing the byte
boundary is invalidated too. The worker retains ownership through DONE leases
and snapshot copying; retirement releases slots without resurrecting their old
valid bits. Geometry configuration keeps the full stable field bases and does
not reset work when only ownership changes. No storage, actor or save row grows.

Exact-revision sanitizer checks cover fixed and active strides, five map
geometries, overlapping slots, tagged snapshots/copies, real dequeue/start,
full idle-cache reuse and release. An independent source review found no
reservation-lifecycle defect; generated ARM fit and timing remain separate
checks.

Replaying the identical 48-actor Z7 ON scene with this header removes the two
observed extra floods. Update cycles at 323/329 become 310,158/310,804, compared
with OFF 308,589/309,093 and old ON 1,290,839/1,301,669. The cache build count
stays 37 at those ticks. All 500 sampled gameplay counters still match both
older variants. Mean update cycles are 362,466 versus OFF 360,802; no aligned
update exceeds OFF by one refresh interval. Whole-frame mean/p95 remain
758,504/1,103,410, with 447/500 above two refresh frames: this resolves the
observed cache regression, not the broader frame-rate problem.

Beginning at `4072d735`, the branch enables this shared behavior for normal
Terran, Zerg, Protoss and Showcase products. An explicit
`CANONICAL_RANGE_WORK_FLAGS=` restores the OFF baseline for comparisons.
Activation is an incremental routing improvement, not completion of all
campaigns or the near-30-fps goal. The known 28-update first-hit tradeoff in
the scripted unit-target case remains documented. Crowded simulation/rendering
costs require further optimization; campaign wins and hardware parity remain
separate evidence.
