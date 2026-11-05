# StarCraft GBA goal and prioritized TODOs

Updated: 2026-11-05. This is the living backlog for the race-ROM/N64-parity goal.
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

- Canonical branch: `codex/source-release`, observed at `b2d18ece` (production
  code equivalent to `c96c84b7`; the later merge is documentation).
- Latest verified Mac package: `20261105-c96c84b7-campaigns`: Terran 1–10,
  Zerg 1–9, Protoss 1–10, plus Showcase. Content coverage is **29/30**;
  this is not a claim of complete native mechanics or campaign parity.
- Campaign products retain Outpost/Blood Bath custom games, all human races
  and 1–3 independently raced AIs. Larger roster and behavior gaps remain below.
- The current normal P7 measured window costs about **2,197,886 cycles per
  full loop**, versus about **559,000 cycles** available at 30 fps. Intrusive
  attribution identifies substantial setup/optical, movement and rendering
  work. These are bounded emulator measurements, not hardware certification.
- Shared transport is enabled in normal release vectors. Its availability,
  AI use and native behavior still need scoped checks; do not describe the
  entire service as absent based on older documents.
- N64 Depot observations repeated a 1,514-VI interval from visible mineral
  debit to supply-cap change. This is **not an internal game-update measurement**.
- Candidate N64 SCV record and timer offsets remain unvalidated. Do not use
  them to change production cadence until command correlation, fresh-reset
  checks and an independent game clock establish their meaning.

## Priority order

Priorities are the maintainer's decisions after the GPT-6.1 Terra audit and
current-code review. Parallel work is appropriate where source ownership and
the serialized native-test lock permit it.

| ID | Priority / state | TODO and reason | Completion evidence required |
| --- | --- | --- | --- |
| C1 | P0 / next | Trace Z10 worker damage, economy and army losses. Current trials lose; Reaver proximity alone is not damage attribution. Stop blind strategy sweeps. | Bounded ordinary-input trace identifies actual attacker, damage event, target, costs and production; separate construction-consumed Drones from kills. Compare the relevant N64 behavior before tuning difficulty. |
| C2 | P0 / open | Complete shared Reaver/Scarab behavior. Campaign Reavers currently use a generic 100-damage attack; custom Reaver purchase is blocked while the paid ammunition service remains staged. | Source/native ammunition and attack-event checks; paid training, capacity, launch, flight, terrain/target loss, splash, death/slot reuse, save/load and AI purchases across applicable products. No generic free shot as a substitute. |
| S1 | P0 / in progress | Finish shared Egg/Guardian production and candidate-local save validation. Independent paid Eggs are needed outside Z7; combined formats remain experimental. | All real profile codecs and journals, interleaved tag contexts, old raw-36 Overlords, bad-newer fallback, atomic rejection, paired births, trigger reservations and Carrier/Egg capacity boundaries. Then ARM/RAM/stack, ordinary controls and genuine SRAM continuation before common activation. |
| S3 | P0 / open | Qualify saves throughout each combined campaign, including early Protoss missions under the P10 product. The P10 default audit reports no P1–P7 continuation; later single-save-point results do not close that gap. | Fresh-process ordinary save/load and continued wins across all campaign entries on current combined flags, with source-owned effects, actors, clocks and objectives preserved. Reproduce and fix each rejection; do not infer arbitrary-save support from one point. |
| C3 | P0 / open | Finish Z10 through ordinary controls and retain all other 29 missions. Rally recall and inefficient selections were fixed in the controller, but no Z10 win exists. | Fresh ordinary-input win, Temple/crystal sequence, text/results, save prefix, fresh-process continuation and ARM replay on the integrated engine; rerun affected carried missions. Never inject a win, resources, units or SRAM. |
| N1 | P1 / in progress | Validate N64 actor records, game clock and event cadence. This is a prerequisite for behavior-changing timing optimizations. | Explicit select/move/stop correlation for independent actors, fresh reset, owner/type/position validation, independent clock and writer-PC evidence. Bound read-only snapshots; no guest writes or MMIO side effects. |
| P1 | P1 / open | Reduce representative busy-frame cost toward 30 fps. Small isolated gains do not close the current gap. | Exact-source normal/diagnostic pairs on P7/P10 and 1-/3-AI custom games, advancing combat frames, ordinary input and modal handling. Measure update/render and worst-frame distribution; require material improvement without mode regressions. Confirm on GBA hardware. |
| C4 | P1 / open | Replace campaign AI scalar approximations where verified source/native behavior differs. Late-Zerg waves use averaged groups/gaps; arrival, rebuilding and transport tactics are not equivalent to script commands. | Source script interpretation plus N64 observations of actual production, wallets, launch groups, targeting and rebuilds. Reproduce a concrete mismatch before changing AI; retain mission objective semantics. |
| U1 | P1 / blocked on correction | Correct campaign product permissions before merging the Scourge picker fix. ROM PUNI permits human Scourge in eight Zerg missions, but denies it in Z1 and Z5. | Picker and production agree with source owner/mission restrictions; denial is nonspending; custom games retain their full supported catalog. Recheck early missions, saves and the six-ROM matrix. Audit PUNI/UPGR/TECx generally, including P10. |
| U2 | P1 / open | Integrate Carrier/Interceptor behavior for supported ordinary units and P10 Gantrithor. The current Gantrithor can move but does not launch Interceptors. | Paid children, flight, target/parent death, capture/owner changes, reload, capacity and full profile save matrix; P10 hero and custom-game ordinary controls. Resolve held Carrier namespace and merge conflicts first. |
| S2 | P1 / open | Maintain one shared capability and persistence contract across products. Transport is present; actor mappings, research restrictions, repair, detection, morphs and remaining abilities still need a current matrix. | Per-product and per-mode inventory from actual flags/call sites, source-owned permissions, lifecycle/death/capture checks and every applicable save codec. No Showcase-only engine fix or mission-objective predicate used as a service gate. |
| K1 | P1 / open | Qualify complete supported custom games for all race combinations and 1–3 AIs. Validate building/unit progression, economy, targeting, victory and long-game stability. | Both maps, each human race, homogeneous and mixed AI races, actual tech progression and save/load. Track global pool/queue pressure and ensure human production is not silently starved. |
| M1 | P1 / open | Keep expanded mechanics within GBA memory and global actor capacity. Z10 has reached the 240-actor pool, preventing its former 70-Hydralisk launch goal. | Unchanged safety reserves, ROM/EWRAM/IWRAM and stack checks, child/queue/trigger capacity accounting and sustained busy play. Document adaptation limits; do not present catalog capacity as native-scale simulation. |
| B1 | P2 / open | Finish combat-rule and attack-event fidelity: fractional HP/shields, armor/size/upgrade effects, Firebat collateral, appear-at-target wind-up, shield-hit effects and specialized weapons. | Verify current code first; compare source/N64 events, not only appearance. Exercise allies, invincibility, shields, deaths and impact saturation with persistence and clipping. See [combat audit](ENGINE-FIDELITY-AUDIT.md). |
| V1 | P2 / open | Audit remaining source sprites, animations and placement/order effects, including each supported unit/building, damaged buildings, corpses, resource cargo and construction poses. | Paired N64/GBA observations, correct source frames/anchors/palette/timing, no missing assets, scene/HUD clipping and crowded effect/VRAM reuse checks. Existing fixes must be checked before reopening old reports. |
| V2 | P2 / open | Improve blocky fog/terrain boundaries without violating RAM/VRAM budgets or making busy frames slower. | Current-source paired visual/performance evidence, both maps/cameras, explored versus visible semantics, sprite/audio/impact compatibility. Rework the held fog draft rather than enabling a losing renderer. |
| Q1 | P2 / open | Resolve controls/advisor PR regressions and polish N64-informed UX: non-destructive deselection, queue cancellation/refunds, alert suppression, camera bookmarks, settings and results. | Ordinary-input campaign/controller matrix and updated replays, visible hints matching input, native alert/control observations where relevant, persistence and no paid-order surprises. |
| R1 | P1 / ongoing | Publish only qualified source, merge ready PRs, and package versioned ROMs with accurate descriptions. | Local source/default/prepared/ARM checks appropriate to each change; recorded exact head and results. No unnecessary Actions. Preserve old saves, verify SD read-back hashes and safely eject when copying is possible. |

