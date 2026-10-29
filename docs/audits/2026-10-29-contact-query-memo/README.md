# Exact contact-mask memo qualification

This change memoizes up to four blocked-steering masks while the collision
world is unchanged. It applies to the existing `SC64_CONTACT_CACHE` builds.
The Showcase profile uses a different collision implementation and is not
covered by this optimization.

## Correctness contract

The key contains the mover's position, type, raw identity, order, step size
and normalized P6 mode. Unsupported keys, zero steps, gathering workers and
immovable actors use the existing calculation. Cached entries are cleared
at bucket begin/end, every bucket move (including movement within a cell),
spawn, damage/death, Consume, egg morph and transport mutation boundaries.
The cache is transient and adds no save fields.

A nonzero steering offset cannot make an actor closer to its own original
position. Consequently, excluding different mover slots with the same key
does not change the predicate. This argument does not apply to zero steps,
which bypass the memo.

`make test-sc64-contact-memo` runs asset-free sanitizer checks for both
collision implementations. Optional prepared-profile differential fixtures
compare the actual geometry against uncached queries. Their direct world
edits require explicit invalidation and do not prove production callback
coverage. A separate host-only oracle recomputes every hit and records world
changes between query calls; the ARM timing builds exclude that oracle.

## Evidence and limits

The frozen dense 55,000-call rendered replay compares the runtime merged at
`f0a26676752d2efb4b30fc57d6404a3600b93864` with the memo prototype. The subsequent
`6b8c49c40299fb81f0c751d9d79d014ae3d4ff8b` merge changes tests and documentation,
not runtime sources. Actual per-translation-unit dependencies were checked
against each variant's owned sources, including the linker script and raw
assembly inputs. All 137 trace, three world and three displayed-pixel
checkpoints match, with unchanged source/input pins before and after execution.

Guest cycles fall by 5.299% overall, 7.949% in the middle interval and 7.094%
in the late interval. The largest update falls only 0.263%; route floods
remain 291/40/1,489 across the three intervals. This does not establish steady
30 fps or remove the large route-building stalls. That dense replay does not
enable sampled imported audio, unlike the normal ROM profiles.

## Normal-menu mixed-AI comparison

Six isolated ARM runs use the actual normal Terran, Zerg and P7 profile flags,
including the imported sampled-audio bank. Each enters Blood Bath through the
native menu with three mixed-race bots and neutral human input, renders every
call, and ends at native game-over. The camera remains at the starting base.
Baseline and memo runs match fresh sanitizer-host world and RGB555 records:
six checkpoints each for Terran and seven each for Zerg and Protoss. Exact
window sample/exclusion counts, cycle/bucket accounting and terminal markers
pass. All six owned emulator processes terminated and were waited on, and
all 726 pinned inputs remained unchanged. An unrelated replay was untouched.

| Profile | Timed calls | Baseline fps | Memo fps | Total cycles | Update cycles |
| --- | ---: | ---: | ---: | ---: | ---: |
| Terran | 10,855 | 28.988 | 28.981 | +0.0224% | +0.7308% |
| Zerg | 14,128 | 28.756 | 28.751 | +0.0171% | +0.6406% |
| Protoss P7 | 12,876 | 28.794 | 28.781 | +0.0456% | +0.5673% |

These fps values derive from measured guest cycles, not host playback speed.
The memo trades a small lookup/invalidation cost in these lighter workloads
for the dense-replay gain. The largest memo frame costs remain approximately
300 ms, 218 ms and 216 ms respectively. This is near 30 fps on average in the
sampled workloads, not steady 30 fps, an all-camera benchmark or a hardware
result. The large pathfinding stalls remain a separate optimization task.

The host native-menu matrix also exercises every human race and 1, 2 and 3
mixed-race bots in each compiled race profile: nine cases per profile, with
world/pixel records at frames 1 and 1,001. Both variants match. This matrix
covers Blood Bath; it does not qualify every map or long active-player match.

## Public source and build checks

All 259 public `common/` and `starcraft/` files match the captured build
snapshot. Removing the private prototype activation flag and enabling the
memo with the existing contact-cache option produces identical normalized
preprocessed game code and loadable ELF sections for all four normal ROMs.
The packaged public ROMs are byte-identical to their frozen prototype builds;
Showcase has no memo symbols and is unchanged. Every normal ROM's embedded
audio-bank bytes match its pinned input. Existing RAM guards pass; Zerg has
120 bytes spare below the IWRAM guard. No guard was relaxed.

`./dev build`, `./dev test`, `./dev doctor`, source-boundary and whitespace
checks pass locally. The default ROM hash is unchanged. The full asset-free
suite includes 649 Python checks (20 skips), eight separate Python checks and
the C sanitizer fixtures. The prepared normal-P7 differential fixture passes
with and without the oracle: 1,200 randomized passes, 200 P6-active cases,
636,892 mask checks and zero mismatches. The oracle checks 48 hits and reports
zero unhooked mutations. Earlier failed synthetic bucket-toggle oracle output
is preserved separately; the fixture now invalidates at those direct edits.

The frozen dense timing ROM is
`205a143dbf33ee0eaa9998075d897c80ec640ab212e7bb26d59f7a0b2dcf70d4`;
its baseline is
`a48936c07b90958fc373a1673e615905a01b4e1db9eeecb2902d9796caebeeab`.
The mixed-run input manifest is
`34470f8a738d7e190bb4b4110baefdee3eed550172018a4d37e4b95ebc50e417`;
the reviewed runner is
`00782b1175fa8936a1147c0ef303eeb64b7d94dacad23233879a3d7977fd9ca6`.
Private source/dependency manifests, ROMs, traces and logs remain local.
No Actions run, complete-campaign result or SRAM-continuation result is
claimed by this comparison. Future actor mutations must preserve the
invalidation contract.
