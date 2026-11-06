# StarCraft GBA goal and prioritized TODOs

Updated: 2026-11-06. This is the living backlog for the race-ROM/N64-parity goal.
Historical audits remain evidence snapshots; they are not the current task list.

## Goal and completion rules

Source-work checkpoint: the [shared production follow-up](audits/2026-11-06-shared-production-source-checkpoint/README.md)
tracks paid Reaver tech progression, source-town wallet routing, typed Scarab
velocity and optional Cocoon rendering. Space recovered and their focused HOST
checks now pass. Staged P10 and Z7 HOST controllers also complete paid Reaver
progression and attack with fresh-process impact continuation; guest integration
and the full persistence and product matrix remain open. These branch changes do not change release
capability, campaign coverage or performance claims.

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

## Latest qualified and queued work

Resume checkpoint (2026-11-06): free space recovered to 154 GiB. The former
`/tmp` qualification worktrees and runner were removed during cleanup; published
source branches are being restored into persistent isolated checkouts. Historical
test results below remain historical evidence, not fresh reruns. Recheck retained
private input hashes before rebuilding; do not regenerate pinned inputs merely
to replace a missing test prerequisite. ARM qualification and the repaired
late-Zerg baseline/candidate pixel oracle are the next local jobs, serialized
under the existing native-build lock. No flags have been promoted.

A fresh bounded owned-N64 launch was stopped after ScreenCaptureKit again
returned error -3811 before gameplay could be inspected. No reference input,
RAM snapshot, native event timing or new N64 parity result was obtained. The
failed launch record remains local in `pylon-resume-20261106-140958`.

Fresh recovery results (qualification remains incomplete):

- Terran's explicit shared-actor ARM candidate linked with the unchanged linker
  guards. Z7 failed IWRAM by 696 bytes: staged end `0x030052b8`, limit
  `0x03005000`; the same derived profile without the stage ends at
  `0x03004f68`. The growth is in hot simulation paths, so no guard reduction or
  unmeasured hot-code relocation was accepted. Per-function stack reports are
  not dynamic stack proof. P10's historical flight fact-header was recovered
  with the exact required `8d569be8...` hash. The earlier rejected output used
  the non-flight recipe; it was not substituted. Fresh controller and journal
  migration checks pass. P10's ARM candidate links at IWRAM `0x03004d90` and
  BSS `0x020391e4`, within unchanged guards. Guest SRAM, campaign saves, dynamic
  stack use and hardware speed remain unqualified. See the
  [ARM recovery audit](audits/2026-11-06-shared-actor-arm-resume/README.md).
  A separate Guardian noinline probe saved 176 IWRAM bytes but still failed by
  520; rare-actor extraction alone also remained oversized. The isolated combined
  rare-actor/Egg dispatch plus existing `GBA_DIAMOND_ROM` option links all three
  products: Z7 ends at `0x03004f10` (240 bytes below the boundary), Terran at
  `0x030047f8`, P10 at `0x03004bb8`. Its Z7/P10 HOST controller, journal and gfx
  checks pass, as does the Reaver type-domain check across all three profiles.
  This remains an opt-in probe, not a release: ordinary guest/SRAM behavior,
  diamond-heavy/Cocoon/HUD timing and system/IRQ stack qualification are open.
  See the [placement probe](audits/2026-11-06-rare-actor-iwram-probe/README.md)
  and [stack review](audits/2026-11-06-shared-actor-stack-review/README.md).