P0 items are completion/behavior blockers, not permission to skip their memory,
save, source-boundary or regression gates. N1 and P1 should proceed alongside
campaign work; late cosmetic work should not displace them.

## Current work and open PRs

| Work | State and next gate |
| --- | --- |
| [#289](https://github.com/ggu/starcraft-gba/pull/289), shared Egg HOST research | Draft. Seven targeted HOST checks and six StageOFF byte comparisons passed on the documented source. Current combined Egg/Guardian successor is separate; P10 initially rejects its save. Fix the actual predicate and finish all four profiles before promotion. |
| [#290](https://github.com/ggu/starcraft-gba/pull/290), Scourge picker | Draft. Z8/Z9/Z10 picker and selected-product journal tests passed. The all-campaign source audit subsequently found Z1/Z5 denials; correct those before the release matrix and merge. |
| Guardian candidate-local decoder | Guardian-only four-profile journals, broad failed-load snapshots and mixed old raw-36/newer-invalid fallback passed. Combined Egg/Guardian admission is not thereby qualified. |
| Z10 controller | Ordinary drag boxes fixed command latency; the rally fix removed actual recalls. Upgrades reached 2/2 and improved the force at road leg 6, but the mission still lost. Repeated Sunken rebuilding consumed the economy and failed its bounded prefix; do not extend that losing run. |
| [#262](https://github.com/ggu/starcraft-gba/pull/262), queue cancellation | Held: Z1/Z3/Z4 and T5/T6 ordinary-controller regressions, plus current-control UX/refund review. |
| [#265](https://github.com/ggu/starcraft-gba/pull/265), advisor alerts | Held: burn/combat suppression, one-shots, cooldown initialization, voice policy and current-combination qualification. |
| [#212](https://github.com/ggu/starcraft-gba/pull/212), [#210](https://github.com/ggu/starcraft-gba/pull/210), Carrier/Defiler | Draft/conflicting: namespace collision with transport, missing genuine migration/current qualification; also audit guarded AI purchases. |
| [#146](https://github.com/ggu/starcraft-gba/pull/146), fog | Draft: renderer conflicts and missing current memory/paired ARM qualification. |
| SD delivery | Latest verified Mac package is c96; last verified KODAK copy is f7. Card detection/copy is pending. Never claim the newer build is on the card without read-back verification. |

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
