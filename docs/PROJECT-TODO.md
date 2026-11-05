# StarCraft GBA goal and prioritized TODOs

Updated: 2026-11-06. This is the living backlog for the race-ROM/N64-parity goal.
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

## Current baseline

- Canonical branch: `codex/source-release`. The source-permitted Scourge/Spire
  correction in #290 and save-context hardening in #293 are retained. #295
  adds the explicit shared Egg preview; its normal product flag remains off.
- Latest verified normal Mac package: `20261106-e9838086-campaigns`: Terran 1–10,
  Zerg 1–9, Protoss 1–10, plus Showcase. Content coverage is **29/30**;
  this is not a claim of complete native mechanics or campaign parity.
- Separate opt-in Mac package: `20261106-e9838086-custom-eggs-preview`.
  Versioned filenames and manifests distinguish it from the normal products;
  its shared Egg service applies to custom games, not Z8–Z10 production.
- Campaign products retain Outpost/Blood Bath custom games, all human races
  and 1–3 independently raced AIs. Larger roster and behavior gaps remain below.
- The [current-source P7 measurement](audits/2026-11-06-current-native-qualification/README.md)
  at `72a71d22` costs about **2,200,226 cycles per advancing full loop**, versus
  about **559,000 cycles** available at 30 fps. The paired diagnostic matches
  90 sampled state/pixel checkpoints and adds 3.32% overhead; order/Arbiter
  setup, movement and rendering remain major scopes. These replacement-main
  emulator measurements are not hardware certification or a new speed gain.
- A fresh ordinary-input N64 Blood Bath Zerg session displays Drone cost
  **50 minerals / 1 supply**, then supply **4/9 → 7/9** and Cancel after a
  command to three selected larvae. Birth, refund and exact timing/debit were
  not observed; the same current-source report records the limits.
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
round used existing N64 evidence and current source; it ran no fresh emulator
comparison. Parallel work is appropriate where source ownership and
the serialized native-test lock permit it.

| ID | Priority / state | TODO and reason | Completion evidence required |
| --- | --- | --- | --- |
| C1 | P0 / in progress | A [paired 31k-call HOST trial](audits/2026-11-05-z10-damage-attribution/README.md) now attributes 12 lethal Drone hits and 63 lethal Hydralisk hits to the generic Reaver path. Trace/control and original-input sequence matched; all 99 observed AI unit purchases paid their expected costs. | Finish source attribution for generic placeholders, construction removals, target priorities and income; compare relevant native behavior. This HOST-stage result is not a guest win or proof that native ammunition alone changes Z10's outcome. |
| C2 | P0 / open | Complete shared Reaver/Scarab behavior. Campaign Reavers currently use a generic 100-damage attack; custom Reaver purchase is blocked while the paid ammunition service remains staged. | Source/native ammunition and attack-event checks; paid training, capacity, launch, flight, terrain/target loss, splash, death/slot reuse, save/load and AI purchases across applicable products. No generic free shot as a substitute. |
| S1 | P0 / in progress | Finish shared Egg/Guardian production. The [opt-in late service](audits/2026-11-06-current-native-qualification/late-eggs.md) now has four actual ARM campaign Egg/SRAM continuations; normal activation across products and Guardian gameplay remain unfinished. | All real profile codecs and journals, interleaved tag contexts, old raw-36 Overlords, bad-newer fallback, atomic rejection, paired births, trigger reservations and Carrier/Egg capacity boundaries. Then ARM/RAM/stack, ordinary controls and genuine SRAM continuation before common activation. |
| S3 | P0 / open | Qualify saves throughout each combined campaign, including early Protoss missions under the P10 product. The P10 default audit reports no P1–P7 continuation; later single-save-point results do not close that gap. | Fresh-process ordinary save/load and continued wins across all campaign entries on current combined flags, with source-owned effects, actors, clocks and objectives preserved. Reproduce and fix each rejection; do not infer arbitrary-save support from one point. |
| C3 | P0 / open | Finish Z10 through ordinary controls and retain all other 29 missions. Rally recall and inefficient selections were fixed in the controller, but no Z10 win exists. | Fresh ordinary-input win, Temple/crystal sequence, text/results, save prefix, fresh-process continuation and ARM replay on the integrated engine; rerun affected carried missions. Never inject a win, resources, units or SRAM. |
| N1 | P1 / in progress | Validate N64 actor records, game clock and event cadence. This is a prerequisite for behavior-changing timing optimizations. | Explicit select/move/stop correlation for independent actors, fresh reset, owner/type/position validation, independent clock and writer-PC evidence. Bound read-only snapshots; no guest writes or MMIO side effects. |
| P1 | P1 / open | Reduce representative busy-frame cost toward 30 fps. Small isolated gains do not close the current gap. | Exact-source normal/diagnostic pairs on P7/P10 and 1-/3-AI custom games, advancing combat frames, ordinary input and modal handling. Measure update/render and worst-frame distribution; require material improvement without mode regressions. Confirm on GBA hardware. |
| C4 | P1 / open | Replace campaign AI scalar approximations where verified source/native behavior differs. Late-Zerg waves use averaged groups/gaps; arrival, rebuilding and transport tactics are not equivalent to script commands. | Source script interpretation plus N64 observations of actual production, wallets, launch groups, targeting and rebuilds. Reproduce a concrete mismatch before changing AI; retain mission objective semantics. |
| U1 | P1 / partial | Scourge/Spire source restrictions and shared picker correction are merged in #290. Continue the broader campaign PUNI/UPGR/TECx audit, especially P10. | Preserve current controlling ownership, nonspending denial and full supported custom catalog. Apply remaining source permissions with per-owner tests and save compatibility; do not infer complete availability parity from this one product fix. |
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
| SD delivery | Latest verified normal Mac package is `e9838086`, with a separate custom Egg preview; last verified KODAK copy is f7. Card detection/copy is pending. Never claim the newer build is on the card without read-back verification. |

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