- The late-Zerg art bank and Z8/Z9 authored program headers were recovered with
  exact recorded hashes. The repaired renderer runner fails on compilation or
  oracle errors and compares indexed frames plus palettes across binaries.
  Z7 and identity-checked Z9 campaign fixtures passed both reference checks and
  paired equality (91 captures each), with frozen source/input hashes unchanged.
  Expanded custom fixtures initially found six full-clear mismatches in both
  baseline and candidate despite matching paired traces. This was a fixture
  error: drawing mutates walk/facing state, so one static reference cannot cover
  four successive draws. Independent per-render references now start from the
  same animation state and compare post-render walk/facing too. The corrected
  Z7/Z9 custom baseline/candidate pairs pass with production-dispatch parity
  (576 captures each, all available maps/human races and 1/2/3 AI rosters).
  Initial runs used an older frozen Z7 bank; those results are retained with
  that scope. Exact receipt-matched Z4/Z6/canonical-Z7 banks were then recovered
  in isolated directories. All affected campaign/custom HOST pairs and four
  normal ARM links pass again with authenticated banks and unchanged manifests.
  Source/doctor/default build passed. The full 1,004-test Python run had five
  prepared-input/symlink-safety failures. Targeted Zerg roster and safety checks
  now pass; Terran/Protoss cover-encoded inputs remain missing. No full-suite
  pass is claimed. The [renderer audit](https://github.com/ggu/starcraft-gba/blob/codex/late-zerg-render-oracle-20261106/docs/audits/2026-11-06-z7-render-cache-trial/README.md)
  records the complete scope and private input provenance.
- A bounded current Z7 early diagnostic (calls 8,021–9,020) matches 90 sampled
  state/display checkpoints, workload endpoints/minima and mode counts across
  variants. Its 965 advancing-PLAY calls average 1,298,955 cycles without the
  cache flags and 1,236,870 with them (about 4.8% less); 35 nonadvancing calls
  are reported separately. Snapshot work occurs outside the timed calls and
  may affect pacing. This is sampled early diagnostic evidence, not a full,
  late/busy, normal-main or physical 30-fps result. The initial busy HOST seed
  was correctly rejected for zero-filled unused SRAM and fake zero timing;
  that failed evidence remains retained. The HOST backend now starts erased
  once per process and preserves SRAM across soft boots. A newly generated
  ordinary-button seed (cut 136,058, journal SHA `182407d91e782c4e879e65047223a70a0ec5f83bbec0131d4e205debddbd0d39`)
  passes 258/258 observations. Fresh actual ARM baseline/candidate loads each
  pass 49/49 resumed observations and 50/50 selected-unit CARGO7 digests through
  the recorded win. The HOST prefix is explicitly untimed; it establishes no
  combined mission timing or full-state equivalence.
- The genuine busy paired diagnostic (calls 136,059–137,058) contains 975
  advancing PLAY calls, 25 nonadvancing calls and no text/modal calls, with
  at least 48 friendly and 169 other units. Ten sampled indexed-frame/palette
  and selected-state hashes match. Advancing means are 2,512,225 cycles baseline
  and 2,446,291 candidate (about 2.6% less), still far above the roughly 559,000
  cycle 30-fps budget. Snapshots outside the timed segment affect pacing; sampled
  hashes are not all-frame/full-state proof, and reported top-tail order samples
  are not exact p95/p99. Keep flags experimental. Same-source control and coarse
  update/render/route attribution builds are now the next measurement, with the
  actual 14 route slots unchanged. The subsequent same-source four-ROM
  control/attribution run matches all ten sampled state/display hashes and
  workload checks. Measured attribution overhead is about +0.03% baseline and
  -0.004% candidate in this window. Baseline advancing means: update 1,812,688
  cycles (orders nested within it: 1,520,699), render/present 700,185 (map nested:
  434,913; HUD: 152,747). Nested scopes must not be added. Route counters show
  3,576 hits and 392 builds; 299/975 advancing calls build at least once, maximum
  seven builds. Build-bearing updates average about 3.83M versus 0.92M otherwise;
  this is correlation, not isolated build cost. Further same-source classification
  matched all prior checks and counted 385 movement builds, zero component-
  reachability builds and seven depot builds; 27 used empty slots and 365
  replaced valid slots. There were 20 bounded distinct keys and 372 repeats,
  with no tracking overflow; one full reset, no age wrap, and 14 usable slots.
  Test the existing exact monotone shortcut against independent FIFO references
  and the same guest window before considering any default change. The worst sampled tick builds
  seven routes and costs 10.11M cycles. Prioritize orders/pathfinding rebuild
  causes ahead of further renderer tweaks, without reducing actors, AI cadence,
  route slots or sight. This remains an after-load diagnostic, not global FPS.
- An opt-in test of the existing exact monotone path witness passed independent
  FIFO-dispatch tests, including the actual packed 128x128 Z7 14-plane (12 when
  two are reserved) configuration and blocked endpoints/partial fallback. In
  the same 1,000-call guest window, baseline advancing mean fell from 2,512,864
  to 1,679,145 cycles (-33.18%); with the five render-cache flags, 2,446,063 to
  1,612,046 (-34.10%). All ten sampled state/indexed-frame/palette checks, tape
  observations, cargo digests, seed and unchanged journal checks matched.
  Route builds fell 392 to 26 (movement 385 to 20), and valid-slot replacements
  365 to five. This changes derived cache state as intended. The worst advancing
  call still costs 8.12M cycles, and the mean is about three times the nominal
  30-fps budget. These are narrow diagnostic measurements, not release speed.
  Guarded diagnostic IWRAM ends at 0x03004fe8 in the uncached variant, only 24
  bytes below the boundary; bounded stack watermark is not all-path/IRQ proof.
  All eight normal Z7-Z10 control/shortcut ARM links pass, but flags remain off
  after the full-replay outcome failure below. Uninterrupted campaign,
  early/warm custom 1/3-bot, save/load and representative ARM qualification.
  See the [measured source branch](https://github.com/ggu/starcraft-gba/tree/codex/z7-host-sram-replay-20261106).
  **Full fresh-start replay rejects promotion:** the control passed 307/307
  recorded observations and won at input tick 175,455; the unchanged tape on
  the shortcut variant lost at tick 16,030. The first selected actor-state
  fingerprint difference is call 817 (world/display hashes still matched then).
  The narrow post-load speed result does not qualify this change. Preserve all
  failed evidence and keep the flag disabled. Trace the first differing route
  output and cache/reuse state; standalone FIFO witness tests cannot prove that
  changing field allocation/eviction preserves the existing stateful dispatcher.
  Do not remove state checks or change controller inputs to conceal divergence.
- The fitting shared-actor stack diagnostic completed real custom START/B save
  and fresh HOME-L continuation with matching candidate/diagnostic SRAM and
  named states. At least 9,740 of 11,208 painted bytes remained untouched in the
  bounded load/resume path; this is not an IRQ/BIOS/all-path stack bound. New
  campaign saving still reports SAVE FAILED, and the genuine older campaign
  journal is rejected with load error 89. Preserve both safety gates. The full
  30-mission persistence audit identifies campaign context, actor origin,
  reservations and atomic migration as required work; the exact error-89
  rejecting predicate is now reproduced at save selection: the genuine
  predecessor tag is `0x56435a54` with a 16,312-byte payload, whereas this staged
  Z7-derived selector accepts different authenticated namespaces and has a
  14,632-byte scratch limit. Both tag and length reject before prepare/preflight.
  Retagging/truncation cannot migrate it; implement an authenticated predecessor
  decoder and explicit campaign policy. This corrects the earlier unproven
  hypothesis that the custom Egg validator alone explained error 89.
  The exact producer source and all 418 retained source/input manifest entries
  are verified. Its packed RaceSave is 16,296 bytes plus a 16-byte trailer, with
  35-byte unit and 28-byte building rows; the candidate has 29/25-byte rows.
  Existing 32,768-byte scratch may hold detached old and new records together
  without increasing EWRAM, but aliasing/admission and authenticated decoding
  must be implemented and qualified before any migration is enabled.
- The current Z8 controller pair reproduced fault-free losses: common service
  at frame 11,048 and service-off at 21,782. The first sampled state difference
  is an additional Egg, consistent with the intended lifecycle, not proof of a
  defect. Trace-identical telemetry found defenders initially outside weapon
  range and selection/chasing commands consuming substantial simulation time.
  Count-only, target-only and combined controller trials failed to win; the
  failed changes were reverted and evidence retained. See the
  [Z8 bounded audit](audits/2026-11-06-z8-controller-baseline/README.md).
  Two further single-change HOST trials (attack-move repost and an earlier
  northeast guard position) also lost at frame 11,048, fault-free. No controller
  change was retained. Idle actors already have within-range acquisition in this
  product. A read-only per-eight-frame observer reproduced the baseline trace
  byte for byte and confirmed Hydra firing/cooldowns, 64 runtime-pixel reach
  (128 source pixels) and depletion of the raider's 80 shields before HP loss.
  Retaining accepted direct-target orders alone also lost, at frame 11,009;
  no controller or engine change is retained. No qualified win follows.
- The user confirmed the screen is unlocked. After reconnecting, native text
  access worked but screenshot capture timed out; N64 gameplay comparison is
  still unavailable. A subsequent native-screenshot attempt could not bind the
  emulator through CUA (error -3811), so no keys were sent; that owned session
  was stopped and confirmed terminal. The disk blocker is resolved and reserve
  guards stayed enabled. GitHub's workflow is verified `disabled_manually`.

Earlier review findings (resolutions and remaining scope are above):

- The runner's false-pass risks were corrected: failed compilation/tests now fail the run; baseline/candidate indexed frames and palettes are compared directly; real Z9/custom identities and lifecycle-driven fog/minimap transitions are covered by the bounded fixtures. These do not establish full gameplay or speed.
- The explicit ARM shared-actor candidate remains experimental: race_save_adapter.h rejects campaign-mode saves under the full staged codec. Preserve that safety gate until authenticated campaign writing and reservation are implemented and qualified; custom-only tests cannot close campaign persistence.
- Cocoon art/rendering is still HOST-only. ARM links alone cannot establish correct Cocoon visuals. Per-function stack-usage files and linker margins also do not prove nested runtime stack safety; retain actual ARM stack, ROM-size/header, unchanged-normal-build and guest SRAM checks.
- The earlier ENOSPC interruption is resolved. Qualification resumed from published sources in persistent isolated checkouts; no release or SD update follows merely from the recovery.


After space recovered, the following local sanitizer checks passed:

- Combined source `a32d87f`: full P10-typed and Z7-derived button-controlled
  Reaver tech/training/ammunition, real-opponent attack and genuine fresh-process
  pending-impact continuation with confirmed enemy damage. Initial ample funds
  and idle workers remain declared HOST setup, not an unassisted economy proof.
- `4f4e0b5a`: independently compiled genuine feature0/1 predecessor writers,
  legacy raw36 Overlord, malformed newer-slot fallback and atomic load rejection.
  Unsupported feature5/7 cases are fully retagged adversarial records, not
  historical producer claims. Their exact-profile fences preserve other genuine
  compact/Terran migrations.
- `51a78266`/`fbb8511b`: paid AI stocking selects active custom bot slots,
  including the second slot and excluding closed slots. Terran/Z7/P10 fixtures
  cover every prepared map, 1–3 bots, human/neutral/inactive owners and insufficient
  funds. This corrects eligibility; it does not qualify AI build orders or native
  replenishment policy. Consolidated source and receipts are at `e9c3af3b`.

The repeated source-tick optical-cell cache proposal was rejected: only
409/3,947 refreshes (10.36%) reused a cell in the verified paired window.
The busy current-normal loop still averages about 2.20 million cycles. No
performance gain or 30 fps qualification follows from attribution timers.

Next qualification order:

1. Qualify the isolated fitting shared-actor probe beyond linking: real guest
   controls and fresh SRAM, system and IRQ stack use, diamond-heavy/Cocoon/HUD
   timing, campaign-save policy and complete art. Preserve the unchanged guards;
   all three probe links and bounded HOST gates are prerequisites, not promotion.
2. Qualify render-cache flags on representative current early and busy Zerg
   windows. HOST pairs and ARM links pass; the bounded early cycle diagnostic is
   preliminary. Genuine busy save/ARM continuation and sampled paired cycles now
   pass, but the gain is only about 2.6%. Attribute the remaining update/render
   cost with same-source controls, quantify instrumentation overhead and route
   builds/hits, then optimize the measured dominant work. Require equivalent
   workload and state/display checks before promotion. P10 already has these
   flags; Z7–Z10 do not.
3. Implement authenticated campaign persistence for the full actor codec across
   all 30 mission contexts, including source-placed and paid actors, owner
   transfer, pending births/reservations, predecessor migrations and atomic
   rejection. Do not weaken custom-only validators to admit campaign records.
4. Continue AI/research/source-owner and campaign mechanics, preserve carried
   missions and finish Z10 normally. Repair native capture/input access and
   collect the still-missing N64 gameplay comparisons.

The earlier disk interruption and background-I/O observations are historical;
free space has recovered. No personal data or unrelated service was changed.
ScreenCaptureKit still fails before N64 reference input; the owned emulator
sessions exited and supplied no new gameplay proof.
The clean canonical source-only HOST SRAM/replay patch passed source boundary,
HOST save/selection targets, replay-validator tests, default build and full
`./dev test` (449.68 seconds). These checks are independent of the experimental
actor branches and their known private-input/default-suite failures. The focused
patch merged as [PR #303](https://github.com/ggu/starcraft-gba/pull/303) at
`f8d890077fa03ef16bf73dfe2639ead0c28e5d38`; no experimental runtime flags are included.

Official ares v148 source inspection (tag commit
`0aafd85789215e84e1e43415c07d4c88461b7899`) found internal emulated-controller
and platform video callbacks, but no turnkey supported headless CLI. A separate
source-integrated frontend is a research path, not a working N64 reference
runner. Vulkan/paraLLEl availability, threaded video synchronization and cadence
must be verified before any screenshot or gameplay parity claims. No OS event
shim or guest RAM writes are part of that proposed harness. An isolated opt-in
frontend now links against the pinned source/MoltenVK and ran one neutral boot,
but produced no frame before its original callback timeout. Exact source
inspection explains that inactive VI returns without calling screen->frame;
callbacks and run steps must be separately bounded and recorded. A corrected
probe now produces a genuine 640x240 nonblack Expansion Pak / Rumble Pak
startup image and then a readable Single-Player / Terran / Episode I main menu
with paraLLEl-RDP enabled. The root reviewer visually inspected both captures.
Startup gaps and 1x1 null-scanout frames
are recorded separately, not treated as game frames. This establishes useful
capture plumbing and ordinary boot, not campaign/skirmish/engine parity or
native hardware timing. Observed button polls are logged, but the earlier
Start/A schedule ran before menu input could visibly respond; it proves no
menu transition. Next enter a mission/custom game through ordinary emulated
buttons, capture real gameplay and verify the relevant mechanics visually.
Processed framebuffer geometry/aspect and emulator renderer equivalence also
remain separate from asset or behavior parity.

All checks remain local; GitHub Actions is still disabled. No experimental
release flags or campaign coverage were promoted. The later verified KODAK copy
of existing playable packages is recorded in the delivery row below.

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
| S3 | P0 / open | Qualify saves throughout each combined campaign. P1–P7 now have actual early frame-31 saves, old-SRAM migration and neutral continuation to frame 1,031 on the candidate P10 product; active battles, other save points and continued wins remain unqualified. | Fresh-process ordinary save/load and continued wins across all campaign entries on current combined flags, with source-owned effects, actors, clocks and objectives preserved. Reproduce and fix each rejection; do not infer arbitrary-save support from one point. |
| C3 | P0 / open | Finish Z10 through ordinary controls and retain the other 29 implemented mission entries. Current candidate ordinary controllers cover 28 wins after the Z4 picker fix; Z8 remains an existing-content gate and Z10 has no ordinary win. Content coverage and qualified wins are different counts. | Fresh ordinary-input win, Temple/crystal sequence, text/results, save prefix, fresh-process continuation and ARM replay on the integrated engine; rerun affected carried missions. Never inject a win, resources, units or SRAM. |
| N1 | P1 / in progress | Validate N64 actor records, game clock and event cadence. This is a prerequisite for behavior-changing timing optimizations. | Explicit select/move/stop correlation for independent actors, fresh reset, owner/type/position validation, independent clock and writer-PC evidence. Bound read-only snapshots; no guest writes or MMIO side effects. |
| P1 | P1 / open | Reduce representative busy-frame cost toward 30 fps. Detector-first was rejected after a +0.1074% mean regression with matched checkpoints; route-tail attribution and exact collision/steering work are next. Small isolated gains do not close the current gap. | Exact-source normal/diagnostic pairs on P7/P10 and 1-/3-AI custom games, advancing combat frames, ordinary input and modal handling. Measure update/render and worst-frame distribution; require material improvement without mode regressions. Confirm on GBA hardware. |
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
| [#298](https://github.com/ggu/starcraft-gba/pull/298), common Egg release candidate | Draft, not activated on main. Six product ROMs match qualified previews; six-product input/journal/capacity checks, all Terran/Protoss controllers, eight Zerg controllers after Z4 fix, and finite P1–P7 actual old/new/migrated save continuations pass. Combined `95b30e10` passes full clean default checks (1,000 Python tests, 26 skipped, compiled/auxiliary checks). Z8 ordinary controller is the remaining existing-content gate; Z10 is still unproven. See [exact scopes and rejected performance experiment](audits/2026-11-06-common-egg-release/README.md). |
| [#297](https://github.com/ggu/starcraft-gba/pull/297), late-campaign Egg integration | Runtime `f32356c6`, test-only `7166def0`: opt-in Z8–Z10 producer/journal/owner/trigger reservation integration, eight sanitizer targets and four matching-product predecessor cases passed. Actual Z10-product Z8/Z9/Z10 and Z9-product Z9 pause-save/export/fresh-load/natural-birth cases passed. All six normal ROM hashes/RAM ends remain identical to `6ba`; clean default checks pass (1,000 Python tests, 26 skipped, plus compiled/auxiliary suites). Both public preview recipes reproduce the actual guest-qualified ROM hashes at publication `4f0d7080`. See [qualification and limits](audits/2026-11-06-current-native-qualification/late-eggs.md). Common normal activation, Guardian, full campaigns, stack and performance remain open. |
| [#295](https://github.com/ggu/starcraft-gba/pull/295), shared production preview | Merged as `34c1b38f`; whole tree equals reviewed publication `b57351af`. The [Egg-only custom candidate](audits/2026-11-05-shared-egg-custom/README.md) at `6ba76f8e` no longer depends on Guardian. Six normal ROM/RAM checks and clean default tests pass; all five opt-in GBA builds and HOST controller/journal/owner suites pass. The exact Terran ROM has physical-button ARM, genuine fresh-process SRAM and a separate static-frame/HUD visual check. Known pre-cargo Z6/Z7 reader regressions were reproduced and fixed without changing the authenticated inner token. Separate preview commands are available; default service remains off. Campaign integration, remaining guest products, long matches and performance are not thereby qualified. |
| [#290](https://github.com/ggu/starcraft-gba/pull/290), Scourge picker | Merged as `73cc74a5`. Runtime/test head `b3d6f8ef` passed viable producer/worker/site sanitizer controls, selected-product journal checks, six new ROM/RAM builds, full clean-Git default tests (989 Python, 26 skipped, plus compiled suites) and fresh-process loading of an actual prior-code pre-cargo HOST journal. Documentation successor/merge preserve identical runtime/build/test objects. No arbitrary guest-save or hardware claim. |
| [#293](https://github.com/ggu/starcraft-gba/pull/293), normal save-context hardening | Merged as `74922d57`; whole Git tree equals qualified `7123176e`. Normal Z7/Z9 custom and Z9 mission controls reject unsupported Eggs, preserve raw-36 Overlords and older-valid fallback, and retain a genuine prior Z7 paid-Egg journal. All six ROM/RAM checks and clean default checks pass (989 Python tests, 26 skipped, plus compiled suites); pins unchanged. A disk guard stopped the first six-build run before Z10; lossless archival permitted the complete retry. |
| Z10 controller | Ordinary drag boxes fixed command latency; the rally fix removed actual recalls. Upgrades reached 2/2 and improved the force at road leg 6, but the mission still lost. Repeated Sunken rebuilding consumed the economy and failed its bounded prefix; do not extend that losing run. |
| [#262](https://github.com/ggu/starcraft-gba/pull/262), queue cancellation | Held: Z1/Z3/Z4 and T5/T6 ordinary-controller regressions, plus current-control UX/refund review. |
| [#265](https://github.com/ggu/starcraft-gba/pull/265), advisor alerts | Held: burn/combat suppression, one-shots, cooldown initialization, voice policy and current-combination qualification. |
| [#212](https://github.com/ggu/starcraft-gba/pull/212), [#210](https://github.com/ggu/starcraft-gba/pull/210), Carrier/Defiler | Draft/conflicting: namespace collision with transport, missing genuine migration/current qualification; also audit guarded AI purchases. |
| [#146](https://github.com/ggu/starcraft-gba/pull/146), fog | Draft: renderer conflicts and missing current memory/paired ARM qualification. |
| SD delivery | On 2026-11-06, copied and SHA-256 read-back verified all four normal `e9838086` ROMs at `Games/Development/StarCraft-2026-11-06/` on KODAK. Its `Experimental-Eggs/` contains the two `4f0d7080` late-Egg Zerg previews (Z9 and unproven Z10 candidate). Controls, checksums and copy manifest are included; existing ROMs/saves were preserved. Today's actor/memory/render probes were not deployed. Local copy receipt: `.local/qualification/20261106-kodak-copy-receipt.json`. |

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
