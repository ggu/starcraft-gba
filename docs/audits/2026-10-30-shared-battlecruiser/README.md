# Shared Battlecruiser integration — 2026-10-30

## Scope

This candidate adds ordinary paid Battlecruiser production, energy, Physics Lab
research, Facility flight and source-backed Yamato orders to all four canonical
Terran, Zerg, Protoss P7 and Showcase builds. Custom-game human race selection
is independent of the ROM's campaign race. Mission objectives remain scoped to
their missions. It is stacked on the shared custom-game integration, PR167;
neither the full standard roster nor all 30 campaigns is complete.

The setup instructions are in [Development](../../DEVELOPMENT.md#battlecruiser-source-inputs).
The default original-art build remains asset-free. BC's nine compile-time
defines are a single canonical capability bundle; missing ordinary production
is not accepted as an otherwise complete bundle. Generic/legacy chapter probes
are not all upgraded to the same actor/content configuration.

`prepare-bc` writes independent CV5-cover map inputs and facility/Yamato art
under ignored local directories. Source walkability reads bit zero, separately
from cover. The authored Yamato exporter reproduces the retained bank exactly:
161 frames, 106,284 pixel bytes and a 63-row light table, header SHA-256
`b9a438857150684b80ad4ba6ca6ae3959eeb1c8dbac8a335ee0a5f5915857bd5`.
The tiny raster adapter is tracked authored code; the generated pixels are not.
The facility sidecar SHA-256 is
`fb1c69d0485312f8f0e1d43bd1566b490d237fdab6108596bfa5e2ccca2fe71c`.

## Functional qualification before idle optimization

At runtime source shared by `faf77715` / `4e57a5a`, local sanitizer fixtures
pass 64 canonical gameplay cases and 20 fresh-process save cases across four
profiles. They cover paid production/research, facility operations, Q8 energy,
automatic release and B/menu/A targeting, damage/cover, slot cleanup and save
capacity/migration. Infrastructure, funds and energy are seeded in these tests;
they do not prove a fresh human opening can reach a Battlecruiser before a rush.

At `4e57a5a`, 54 ordinary-input skirmish matches reach terminal outcomes across
both maps, every human race and one to three rotated AI races: ten wins and
44 losses. All 72 canonical menu/save/continuation cases pass. A human loss is
a terminal result, not a claim that every composition has a demonstrated win.

The full Terran aggregate passes at `f7437fa`, including ten authored host
controllers, persistence, target/collision/HUD and world/repair checks. The
controller result is host evidence; it is not a full new ARM campaign replay.
The aggregate log SHA-256 is
`348297c5d5f3d050006c9dc099d91cf2831c29cca905d72232c1422f039ded4a`.
Controller entry/policy limitations from the existing validation documentation
still apply.

An asset-free `faf77715` archive passes doctor, default build and the full
default suite (700 Python tests, 23 skips, plus C/sanitizer gates). Prepared
unified build/test passes on `4e57a5a` with 100 frozen input headers and no
private generators. Later test/Make changes have their focused checks; these
earlier broad runs are not described as repeated on every subsequent commit.

Four normal Make ARM products pass unchanged memory guards and cartridge
header, payload, padding and embedded-audio checks. All nine defines and the
producer-backed input paths occur in the actual commands/dependency closure.
Pre-optimization remaining EWRAM/IWRAM bytes are:

| Product | EWRAM | IWRAM |
| --- | ---: | ---: |
| Terran | 28,488 | 464 |
| Zerg | 28,380 | 144 |
| Protoss | 23,836 | 952 |
| Showcase | 200 | 56 |

These values do not prove stack safety for every battle. No actor limit or
reserved stack boundary was reduced. Driver command records and post-build
`-M` captures are available; they are not executed compiler-internal subcommand
logs. Sources and consumed input hashes were unchanged at dependency capture.

## Save test correction

The initial full aggregate exposed old T4/T5 fixtures writing `sizeof(RaceSave)`
directly after the production writer had switched to a compact variable-length
BC/impact extension. Ordinary production round trips already succeeded; the
fixture's restored record had the wrong envelope. Some corruption tests also
passed before reaching their intended semantic check.

The shared test-only helper now captures a production-decoded candidate and its
pending impacts, writes via the production compact writer, and proves the
record decodes to the exact intended candidate/impact bytes. It retains an
independent expected copy even when the input aliases scratch storage. Existing
semantic rejection and atomicity checks remain. Intentional old-tag cases stay
distinct. T3/T4/T5/late Terran and BC negatives, plus applicable construction,
Battery, Bunker, Dropship and P7 Interceptor fixtures use this contract.

Twenty focused semantic fixtures and 16 additional shared-service sanitizer
variants pass. The Z7 construction variant was not run: its prepared contact
mask header is absent. That is not current Z7 construction qualification.
P7 standalone Carrier/projectile fixtures without BC flags retain their prior
test layout. The full Terran aggregate passes after the corrections.

With two active Yamato records, Zerg and P7 have room for four pending ordinary
impact records; Terran/Showcase retain 32. The runtime impact queue remains 32.
An over-capacity save is refused before SRAM is written; records are not
truncated. Older saves with ambiguous BC energy/state are not guessed into a
new representation. Fresh testing should retain the preceding ROM/save pair.

## Performance finding and response

The debuggerless ordinary-menu Zerg versus three AIs probe uses the same
authored input tape, no state injection, and exactly calls 8021–9020 inclusive.
The instrumented totals include update, audio, render and presentation wait.
Hash/log work outside the timed region can affect presentation phase; these
are bounded emulator measurements, not general or physical-hardware FPS.

| Source | Cycles over 1,000 calls | Equivalent calls/s |
| --- | ---: | ---: |
| Pre-BC `e150ff6` | 698,580,726 | 24.02 |
| Initial BC candidate `f7437fa` | 758,932,459 | 22.11 |
| Idle fast path `dc286dd` | 733,372,575 | 22.88 |

All 90 sampled selected-state/fog and displayed palette-resolved RGB555 FNV32
digests match, as do sampled frame/screen/outcome fields. This is sampled parity,
not exhaustive equality. No Battlecruiser is trained in that replay, so it
cannot qualify the ability itself. The 8.64% extra cycles are a performance
regression and a reason to keep the candidate under qualification.

The change `dc286dd` skips BC order dispatch for unrelated actors unless
a recharge or named stale caster requires it, uses exact raw-12 enum predicates
in hot energy paths, and tests cheap Facility/Lab state before identity lookup.
It adds no runtime storage, save fields, counters, clock changes or RNG changes.
Four canonical sanitizer differential runs compare 7,360 / 6,880 / 8,320 / 8,160
old/new actor combinations, invalid identities, zero to two reservations,
recovery and source-clock phases, plus a real powered Battery/stale-caster
ordering case. These pass. The new debuggerless run saves 3.37% of cycles from
the initial BC candidate and matches all 90 sampled state/pixel checkpoints,
but remains 4.98% above the preceding engine's cycle total. The regression is
only partly recovered; near 30 fps is not achieved.

Post-change normal ARM links pass all four guards. EWRAM bounds are unchanged;
IWRAM code grows, leaving Terran 320, Zerg 48, Protoss 952 and Showcase 56
bytes below the existing limits. No new mutable runtime storage is allocated.
The post-change 54 ordinary skirmish traces and three 24-case continuation
logs are byte-identical to the preceding BC candidate. At the optimized runtime
with Make-only integration `4f2f497`, all 68 canonical fixtures and 20
fresh-process cases pass after forced rebuilds. The full Terran aggregate also
passes; all nine emitted controller traces are byte-identical to the preceding
BC candidate (T3 emits no trace). Its log SHA-256 is
`27cbad41dcbc4ccf438c2c537caf61bd278e11a1e5cb327348d80c04dac0a40f`.
The asset-free default ROM is byte-identical to the previously qualified
`faf77715` default. These are new targeted/build/host results, not a repeated
700-test archive run or new full-mission ARM campaign evidence.

Post-change actual ARM driver vectors match the prior vectors exactly. The
dependency/input comparison differs only in the five intended runtime files;
generated headers, audio and other consumed sources remain unchanged. All four
packaged payload/header/padding checks pass. The four ROMs are experimental
Mac artifacts, not a new recommendation to replace the preceding tested card.

## Bounded tracer arithmetic follow-up

Runtime `4203255` retains the original checked wide-arithmetic culling path,
with an exact 32-bit path only when both endpoints and the camera fit signed
16 bits and the direction is valid. Source offsets, division rounding, clip
bounds, fog, motion and effect lifetimes retain their existing meaning. The
renderer reuses its already-decoded source key for the same profile lookup.
It introduces no runtime state or reduced reserves. Renderer identity cleanup
`b83fb6b` precedes this change; test wiring is `e6791a8`.

The wide-oracle comparison passes 3,526,824 real-bank, synthetic-offset,
coordinate-boundary and invalid-direction cases under ASan/UBSan. The existing
raster culling oracle passes 1,410,048 cases. Separate asset-free processes
exercise maximum valid 32,767-pixel cached radii and invalid bounds, retaining
fail-closed behavior. These are arithmetic/raster checks, not proof of complete
native-engine behavior.

The same debuggerless tape and 1,000-call measurement window compares
`b83fb6b` with `4203255`: 733,099,732 to 723,533,683 guest cycles, a 1.3049%
reduction (22.8853 to 23.1879 equivalent calls/s). All 90 sampled state/fog/
displayed RGB555 checkpoints match. The remaining cycle overhead versus the
pre-BC 698,580,726 total is 3.57195%. This is one measured configuration, not a
hardware or general FPS result; no BC is trained by the tape. The separate
release extraction, [PR183](https://github.com/ggu/starcraft-gba/pull/183), has
its own source, inputs, qualification and measured result; this BC improvement
must not be transferred to that configuration.

Private receipts are in `.local/bc-arm-menu-perf-v4/`, including exact ordered
flags, probe/tape identities, before/after consumed-file hashes, ROM identity
and the parity/stage result. Probe BSS/IWRAM ends are `0x02036934` and
`0x03004fd0`; these belong to the instrumented probe rather than the normal
cartridges. Normal-product and broad qualification records are retained
separately in `.local/build/bc-cull-qualification-v1/`.

At test wiring `e6791a8`, four normal Make ARM products pass, with the same
bounds as the idle fast-path products. Their captured compiler vectors match
and their consumed-source changes are limited to `game.c` and the renderer
header. Forced execution passes all 68 canonical fixtures and 20
fresh-process cases. The 54 skirmish traces, three 24-case continuation logs
and nine emitted full-Terran controller traces match the retained idle-build
results byte for byte. These are host controllers and fixtures, not a new ARM
Battlecruiser ability or full-campaign replay.

Those 88 fixture executions used the preceding P7 fact header. At `7110681`,
the normal P7 Make build consumes the corrected fingerprint header and produces
exactly the same ROM (`bb338c7688932a248c850697dd77668336fd3713d23f05d1718a1539ab93907d`).
Its input closure differs only in that header. All 24 relevant P7 fixture and
controller targets use the same ordered preprocessor context, which accepts
the corrected guards. The numerical-table and binary comparisons support
carrying forward runtime results; they are not a claim that the entire
88-case matrix was rerun with the new metadata.

The independent release configuration at `171db25`, merged as PR183 at
`633997577df5f2c4c7d39fe579edd750a45ba064`, passes its default suite
(735 Python tests, 22 skips), unified gates, four ARM products and the full
Terran host aggregate. Its own paired replay measures 708,511,029 to
705,978,679 cycles (0.3574% fewer; 23.6795 to 23.7645 equivalent calls/s), with
90/90 matching sampled state/fog/display checkpoints. Different source and
capability configurations require these separate results. Neither reaches
the requested near-30-fps result.

## Fact-table gate correction

The first broad default check after the hot-loop work exposed missing
`SC64_BATTLECRUISER_ENERGY_EXPERIMENT` and
`SC64_BATTLECRUISER_YAMATO_EXPERIMENT` declarations in the generated fact-table
fingerprint. It was a real failed check, not a passed default suite. Fix
`7110681` records both gates and adds them to the exact BC P7 generator recipe.
The generated numerical payload is byte-identical to the preceding header
when exactly the two new guard blocks are removed. All 2,010 positive P7 fact
checks pass; negative compiles verify the specific generated-gate diagnostics.
Asset-free tests cover each gate mismatch in both directions. These facts are
generated from authored engine rules; no ROM/art bank is regenerated or edited.
The old input and validation failure are preserved in
`.local/bc-fact-fingerprint-v1/`. Subsequent gate results must be tied to their
actual source and fact-header identities.

The clean `7110681` archive test log records 703 Python tests with 23 skips,
the separate eight-test suite and the final C checks completing without a
logged failure. The numeric process exit receipt was not retained, so this
record is distinguished from gates with an explicitly captured exit status.
No new full default-suite exit-0 receipt is claimed for that archive run.

## Ordinary-input acquisition, ability and SRAM witness

Test-only commits `a4997ba`, `e634f7b`, `1cf3cfe` and `edeba78` add a
reproducible controller, independent replay/fresh-load programs and strict
transition conversion. They do not change production code. The source-bound
host workflow is `make test-sc64-bc-normal-input`, using already prepared
canonical Terran/BC inputs; it never invokes their import generators. The
converter's authored malformed/duplicate/order/transition checks pass, and
the end-to-end Make target has a captured exit-0 receipt.

This is one Blood Bath custom game: Terran against one active Zerg opponent.
Its actual normal start is **200 minerals, zero gas and four workers**; no
50-mineral start is claimed. The policy reads observations to choose buttons,
but all authoritative gameplay changes pass through `game_update` inputs.
Failed matches are retained rather than described as successes. The successful
run pays for the Barracks, Refinery and gas workers, defenders, Factory,
Starport/Control Tower, Science Facility/Physics Lab, Yamato research and the
400/300 Battlecruiser. It waits for natural energy, moves to reveal a target
and issues Yamato through B/menu/A. No actors, funds, clocks, energy or
research are seeded.

The original flight endpoint has 575 held-key transitions. The separate
pause/save/impact continuation has 581; fresh host processes replay both
independently. The source-bound milestones also reproduce in new debuggerless
mGBA processes with canonical Terran features, normal update/audio/render
calls and read-only witnesses:

| Input tick | Observation |
| ---: | --- |
| 12,632 | First live BC, research present, energy 12,808 Q8 |
| 16,630 | Natural energy 38,400 Q8 (150 points) |
| 17,866 | Yamato charging |
| 17,921 | Yamato flight; BC energy 7,632 Q8 |
| 17,925 | START/B pause save, game frame 17,647, valid 15,836-byte record |
| 17,937 | Shot clears; target HP 200 to -60/dead; match remains active |

The host witnesses derive the real projectile target, accept the combined
unit/building target namespace and require an actual HP decrease. The
canonical controller reports `UI_NOTICE_SAVED`; no matching-version record
exists before the save input. A fresh host process loads through HOME/L and
reproduces the hit. An independent new ARM process loads the actual
ARM-created 32-KiB SRAM file through HOME/L, sees the same in-flight shot and
200-HP target, then reproduces the impact after 11 neutral updates. The copied
ARM save SHA-256 is
`ead9f15e7ad05fbf77d675388d077c59b19a2168cbd922da0e13f12baea4771d`.
No production save/load function is invoked directly by the input driver.

Private evidence is in the `bc-normal-input-qualification-20261030` worktree:
`.local/bc-normal-arm/`, `bc-normal-arm-impact/` and `bc-normal-arm-fresh/`,
with separate probes, compiler vectors, ROM/ELF identities, logs and saves.
The 35 private production inputs match the retained Terran input receipt;
production `game.c` SHA-256 remains
`2c0651085b5325734aeaef4f0295796293af0da8e86b95b065d3ecc79ee0fdad`.
These probes use a test entry point and witnesses: they are not byte-identical
normal cartridges or physical-GBA measurements. Their unchanged IWRAM guard
passes (`0x03004ec0`); the ordinary four-product bounds above remain separate.
The hardened public workflow's exit receipt and logs are in
`.local/build/bc-normal-witness-v2/` in the shared integration worktree.

This clears the earlier absence of any ordinary-input ARM BC/ability/SRAM
witness for this specific path. It does not establish ordinary acquisition
in every race product, map, opponent combination, campaign or crowded state.
The multi-profile seeded fixtures remain separately scoped. Near 30 fps,
physical hardware, complete roster services and all 30 missions remain open.

Final clean indexed archive `edeba78` passes source boundary (1,107 indexed
text sources), doctor, default build and default test with explicitly retained
numeric exit-0 receipts. The test run records 707 Python tests with 23 skips,
the separate eight-test suite and the C checks. This resolves the earlier
missing archive-exit receipt; that historical limitation and failure logs are
retained. Evidence is in `.local/bc-final-normal-qualification-v1/`. The later
audit-document edit changes no compiled or tested source.

## Evidence and remaining work

Private evidence is relative to the BC integration worktree:

- `.local/build/bc-production-closure-v1/`: normal ARM commands, raw dependencies,
  source/input hashes and header/payload/audio verification.
- `.local/build/bc-pipeline-inputs-v1/`: canonical BC fixture commands/results,
  441 per-TU dependency captures and executable hashes.
- `.local/build/bc-skirmish-matrix-v2/`: 54 terminal matches and 72 continuations;
  the initial aggregate failure is retained alongside its diagnosis.
- `.local/build/bc-t4-save-diagnostic/`: pre/post envelope evidence, focused
  semantics, fresh saves, passing full aggregate and idle-path differential.
- `.local/build/bc-default-gates-v1/` and `bc-unified-gates-v1/`: broad gates.
- `.local/bc-arm-menu-perf-v1/`: initial paired ARM menu/state/pixel/timing proof.
- `.local/bc-arm-menu-perf-v2/`, `.local/build/bc-idle-arm-v1/` and
  `.local/build/bc-idle-broad-v1/`: post-idle-change measurement and qualification.
- `.local/build/bc-production-closure-v2/`: final four ARM source/input deltas,
  guards, raw payload and cartridge checks.

Ordinary-input ARM ability/SRAM coverage is currently the one path above.
Other canonical scenarios, broader whole-frame and stack qualification,
physical hardware, full roster services and remaining campaign stages are
still open. Native N64 resolution/random-history/blend behavior is
not reconstructed completely by the imported data and PC behavioral proxies.
Near 30 fps remains an active goal. No Actions, GDB or SD-card writes were used
for this stage. No assets, ROMs, saves, captures or private traces are tracked.
