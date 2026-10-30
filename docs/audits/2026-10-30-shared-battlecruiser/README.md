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

Current normal-input ARM ability/SRAM coverage, broader whole-frame and stack
qualification, physical hardware, full roster services and remaining campaign
stages are still open. Native N64 resolution/random-history/blend behavior is
not reconstructed completely by the imported data and PC behavioral proxies.
Near 30 fps remains an active goal. No Actions, GDB or SD-card writes were used
for this stage. No assets, ROMs, saves, captures or private traces are tracked.
