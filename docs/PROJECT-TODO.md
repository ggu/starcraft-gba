# StarCraft GBA goal and prioritized TODOs

Updated: 2026-11-07. This is the living backlog for the race-ROM/N64-parity goal.
Historical audits remain evidence snapshots; they are not the current task list.

## Goal and completion rules

Deliver one ROM per race, campaign/map selection, any human race against one to
three independently selected AI races in custom games, and gameplay near 30 fps.
Complete all 30 original campaign missions in stages. Use the owned N64 ROM and
emulator to verify campaign, skirmish and engine behavior where it matters.
Shared engine fixes must reach every applicable campaign, custom and Showcase
product. See [the shared-engine policy](SHARED-ENGINE.md).

An item is done only when its stated checks pass on the actual combined source.
A HOST fixture, catalog entry, source-data table or successful campaign shortcut
does not establish ordinary-input gameplay, native parity or hardware speed.
Keep N64 observations, pinned PC references and hypotheses clearly separate.

## Current real-device feedback and integration

The November 6 report is confirmed as **Protoss10 custom gameplay** from the
older KODAK package. Source merges do not update an already copied ROM.

- **Merged shared fixes:** #304 aligns B-menu icon-row highlights and prices;
  #305 enables ordinary producer L cancellation/full unit refunds; #306 gives
  all active unified-custom players 50 minerals/zero gas; #308 preserves
  paused-menu resource refusal cues; #309 continues established mineral jobs
  to eligible nearby known fields; #311 uses the source-shaped Pylon field
  for placement, production and cached/uncached Cannon power. Separate
  Egg/Carrier/Scarab lifecycles are unchanged; the visual field overlay is off.
- **Validation and package:** the normal Mac package is now
  `StarCraft-GBA-2026-11-07-d3535af6`, adding pending mineral-trip continuation
  #339 and shared detector optimization #342 to the earlier stock-sight,
  menu/mining/audio/power package. Four qualified canonical ELFs were reused
  after confirming production sources/build definitions match merged
  `d3535af6`. The six normal links froze 908 selected source/private/toolchain
  hashes against the previously qualified input bundle; root independently
  rechecked all 908 and all six ELF/map hashes. The packaged P10 is exactly
  the binary used in the combat and one-AI custom comparisons, with sampled
  state/fog/full displayed framebuffer/palette parity and 2.782% fewer mean
  cycles in the one combat window. Clean default checks (1,023 Python tests,
  26 skips, plus compiled checks) and prepared unified build pass. Broader
  unified/Terran full-campaign checks lack documented private preparation
  files and are not claimed. Earlier audio/menu/mining qualification remains
  scoped in its audits; it was not all repeated here.
  Headers, logos, SRAM signatures and all 19 payload/metadata checksums passed
  read-back. Four-product minimum IWRAM/EWRAM margins remain 1,464/780 bytes;
  extra Z7 retains 1,168 IWRAM bytes. No hardware FPS or stack claim follows.
  **KODAK still has the older e9838086 package** until a new physical copy is
  recorded; the card is not currently mounted. Earlier Mac packages remain.
- **Creation provenance #323 merged:** shared foundations and ordinary
  products fill missing source-owner slots only from a verified creator or
  uniquely mapped human controller. Existing rescued actors retain their
  origin and existing mission hooks take precedence. P1's real source-backed
  rescue→Nexus Probe→Forge HOST fixture and P3/P4/T3 cases pass with explicit
  authored setup; all six normal ARM links pass unchanged guards. P5's fixed
  reverse-map hook, source-dependent constructor initialization, dynamic
  target tests and universal controller identity remain open. See the
  [scoped creation audit](audits/2026-11-06-creation-provenance/README.md).
- **Controller cleanup #310:** producer selection escapes use SELECT, keeping
  intentional L cancellation and moved drags. Fresh HOST T5/Z3 wins on the
  worker source and combined power P3/P7/P10 wins are scoped in their audits.
  Current Z7 attempts under the tested Z9 inputs do not win at the documented
  bound with either unchanged or updated controls. Z4/T6 also remain unqualified;
  existing Z8 and persistence gaps below are not closed. Historical win totals
  and old tapes must not be reported as current-source certification.
- **Mining ordering #328 merged:** workers now spend the configured 45/60
  stationary updates before mineral/gas pickup, rather than bypassing a timer
  set after pickup. Both shared and legacy paths clear interrupted progress
  without deleting cargo. P10 all-race/resource/debit/travel/interruption
  fixtures, the public cargo target, a genuine old-writer/new-reader timer
  journal, clean default checks, six guarded links and one P3 HOST win pass.
  Empty explicit mineral trips now also continue to an eligible nearby field
  if their destination is depleted en route; see the
  [pending-trip audit](audits/2026-11-07-pending-mineral-retarget/README.md).
  Native field queues and exact gathering rates remain open: the retained
  durations are not an N64 timing claim. Old nonzero N now collects on N,
  versus the former N+1. See [the scoped cycle audit](audits/2026-11-07-worker-gather-cycle/README.md).
- **Menu navigation #330 merged:** a fresh D-pad tap moves immediately;
  a single held direction repeats after 18 updates, then every five.
  A/B remain edge-triggered, transient state resets, and no saved fields change.
  Actual P10 custom uses the text list; only card-eligible contexts use the
  icon grid. Focused list/card fixtures, clean default checks and six guarded
  links pass. The current Mac package includes both fixes; category grouping
  and real-device feedback verification remain open.
