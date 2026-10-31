# Shared Carrier and save experiment

This is a staged, default-off custom-game implementation, not a claim of full
cross-product parity. The Carrier experiment and its separate save experiment
are opt-in. Non-P7 campaign services remain fenced pending qualification.
Bot Carrier auto-purchase is restricted to the explicit shared Carrier
experiment; normal P7/P8 custom-game AI retains its previous behavior. With
the Carrier experiment enabled without the save experiment, non-P7 saves and loads are refused. The experimental Carrier save
namespace cannot currently be enabled together with the Scarab namespace.

The new compact journal generation preserves the Carrier pool and its free-row
ordering and omits only a checked, zero-filled campaign-only region. The older
Carrier generation remains supported. Tagged candidate loading is preflighted
before commit. The experimental reader now rejects selected predecessor
records without entering the legacy commit, whose mission rebind paths still
require an atomicity proof; the default-off legacy reader remains unchanged.
Combined Carrier, Scarab and Defiler persistence remains unfinished. This checkpoint does not activate these gates.

## Qualification evidence

Runtime source at merge commit `487db93` passed host checks for Terran, Zerg,
and Showcase. Each profile covered 234 custom-game configurations spanning
human and opponent races, one to three AI players, and two prepared maps.
Saturation fixtures used full actor and building pools, 66 synthetic valid
Carrier projectiles, 32 ordinary impacts, and two detached Yamato shots. The
resulting save payload sizes were 14,146, 16,308, and 16,252 bytes respectively,
within the unchanged 16,368-byte limit. Fixtures checked the full Carrier
packet, including free entries and subsequent reuse order. Separate paid-game
fixtures exercised Stargate, Carrier and Interceptor production and flight.

Before the predecessor-load fence, save/load fixtures covered fresh-process
mock SRAM, the older Carrier format, legacy-reader compatibility, corruption
fallback, and rejected saves without journal mutation. These historical
results do not establish that the newly fenced predecessor entry point loads.
Rendering fixtures exercised the active projectile arena with cached terrain
writes and canaries. The normal P7 services and
Carrier fixtures passed; the mixed Dropship branch was unavailable in that
product's actual raw map. All 109 prepared input hashes matched. Seven ARM
links passed the existing memory guards for the three experimental profiles
with gates on and off and normal P7. No hardware performance, stack-depth, or
full campaign-parity claim follows from these checks.

The source-only follow-up commits `99f0801` and `f784e09` fix test fixture
stubs, legacy-loop extraction anchors, and the fact-generator gate list. A
fresh unprepared default build and complete test suite passed at `f784e09`.
The added fact gates change generated fact signatures. Fresh, separately named
P7 tables passed a normal P7 ARM link and services host fixture, plus the
experiment-enabled Carrier host fixture. The saturation actors include synthetic
states; the tests do not exhaust every possible live phase, mission rebind, or
combined-namespace state.


## Default-off and predecessor-fence follow-up

At source commit `2a1d246b724040f8920e6a2f77032521fe18c66b`, the actual
normal P7 bot-AI fixture reproduces the unintended Interceptor purchase on the
original PR head `e276425c63bae05dc4e55133bf04c481274b0066` and passes with
the fix. Both runs used the normal P7 production vector with the shared
Carrier gate off, the same copied test source, and the same prepared inputs.
The already-qualified revision-2 P7 fact header had SHA-256
`1a981543fd91cae55536f5039899c1ca3d17c8d76c91baef2f57fa47e5d39082`.
The pre-fix assertion failure is retained in the log. This is a focused P7
host check; no separate normal P8 runtime replay is claimed.

The following host targets were built using the existing Terran, Zerg and
Showcase production vectors, with the indicated explicit experimental gates:

- `build/test-sc64-shared-carrier-{terran,zerg,showcase}`: Carrier gate on,
  save gate off; all three complete gameplay fixtures passed.
- `build/test-sc64-carrier-legacy-{terran,zerg,showcase}-writer`: both gates
  off. Each writer produced a custom and a campaign predecessor journal.
- `build/test-sc64-carrier-save-{terran,zerg,showcase}`: Carrier and save
  gates on. Each reader passed for both journals, including preflighted
  predecessor rejection, captured live-state and SRAM checks, and the existing
  tagged Carrier save/load and migration tests. Selection of an older valid
  predecessor is still visible to the journal selector, but the game reader
  refuses to commit it under this experiment.

The host compile commands were the `make -n` output for those ten targets,
with 109 prepared inputs marked old and compiler commands run under the
shared native-build lock. All 109 pinned input hashes and separately recorded
auxiliary input hashes matched before and after. The exact flag vectors and
commands are retained in the dry-run log identified below. No experimental
Carrier or save gate is enabled by this follow-up.

A separate unprepared worktree at the same source commit passed `./dev build`
and `./dev test`; the Python portion ran 886 tests with 23 skips, and the
native checks completed successfully. A first full test attempt in the
prepared-input worktree is retained as a failure: three optional renderer
checks could not find `palette-remap.bin` in the deliberately partial input
set. Its `./dev build` passed and its 109 pinned input hashes remained
unchanged. The initial P7 fixture compile also failed on an older pinned
rectangle fact table before the separately qualified revision-2 table was
used; that failure is retained in the targeted host log.

This fence does not prove that predecessor rebinds are infallible or make
predecessor loading available in the experimental reader. The default-off
legacy reader is unchanged. Normal P8 behavior has not been separately
replayed here, and this follow-up does not establish ARM SRAM continuation,
hardware timing, combined Carrier/Scarab/Defiler persistence, or full campaign
parity. The broader Carrier stage remains draft and default-off.

| Retained log | SHA-256 |
| --- | --- |
| Targeted host builds and fixtures, including initial stale-fact failure | `8ec2daf92ee35d63fd9de1771ab01be2a33d68b8ae170f362baa3272899b2be2` |
| P7 before/after default-off reproduction | `6aef9a3c414674f91928c8caeab7fa5eefef72284d6ce4d63fd8a983ad0abb4c` |
| Prepared-worktree default test attempt, including missing-input failures | `2c8e33e1241adeedfe18ef393ddc10e70981be40cb2e55aeba113173d2d9e2e6` |
| Clean unprepared `./dev build` and `./dev test` | `b919bf058090e9fef0bfd3553869c5c2face913d749b38bbe23db8b034841638` |
| Targeted host compile command dry run | `b030f1bf2cab518736514e92f669b92920360a1c908c6163b596956b31c98654` |
