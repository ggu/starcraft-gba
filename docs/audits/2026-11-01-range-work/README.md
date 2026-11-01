# Current-release range-routing integration

This reconciles draft PR #134 with release `7181c093`. The experiment remains
disabled in every normal product. The earlier audit in
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

## Activation remains unqualified

ARM link success and host sanitizer checks do not establish whole-frame speed,
acceptable navigation latency, full mission completion, fresh-process SRAM
continuity or hardware behavior. In particular, a work-unit quota is not a
cycle budget. Paired gameplay latency and ARM timing must exercise blocked,
unreachable, moving and competing targets, along with ordinary economy work.
Keep the shared flag disabled until that evidence supports activation across
campaign, custom-game and Showcase builds.

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
feature ON. The added cold-route hesitation is an activation blocker.
These are game-update counts, not measured seconds or hardware FPS. The
separate two-attacker moving-target observation does not prove fairness:
order/target continuity was not independently asserted and one attacker never
hit in either variant. Earlier invalid scene iterations are excluded.

The verified host probe is checked in for further diagnosis. ARM timing
harness drafts remain local and unvalidated, and are not part of this change.
The declaration and test-helper fixes after `ac5bf43a` do not change the
measured Z7 runtime. No release flag is enabled by this integration.