- **Research groundwork #318 merged, gameplay open:** the default-off
  completed-level journal uses a distinct namespace and validated building-row
  padding without reducing impact capacity. Review repaired a new-match reset
  leak, roster-tag collision and unsupported-layout admission. Genuine 94bc
  P10/Z9 campaign/custom predecessor journals load in separate consumers;
  earlier feature-on sanitizer and six-profile link evidence remains scoped to
  that base. At the #318 integration point, flag-off Z9 matched the then-qualified
  package; this does not describe the later #323 binary. The clean full default
  suite passes at `5b733f16` (1,023 Python tests, 26 skips, plus compiled checks);
  the prior missing-header fixture failure is preserved.
  **No Forge/Core menus, paid jobs or combat effects are enabled.** Current
  feature-on integration, source-owner policy, active-job persistence, legacy
  migration, runtime stack and gameplay qualification remain open. See the
  [wire audit](audits/2026-11-06-research-wire/README.md).
  **#327 merged** verified family facts for all 228 armor and 130 native weapon
  IDs, with a private all-30-campaign policy generator. Review fixed base/expansion
  cost precedence; the authenticated 30-map regeneration still marks all 180
  selected mission/family price policies supported. The new facts header is
  unused by the engine. These are groundwork, not usable upgrades.
  The separate `codex/shared-research-current-20261107` branch has passing
  constructed P10/Z9 HOST fixtures for actual producer fields, quoted payment,
  power pause, completion, cancellation/refund, slot reuse, custom owner wallets,
  real custom-menu admission and ordinary weapon/range integration. Source
  prerequisites distinguish level-one Shields from its later Core requirement.
  Detached candidate checks reject duplicate jobs, forged payers, incompatible
  legacy jobs and inactive/non-Protoss owners' typed levels. These stages are
  **not in main or enabled in a ROM**. Active-job journal work is isolated on
  `codex/shared-research-journal-host-20261107`; the reserved namespace alone
  does not authorize a ROM writer. At runtime `c98542e2`, constructed P10
  HOST fresh-process tests now pass genuine normal 0x43, predecessor 0x44,
  new active-job 0x45 and mixed journals; checksum-invalid and semantic-invalid
  newer records fall back to the untouched 0x43 predecessor. All 589 pinned
  inputs remained unchanged. That checkpoint did not qualify Z9 journals or
  GBA memory: its HOST receipt array exceeded Z9's reserve.
  **Current bounded stage: [draft #336](https://github.com/ggu/starcraft-gba/pull/336)**,
  branch `codex/bounded-research-20261107`, tested runtime `8a96f995`:
  both per-building receipt arrays are removed. Canonical producer fields and
  a frozen version-specific 0x45 quote contract reconstruct the payer/price.
  P10/Z9 lifecycle tests, P10 ordinary/splash armor and real Archon/Interceptor
  weapon-event fixtures pass with 589 inputs unchanged. Genuine 0x43/0x44,
  new 0x45, mixed journals and damaged-newer fallback pass in fresh P10 consumers;
  a separate Z9 bounded-source repeat also passes all combinations and four
  fallback cases with 592 inputs unchanged (test/audit branch
  `codex/research-z9-bounded-journal-20261107`, `f58cab3e`).
  All six experimental ARM profiles link with the actual active-job journal
  enabled and unchanged guards; root verified all six ELF/map hashes and
  margins. Z9/Z10 retain 732 EWRAM bytes; Z7 retains 1,024 IWRAM and 2,900
  EWRAM bytes. These static margins do not prove runtime stack or hardware safety.
  The combat-only feature-off P10 control reproduces the exact latest normal
  binary `b1b5edcc...`; final combined default/full-product checks remain open.
  **Ordinary P10 custom checkpoint now passes on the prototype:** source
  `90d84f3e` used title/field buttons to choose one AI, gather resources, build
  a Pylon/Assimilator/Forge and pay for Ground Weapons. The exact guarded P10
  ARM guest exported a genuine 0x45 save; a fresh guest loaded it through title
  L and completed level one without loss. The ARM and HOST save bytes match
  (`ddb79ad3...`), and root independently rechecked the pinned runner, ROM,
  ELF, tapes, save bytes and all 589 prepared inputs. The runtime remains
  `8a96f995`; test-only checkpoint source is on
  `codex/research-ordinary-controller-20261107`. This is one emulated custom
  checkpoint, not a hardware test, match win, full-state oracle or other-product
  ordinary-gameplay qualification. A reusable replay guard is under review.
  **Normal release flags remain off.** Exact shields/fractional HP, remaining
  special/projectile routes, campaign context/source policy, stack/performance,
  other-product ordinary gameplay and full default/production checks remain
  required. Static compiler stack inspection on
  `codex/research-stack-budget-20261107` reproduces both qualified P10/Z9 raw
  binaries; inspected structural paths sum to about 4,348/4,528 bytes, with
  unresolved indirect/assembly/interrupt paths. This is not stack high-water
  or a complete safety bound.
  The separately reviewed human-binding stage at `344e48de` passes pure and
  actual P1–P10 initialization/wallet/P1 rescue fixtures. P1–P9 human source
  identity is 7; P10 is 1. Retained actor origin is not the current controller or
  payer. This helper is unused; it requires authenticated explicit context and
  proves neither enemy wallet routing nor candidate-loader admission. See
  [the scoped human-binding audit](audits/2026-11-07-research-human-binding/README.md).
  A separate larger-save investigation on
  `codex/flash-journal-geometry-20261107` preserves the first 32 KiB as read-only
  legacy journals and models two new slots with isolated commit sectors and
  28,656-byte payload capacity. The independently reviewed HOST model passes
  sanitizer tests, interrupted writes/partial tag erases, stale-winner rejection
  and semantic fallback. It is not connected to game saving; Flash hardware
  timing, bank/sector behavior, real power cuts, memory and integration remain
  open. No save format or release behavior changes with this experiment.
  Draft #336 now also wires human campaign policy into actual P1–P10
  initialization, Forge/Core menus, payment, update and refunds (runtime
  `13953e82`). A source-cap bypass through generic research commands was
  repaired for staged custom and campaign play; genuine old completed/pending
  generic jobs remain floors. Campaign/custom sanitizer fixtures pass, and
  research-off normal P10 reproduces the qualified `8e9f0a0a` bytes. This campaign
  flag is HOST-only and refuses persistence: campaign job journals, enemy
  controller bindings, exact shields/HP, ordinary play and full product checks
  remain open. The implementation/audit is on [draft #336](https://github.com/ggu/starcraft-gba/pull/336),
  not enabled on main. Source layout/checks and final composition continue there.
  Category navigation and the power-field overlay still need ordinary-input,
  framebuffer and dense-scene checks.
- **Other feedback:** death sounds, large death effects, remembered
  resource/fog presentation, AI difficulty choices and direct native defeat
  behavior remain open. Current custom defeat counts the last live owned
  building. Delivered death pixels/timelines were authenticated; their large
  source frames require a paired native presentation check, not guessed scale.
- **Protoss construction audio #329 merged:** explicit foundation and
  completion cues use the separate source-indexed 528/529 clips. Load/new-match
  priming prevents replay; old banks preserve completion fallback without
  spending effect budget on an unsupported start. The new bank preserves all
  510 previous SFX and 13 music streams, adding only the two clips. Combined
  new/old-bank fixtures, ordinary-input Protoss custom sample requests,
  clean default checks and six guarded links pass. The new package includes
  that bank. Native audibility, exact event timing and physical GBA sound
  remain unverified; see [the audio audit](audits/2026-11-06-protoss-warp-audio/README.md).
- **Native construction reference #317 merged:** fresh ordinary-input N64
  custom gameplay selected a Probe, opened the Pylon menu, placed its green
  preview and captured the blue-white warp orb. **#326 merged** adds a fresh
  completed-Pylon observation with supply 4/9 to 4/17. Power boundaries,
  sound and an actual delivered-GBA render comparison remain open. The
  authored cyan rectangle is only a GBA fallback; it does not establish the
  source-art path's active appearance. See the
  [scoped native audit](audits/2026-11-06-protoss-native-reference/README.md).
  The [P1/source follow-up](audits/2026-11-06-p1-rescue-native/README.md)
  separately records the unproven rescue-passive transfer breadth; no native P1
  rescue was observed, and the custom-game report does not establish that bug.
- **Native Forge #335 merged:** two fresh ordinary-input N64 custom sessions
  show Forge 150, Assimilator 100, a blue power overlay during placement,
  a green Forge preview, and its subsequent warp/structure. Root checked
  binary/ROM/capture/log hashes against recorded source revisions and viewed
  the placement/warp/structure captures. Research menu, selected completion,
  admission, cancellation and power/prerequisite-loss policy remain unobserved.
  The experimental interactive frontend now permits 14,000 total steps while
  retaining its 180-second wall cap and other bounds; the GBA engine is unchanged.
  See [the native Forge audit](audits/2026-11-07-native-forge/README.md).
- **Native fog input #340 merged:** the experimental N64 bridge now exposes
  distinct C-Right/Up/Down inputs alongside D-pad Right, with a public mapping
  test. The initial missing-Move-command failure, subsequent delivery check and
  one preplanned scouting/return run are retained in the
  [native fog audit](audits/2026-11-07-native-fog-memory/README.md). Root checked
  source/binary/capture/log hashes and viewed the captures. Rocky coastal
  movement and textured dark boundaries were seen, but no identifiable world
  cell was proven outside all unit/Nexus sight after discovery. Terrain and
  resource memory remain unverified; do not infer them from a camera pan.
- **Early Egg continuation stage:** branch
  [`codex/shared-egg-next-20261107`](https://github.com/ggu/starcraft-gba/tree/codex/shared-egg-next-20261107)
  at `f7176304` adds a HOST-only Z1 admission/validation slice. A real paid Egg
  survives the actual journal into a fresh HOST process and completes natural
  Drone birth; the genuine unretagged 32 KiB image is unchanged after loading.
  Malformed producer nondebit and checksum-valid forged-owner atomic rejection
  pass, alongside staged Z8/Z9/capacity fixtures with 592 hashes unchanged.
  A separate [Z2 HOST slice](audits/2026-11-07-z2-egg-host/README.md)
  uses the verified owned-ROM permission bank: paid paired Egg birth after a
  fresh-process load, a denied-product new-tag rejection, and fallback to a
  genuine current-main old-tag save pass. The setup is constructed, not an
  ordinary-input win. Normal flags, ARM, Z3–Z6, trigger reservations, ownership
  transfers, the full profile matrix and common activation remain open.
  A separate [Z3 HOST stage](audits/2026-11-07-z3-egg-host/README.md)
  passed actual-bank admission, denied-product validation, fresh paired birth,
  genuine old-tag fallback and a cross-owner slot boundary. Z4 CreateUnit
  reservation has a [bounded HOST retry experiment](audits/2026-11-07-z4-egg-host/README.md)
  with source-tag and fresh-process tests; standalone profile, ARM, ordinary
  play and N64 behavior remain open. Z5/Z6 require separate trigger audits.
  The constructed late captured-Hatchery concern was not promoted: no reachable
  transfer path was found, and relaxing only admission would leave saves invalid.
- **Performance #313 merged:** late-Zerg uses the existing full-field flags
  already active in other products; the separate monotone shortcut remains off.
  Eight release links, route oracles, clean default checks and 18 HOST warm
  comparisons passed. A reachable Z9 custom save/fresh-load pair matched 500
  updates and measured 2.53% fewer emulated cycles in one passive three-AI
  opening. Exact packaged-ROM profiling reproduced the candidate's 611,692
  mean cycles/update and 3,102,721 peak; neither is a hardware FPS claim.
  The strict Z7 winning validator still fails equally in the compared builds;
  it was not weakened. Some earlier prepared-bank campaign map bytes differ
  from the final package, so campaign qualification must pin actual selected
  inputs, not just similar compile flags. The near-30-fps goal remains open.
  Current profiling correlates order spikes with synchronous route-field
  builds. **#315 merged** enables the existing packed terrain-grid builder
  for Z7–Z10, preserving its scalar toggle. Eight normal guarded links,
  33 complete-grid cases, actual Z5 door/save-load parity, four HOST display
  oracles and 18 custom openings pass. In the exact-package Z9 500-update
  comparison, mean cycles fell a further 0.895% and the observed maximum
  18.5%; selected persisted-state fingerprints matched every update. This
  remains one passive-human opening, without guest pixels, a current campaign
  win or hardware FPS proof. The current Mac package includes this optimization through the
  later combined release binary; these earlier timings are not measurements
  of the current menu/mining/audio source.
  See [the scoped grid audit](audits/2026-11-06-late-zerg-bulk-path-grid/README.md).
  Actions stays disabled; these checks run locally.

Current-source P10 combat profiling at `8e9f0a0a` uses the same ordinary P7
input tape and current audio bank. The 1,000-call pair contains 958 advancing
calls, 42 nonadvancing calls and three modal acknowledgments; timelines and 11
sampled selected-state/fog/full displayed indexed framebuffer/palette hashes
match. Control mean is 2,372,998 emulated cycles per advancing loop. The nested
diagnostic adds 2.359% overhead; its setup/aura/Arbiter-cache/detector-refresh
means are 667,526 / 151,884 / 390,148 / 207,680 and overlap, so they are not
independent promised savings. Movement averages 357,258 and dominates the
observed worst order spike. This is one scene, not hardware FPS or all custom
race combinations. Public evidence is on
[`codex/arbiter-perf-current-20261107`](https://github.com/ggu/starcraft-gba/tree/codex/arbiter-perf-current-20261107).

The conservative detector-owner union was rejected after matching sampled
state/pixels but worsening mean cost 0.552%; every observed union was 0x91.
The combined shared snapshot and binding-hoist shortcuts save 2.782% in that
one scene, with sampled state/pixels matching; a separate one-AI custom opening
also matches sampled state/pixels. Six normal ARM links pass unchanged guards.
See the [scoped detector audit](audits/2026-11-07-detector-snapshot-optimization/README.md).
A 16-entry per-tick optical-cell memo also loses 0.332%, with only 8.74% observed
hits. Follow-up classified 3,538 compulsory and only 64 conflict misses,
so hash tuning cannot materially help that workload. The memo is rejected;
these measurements do not authorize changing update cadence. These small/negative results do not close
the near-30-fps gap. Prioritize measured setup and movement work over repeated
blind cache variants. Additional prototype evidence is on
[`codex/detector-snapshot-perf-20261107`](https://github.com/ggu/starcraft-gba/tree/codex/detector-snapshot-perf-20261107).

The optical early-mask experiment remains off: it saved only 0.627% mean
cycles and 0.032% peak in the measured P10 window. Further movement attribution
found one 12,001-cell point flood dominating the peak, but only five floods in
1,000 calls. Its blocked origin has two reachable neighbors; neither a static
connectivity rejection nor an unreachable-route claim is justified. Recurring
movement cost instead centers on contact-mask and step-clear work (about
158k/88k diagnostic cycles, nested scopes with 4.121% observer overhead).
Inspect those recurring operations before more rare-path cache changes. These
experiments are source-only branches, not shipped speed gains.
The contact-mask prototype was **rejected for promotion** after its current-main
comparison. Its independent winner oracle passed 225,890 cases; dense OFF/ON
queries and mutation checks passed, and sampled state, fog and full displayed
pixels/palettes matched in the ordinary-input P7 window. Nevertheless its mean
cost rose from 2,294,431 to 2,346,897 cycles across 958 advancing observations
(2.29% slower). The source-only experiment is preserved on
`codex/contact-early-current-20261107` at `ded29e3d`; no speed gain shipped.

**Shared sight #333 merged as `062acff7`:** normal Probe sight now uses 128
GBA pixels instead of the generic 56, and Nexus 176 instead of 112. All 228
checked-in sight bytes match the owned N64 units table. Common race flags apply
the fix across campaign/custom/Showcase. It aligns explored cells with actual
visibility, incomplete/morphing sight and the detector policy, including P10's
source-slot-1 human and retained rescue origins. Focused checks, full clean suite
and six unchanged guards passed at `ed0dc4ba`; root verified 976 input hashes.
The same-correct-sight scalar/span replay matched sampled state, fog and full
displayed pixels/palettes in 958 advancing observations; spans used 19.85% fewer
mean cycles than scalar code. Against older incorrect short sight, mean cost is
3.42% higher numerically (world visibility intentionally differs). This is a
correctness/cost tradeoff, not a shipped overall speedup or 30-fps result.
An ordinary-button GBA visual pair shows the larger revealed terrain at tick
800, with matching observed frames and minerals at four checkpoints. Root
checked all eight captures. The normal Mac package includes this correction.
Exact N64 border geometry, remembered resources and initial masks on other maps
are separate questions; larger nominal sight does not prove those behaviors.
See [the source sight audit](audits/2026-11-07-custom-fog-sight/README.md).

Priority: deliver the verified normal package when KODAK is connected, implement
usable shared Forge/Core research, reduce measured recurring collision work,
and repair current campaign/winning-replay gaps. Continue native visual/audio comparison with explicit
save/source-owner and timing evidence; do not advertise unwired components.

## Current baseline

- Canonical branch: `codex/source-release`. The source-permitted Scourge/Spire
  correction in #290 and save-context hardening in #293 are retained. #295
  adds the explicit shared Egg preview; its normal product flag remains off.
- Latest verified normal Mac package: `StarCraft-GBA-2026-11-07-062acff7`:
  Terran 1–10, Zerg 1–9, Protoss 1–10, plus Showcase. The older KODAK package
  is `20261106-e9838086-campaigns`. Content coverage remains **29/30**;
  this is not a claim of complete native mechanics or campaign parity.
- Separate opt-in Mac package: `20261106-e9838086-custom-eggs-preview`.
  Versioned filenames and manifests distinguish it from the normal products;
  its shared Egg service applies to custom games, not Z8–Z10 production.
- Campaign products retain Outpost/Blood Bath custom games, all human races
  and 1–3 independently raced AIs. Larger roster and behavior gaps remain below.
- The [earlier P7 measurement](audits/2026-11-06-current-native-qualification/README.md)
  at `72a71d22` costs about **2,200,226 cycles per advancing full loop**, versus
  about **559,000 cycles** available at 30 fps. The paired diagnostic matches
  90 sampled state/pixel checkpoints and adds 3.32% overhead; order/Arbiter
  setup, movement and rendering remain major scopes. These replacement-main
  emulator measurements are not hardware certification or a new speed gain.
- The [pre-#323 P10 busy profile](audits/2026-11-06-current-p10-busy/README.md)
  uses the exact delivered ROM, reproduced byte-identically from merged
  `c27bb48c`, with 392 dependencies verified. Its P7 physical-input window
  averages 2,253,117 cycles across 958 advancing calls, against about 559,000
  available at 30 fps. Eleven selected-state/displayed-frame/palette samples
  match; no every-frame or hardware claim follows. A guarded diagnostic
  (2.643% observer overhead) attributes about 464,761 cycles to Arbiter due
  processing on average, while movement dominates the worst sampled spike
  (2,673,153 cycles). Profile mean and tail separately; do not alter cadence
  or predict savings by adding nested scopes. #320 adds the bounded read-only
  video observation and durable bounds checks, with no release optimization.
- The pre-#323 `ff94a2ea` P10 package has seven bounded early P1–P7
  save/load checks plus a P1 mid-mission save at frame 3793 with matching
  selected-state controller-tail continuation and unchanged SRAM. The old
  tape still does not win, including 1,000 extra neutral advancing frames.
  This is not complete-state or continued-win proof, and the successor #323
  binary needs its own dynamic qualification. S3 remains open.
- Shared transport is enabled in normal release vectors. Its availability,
  AI use and native behavior still need scoped checks; do not describe the
  entire service as absent based on older documents.
- N64 Depot observations repeated a 1,514-VI interval from visible mineral
  debit to supply-cap change. This is **not an internal game-update measurement**.
- Fresh-reset and controlled ordinary movement support two N64 SCV records.
  A bounded interpreter-only write trace and offline guest-code inspection
  identify an order-like byte decrement/reload gate and 36 numeric pre-handler
  IDs matching the pinned PC switch. Actor identities cannot be carried from
  the normal-JIT process into the separate interpreter run without new evidence.
  A [same-process interpreter trace](audits/2026-11-05-n64-order-cadence/README.md)
  now couples four typed worker timers to 126 consecutive countdown intervals.
  Named handler equivalence, STOP, an active-world clock and all-order/JIT
  cadence remain open. Offline code also shows a linked-list timer stagger,
  initialized with 10 and reloaded with 150; the list scope and world-step
  frequency are not yet established. Do not apply a uniform nine-update skip.
- A pause/speed-correlated value was later found inside RNG-like increment,
  multiply and return code. It is not a validated game clock. The retraction is
  part of the evidence, rather than a basis for changing production timing.

## Current Nydus regression qualification

The [paid-target cleanup correction](audits/2026-11-06-nydus-paid-targets/README.md)
reproduces and fixes an Egg product-ID/unit-slot collision during canal transit.
Its full focused sanitizer fixture, six normal ROM/memory builds and prepared
unified checks pass. A prepared-input default run exposed five renderer-fixture
failures also present on unchanged main; the corrected complete-overlay oracle
and current HUD compile dependencies pass both renderer modules. Final default checks now pass at `da316b94` (1,004 Python tests, 13 skipped,
plus compiled suites); engine/build source matches the six-ROM/unified-qualified
`3ebed0e2`. This does not enable shared Egg/Guardian/
Scarab stages, prove native canal timing or change the 29/30 content count.

## Priority order

Priorities are the maintainer's decisions after the [2026-11-06 GPT-6.1 Terra
audit](audits/2026-11-06-n64-priorities/README.md) and current-code review. This
prioritization used existing N64 evidence and current source. Later focused
N64 observations and GBA comparisons are scoped in the audits linked above.
Parallel work is appropriate where source ownership and
the serialized native-test lock permit it.

| ID | Priority / state | TODO and reason | Completion evidence required |
| --- | --- | --- | --- |
| C1 | P0 / in progress | A [paired 31k-call HOST trial](audits/2026-11-05-z10-damage-attribution/README.md) now attributes 12 lethal Drone hits and 63 lethal Hydralisk hits to the generic Reaver path. Trace/control and original-input sequence matched; all 99 observed AI unit purchases paid their expected costs. | Finish source attribution for generic placeholders, construction removals, target priorities and income; compare relevant native behavior. This HOST-stage result is not a guest win or proof that native ammunition alone changes Z10's outcome. |
| C2 | P0 / open | Complete shared Reaver/Scarab behavior. Campaign Reavers currently use a generic 100-damage attack; custom Reaver purchase is blocked while the paid ammunition service remains staged. | Source/native ammunition and attack-event checks; paid training, capacity, launch, flight, terrain/target loss, splash, death/slot reuse, save/load and AI purchases across applicable products. No generic free shot as a substitute. |
| S1 | P0 / in progress | Finish shared Egg/Guardian production. The [opt-in late service](audits/2026-11-06-current-native-qualification/late-eggs.md) now has four actual ARM campaign Egg/SRAM continuations; normal activation across products and Guardian gameplay remain unfinished. | All real profile codecs and journals, interleaved tag contexts, old raw-36 Overlords, bad-newer fallback, atomic rejection, paired births, trigger reservations and Carrier/Egg capacity boundaries. Then ARM/RAM/stack, ordinary controls and genuine SRAM continuation before common activation. |
| S3 | P0 / open | Qualify saves throughout each combined campaign, including early Protoss missions under the P10 product. [One early physical P1–P7 save point each](audits/2026-11-06-early-protoss-save/README.md) now reloads with matching selected-state continuation on the pre-#323 `ff94a2ea` P10 ROM; P1 also has a matching mid-mission controller-tail continuation, but its historical tape no longer wins on the packaged product; P2/P7 remain especially early and no winning continuation is established. | Fresh-process ordinary save/load and continued wins across all campaign entries on current combined flags, with source-owned effects, actors, clocks and objectives preserved. Reproduce and fix each rejection; do not infer arbitrary-save support from one point. |
| C3 | P0 / open | Finish Z10 through ordinary controls and retain all other 29 missions. Rally recall and inefficient selections were fixed in the controller, but no Z10 win exists. | Fresh ordinary-input win, Temple/crystal sequence, text/results, save prefix, fresh-process continuation and ARM replay on the integrated engine; rerun affected carried missions. Never inject a win, resources, units or SRAM. |
| N1 | P1 / in progress | Validate N64 actor records, game clock and event cadence. This is a prerequisite for behavior-changing timing optimizations. | Explicit select/move/stop correlation for independent actors, fresh reset, owner/type/position validation, independent clock and writer-PC evidence. Bound read-only snapshots; no guest writes or MMIO side effects. |
| P1 | P1 / open | Reduce representative busy-frame cost toward 30 fps. Small isolated gains do not close the current gap. | Exact-source normal/diagnostic pairs on P7/P10 and 1-/3-AI custom games, advancing combat frames, ordinary input and modal handling. Measure update/render and worst-frame distribution; require material improvement without mode regressions. Confirm on GBA hardware. |
| C4 | P1 / open | Replace campaign AI scalar approximations where verified source/native behavior differs. Late-Zerg waves use averaged groups/gaps; arrival, rebuilding and transport tactics are not equivalent to script commands. | Source script interpretation plus N64 observations of actual production, wallets, launch groups, targeting and rebuilds. Reproduce a concrete mismatch before changing AI; retain mission objective semantics. |
| U1 | P1 / partial | Scourge/Spire source restrictions and shared picker correction are merged in #290. Continue the broader campaign PUNI/UPGR/TECx audit, especially P10. | Preserve current controlling ownership, nonspending denial and full supported custom catalog. Apply remaining source permissions with per-owner tests and save compatibility; do not infer complete availability parity from this one product fix. |
| U3 | P1 / in progress | Deliver usable shared Forge/Core research. #318 is disabled save groundwork; verified family IDs and campaign-policy extraction remain separate work. | Authenticated owner and payer identity; source initial/max levels; atomic paid producer jobs, power/cancel/death/capture/reset behavior, active-job saves and legacy migration; weapon/armor/shield/range effects on every relevant path; ordinary building menus and cross-product campaign/custom/Showcase checks. |
| U2 | P1 / open | Integrate Carrier/Interceptor behavior for supported ordinary units and P10 Gantrithor. The current Gantrithor can move but does not launch Interceptors. | Paid children, flight, target/parent death, capture/owner changes, reload, capacity and full profile save matrix; P10 hero and custom-game ordinary controls. Resolve held Carrier namespace and merge conflicts first. |
| S2 | P1 / open | Maintain one shared capability and persistence contract across products. Transport is present; actor mappings, research restrictions, repair, detection, morphs and remaining abilities still need a current matrix. | Per-product and per-mode inventory from actual flags/call sites, source-owned permissions, lifecycle/death/capture checks and every applicable save codec. No Showcase-only engine fix or mission-objective predicate used as a service gate. |
| K1 | P1 / open | Qualify complete supported custom games for all race combinations and 1–3 AIs. Validate building/unit progression, economy, targeting, victory and long-game stability. | Both maps, each human race, homogeneous and mixed AI races, actual tech progression and save/load. Track global pool/queue pressure and ensure human production is not silently starved. |
| M1 | P1 / open | Keep expanded mechanics within GBA memory and global actor capacity. Z10 has reached the 240-actor pool, preventing its former 70-Hydralisk launch goal. | Unchanged safety reserves, ROM/EWRAM/IWRAM and stack checks, child/queue/trigger capacity accounting and sustained busy play. Document adaptation limits; do not present catalog capacity as native-scale simulation. |
| B1 | P2 / open | Finish combat-rule and attack-event fidelity: fractional HP/shields, armor/size/upgrade effects, Firebat collateral, appear-at-target wind-up, shield-hit effects and specialized weapons. | Verify current code first; compare source/N64 events, not only appearance. Exercise allies, invincibility, shields, deaths and impact saturation with persistence and clipping. See [combat audit](ENGINE-FIDELITY-AUDIT.md). |
| B2 | P1 / open | Unify shared projectile flight and source attack events. Current follow/bounce traces use fixed endpoints while damage stays bound to a live target; stationary/melee paths can apply damage before depicted wind-up. | Native moving-target/release observations; bounded shared flight/event state, consistent visual/damage impact, separate shots, verified bounce rules, death/slot reuse/overflow, genuine save continuation and crowded ARM cost across products. See [visual/combat audit](audits/2026-11-06-n64-priorities/visual-combat.md). |
| V1 | P2 / open | Audit remaining source sprites, animations and placement/order effects, including each supported unit/building, damaged buildings, corpses, resource cargo and construction poses. | Paired N64/GBA observations, correct source frames/anchors/palette/timing, no missing assets, scene/HUD clipping and crowded effect/VRAM reuse checks. Existing fixes must be checked before reopening old reports. |
| V2 | P2 / open | Improve blocky fog/terrain boundaries without violating RAM/VRAM budgets or making busy frames slower. | Current-source paired visual/performance evidence, both maps/cameras, explored versus visible semantics, sprite/audio/impact compatibility. Rework the held fog draft rather than enabling a losing renderer. |
| Q1 | P2 / open | Resolve controls/advisor PR regressions and polish N64-informed UX: non-destructive deselection, queue cancellation/refunds, alert suppression, camera bookmarks, settings and results. | Ordinary-input campaign/controller matrix and updated replays, visible hints matching input, native alert/control observations where relevant, persistence and no paid-order surprises. |
| R1 | P1 / ongoing | Publish only qualified source, merge ready PRs, and package versioned ROMs with accurate descriptions. | Local source/default/prepared/ARM checks appropriate to each change; recorded exact head and results. No unnecessary Actions. Preserve old saves, verify SD read-back hashes and safely eject when copying is possible. |

P0 items are completion/behavior blockers, not permission to skip their memory,
save, source-boundary or regression gates. N1 and P1 should proceed alongside
campaign work; late cosmetic work should not displace them.

## Playable-production gate after the feature15 audit

Read-only review of staged source at `2e6bc51c` found that the passing HOST
service fixture begins with a spawned Reaver. The actual custom menu still
excludes Support Bay raw171 and product admission rejects Reaver raw83. Paid
ammo controls exist only under staged flags; capacity/damage research is
unwired. These fixtures do not qualify a normal playable tech tree.

Make the next C2 milestone the actual opt-in Support Bay/Robotics Facility
prerequisites, paid Reaver production, menu ammunition, scheduled launch/impact,
and genuine save continuation. First supply the source-slot-aware wallet and
capture policy: Z8 and late-Zerg AI towns have distinct banks, while current
generic Scarab stocking falls back to a combat-owner wallet outside the unit
production bank swap. P7/P8 source-bank paths do not establish those contexts.
Keep unsupported admission closed until the complete path is implemented; do
not count a catalog row or a spawned actor as a playable product.

AI stocking is a separate disabled probe with an authored per-tick policy;
normal Reavers begin empty there and do not stock without it. Current source
hangar bytes do not establish N64 starting ammunition. Research acquisition,
native replenishment/launch observation and campaign mechanics remain required.

For S1/S3, typed motion also needs its full heading/velocity pair: typed cooldown
is eight bits while live Scarab motion uses sixteen. A live-Scarab-only HP
container representation is feasible but unimplemented; dead hidden/training
rows need explicit handling. The full writer still selects ZP rows and feature7;
actual typed/derived writer, guarded decode, reverse bridges, old-writer/fallback
and full selected-journal proof remain unfinished. Component codec tests do not
close these gates.

On 2026-11-06 the shared disk reached ENOSPC, so new builds and file creation
stopped. Main PR300 and its six-ROM/unified/default qualification are preserved;
prototype branches remain guest-disabled. Resume native work only with the
existing disk/output reserve. This dated infrastructure condition does not
reduce the campaign, custom-game, shared-engine or near-30-fps objective.

## Next concrete milestones after the Terra audit

1. Promote the now guest-qualified opt-in late Egg service into one common
   enabled release capability across applicable products, with full campaign,
   custom and source-permission checks. Its four bounded SRAM continuations do
   not qualify arbitrary saves, full missions, every interleaving or performance.
   Then connect Greater-Spire-gated Guardian morph/cancel and Cocoon lifecycle.
2. Replace the Reaver fallback with one coherent paid Scarab lifecycle, including
   source-placed versus newly trained ammunition, AI stocking, flight/impact and
   saves. Keep partial prototypes gated; source splash may increase Z10 danger.
3. In parallel, re-profile current-source order/detection, movement and
   rendering, then reduce measured busy-loop cost with paired state/pixel and
   gameplay checks. Historical inclusive scopes are not promised savings. The N64
   order gate is a research lead, not permission to
   skip cooldowns, movement, pre-handlers or entire unit updates.
4. Retry Z10 through ordinary controls after the relevant mechanics are usable;
   qualify genuine save continuation and affected carried missions. Preserve the
   existing 29-mission coverage rather than tune difficulty around a substitute
   weapon or injected victory. Independently qualify early Protoss campaign
   save continuation under the combined P10 product.
5. Use a separate bounded reference task for moving-target projectiles and
   stationary/melee release events, then implement a coherent shared lifecycle
   (B2). Construction finish effects, shield feedback and Egg animation follow
   the gameplay and persistence blockers above.

The custom Egg service is an explicit preview across all four delivered
products, with legacy Z7 separately checked. Its static frame and selected-Egg
HUD work; pulse/birth/shadow animation and a producer job list remain V1 work.
No feature or build result changes the 29/30 coverage or near-30-fps requirement.

Native dialogue timing remains a concrete UX mismatch: critical outcome
deferral is merged, but removal of synthetic holds remains staged after P4/P6
controller failures. Native custom-game wallets and AI setup still need direct
evidence; corrected human opening supply does not establish all setup parity.

## Current work and open PRs

| Work | State and next gate |
| --- | --- |
| [#297](https://github.com/ggu/starcraft-gba/pull/297), late-campaign Egg integration | Runtime `f32356c6`, test-only `7166def0`: opt-in Z8–Z10 producer/journal/owner/trigger reservation integration, eight sanitizer targets and four matching-product predecessor cases passed. Actual Z10-product Z8/Z9/Z10 and Z9-product Z9 pause-save/export/fresh-load/natural-birth cases passed. All six normal ROM hashes/RAM ends remain identical to `6ba`; clean default checks pass (1,000 Python tests, 26 skipped, plus compiled/auxiliary suites). Both public preview recipes reproduce the actual guest-qualified ROM hashes at publication `4f0d7080`. See [qualification and limits](audits/2026-11-06-current-native-qualification/late-eggs.md). Common normal activation, Guardian, full campaigns, stack and performance remain open. |
| [#295](https://github.com/ggu/starcraft-gba/pull/295), shared production preview | Merged as `34c1b38f`; whole tree equals reviewed publication `b57351af`. The [Egg-only custom candidate](audits/2026-11-05-shared-egg-custom/README.md) at `6ba76f8e` no longer depends on Guardian. Six normal ROM/RAM checks and clean default tests pass; all five opt-in GBA builds and HOST controller/journal/owner suites pass. The exact Terran ROM has physical-button ARM, genuine fresh-process SRAM and a separate static-frame/HUD visual check. Known pre-cargo Z6/Z7 reader regressions were reproduced and fixed without changing the authenticated inner token. Separate preview commands are available; default service remains off. Campaign integration, remaining guest products, long matches and performance are not thereby qualified. |
| [#290](https://github.com/ggu/starcraft-gba/pull/290), Scourge picker | Merged as `73cc74a5`. Runtime/test head `b3d6f8ef` passed viable producer/worker/site sanitizer controls, selected-product journal checks, six new ROM/RAM builds, full clean-Git default tests (989 Python, 26 skipped, plus compiled suites) and fresh-process loading of an actual prior-code pre-cargo HOST journal. Documentation successor/merge preserve identical runtime/build/test objects. No arbitrary guest-save or hardware claim. |
| [#293](https://github.com/ggu/starcraft-gba/pull/293), normal save-context hardening | Merged as `74922d57`; whole Git tree equals qualified `7123176e`. Normal Z7/Z9 custom and Z9 mission controls reject unsupported Eggs, preserve raw-36 Overlords and older-valid fallback, and retain a genuine prior Z7 paid-Egg journal. All six ROM/RAM checks and clean default checks pass (989 Python tests, 26 skipped, plus compiled suites); pins unchanged. A disk guard stopped the first six-build run before Z10; lossless archival permitted the complete retry. |
| Z10 controller | Ordinary drag boxes fixed command latency; the rally fix removed actual recalls. Upgrades reached 2/2 and improved the force at road leg 6, but the mission still lost. Repeated Sunken rebuilding consumed the economy and failed its bounded prefix; do not extend that losing run. |
| [#262](https://github.com/ggu/starcraft-gba/pull/262), queue cancellation | Held: Z1/Z3/Z4 and T5/T6 ordinary-controller regressions, plus current-control UX/refund review. |
| [#265](https://github.com/ggu/starcraft-gba/pull/265), advisor alerts | Held: burn/combat suppression, one-shots, cooldown initialization, voice policy and current-combination qualification. |
| [#212](https://github.com/ggu/starcraft-gba/pull/212), [#210](https://github.com/ggu/starcraft-gba/pull/210), Carrier/Defiler | Draft/conflicting: namespace collision with transport, missing genuine migration/current qualification; also audit guarded AI purchases. |
| [#146](https://github.com/ggu/starcraft-gba/pull/146), fog | Draft: renderer conflicts and missing current memory/paired ARM qualification. |
| [#315](https://github.com/ggu/starcraft-gba/pull/315), late-Zerg packed grid | Merged as `738223a4`; scoped tests and emulator timing are recorded above. The later #323 package retains this capability, with a different binary; its speed has not been remeasured. Strict current Z7 winning validation, broader campaign/skirmish and hardware qualification remain open. |
| [#318](https://github.com/ggu/starcraft-gba/pull/318), research journal groundwork | Merged as `f34004af`, default-off. Full default suite and current flag-off composition passed; feature-on current-base and functional research qualification remain open. The default-off groundwork alone did not change package bytes; #323 is included in the newer normal package. |
| SD delivery | Latest verified normal Mac package is `c96fc5e2` (combined menu/mining/audio source), with a player testing guide and independent 40-file read-back; last verified KODAK package is `e9838086` plus the separate custom Egg preview. Card detection/copy is pending. Never claim the newer build is on the card without read-back verification. |

## Verification and maintenance

- Update this file after meaningful findings, tests, merges or release changes.
  Keep stable task IDs; add new confirmed gaps, retire disproved findings, and
  distinguish partial proof from closure. Reprioritize after evidence changes.
- Link detailed source-only audits instead of copying private ROM data, images,
  audio or saves. Consult [roster status](SC64_SKIRMISH_ROSTER_STATUS.md),
  [shared transport](SHARED-TRANSPORT.md), [validation](VALIDATION.md), and the
  dated audits for the actual scope of each earlier result.
- Verify all four delivered products and the extra Z7/Z10 qualification builds
  when shared changes affect them. Keep generated inputs pinned and unchanged;
  serialize native builds, tests and emulation and reserve their output space.
- Run local checks. GitHub Actions remains disabled/manual-only; do not enable
  or dispatch runners without an explicit request. Preserve the user's dirty
  primary checkout and use isolated worktrees for changes.
- Keep failed attempts and scoped measurements. N64 VI, internal game ticks,
  GBA update cycles, emulator fps and wall time are different quantities.
- The full goal remains open until campaigns, supported skirmishes, shared
  mechanics, near-30-fps behavior and reference/hardware checks are proven
  together on the delivered builds.
