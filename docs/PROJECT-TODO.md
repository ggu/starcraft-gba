# StarCraft GBA goal and prioritized TODOs

Updated: 2026-11-13. This is the living backlog for the race-ROM/N64-parity goal.
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

## Current integration priorities

- **KODAK delivery (2026-11-13):** Terran T10, Zerg Z10, Protoss P10 and
  Showcase built from runtime `cbba1c5` were copied to
  `Games/Development/StarCraft-2026-11-13`. All four 32 MiB files were
  read back from the mounted card and matched their build SHA-256 values;
  the card folder has `COPY-VERIFIED.txt` and `START-HERE.txt`. The files
  include the shared command-menu speedup. Older delivery notes below are
  historical. Existing saves and older ROMs were retained. Next: test B-menu
  scrolling and battle speed on a physical GBA SP.

- **One ten-mission cartridge per race:** the default build and release
  profiles are Terran T10, Zerg Z10 and Protoss P10. All ten Zerg controllers
  win under the Z10 flags; its 234 custom startup/save cases and 18 complete
  ordinary-input skirmish runs are covered separately. Explicit `zerg-z9`
  keeps its original ROM identity and save path. These results establish
  campaign availability and selected play paths; frame budget, broader save
  coverage, and original N64 behavior remain separate gates below. The normal
  Protoss cartridge now has a [full P10 win from HOME](audits/2026-11-13-p10-normal-home.md)
  with the complete briefing, authored dialogue and all three heroes alive.
  The exact KODAK P10 ROM reproduces that win at 8.79 emulated updates per
  second and from one fresh mid-mission SRAM load. The exact KODAK Z10 ROM
  wins [Z1](audits/2026-11-13-z1-normal-home.md) and
  [Z2](audits/2026-11-13-z2-normal-home.md) from HOME, matching 51 and nine
  observations respectively, including the briefing and authored pages.
  Their emulator rates are 23.74 and 21.86 updates/s. The exact KODAK P10
  cartridge [also wins P9 from HOME](audits/2026-11-13-p9-normal-home.md)
  with 59 matching checks, both Cerebrate objectives and both heroes alive.
  Its replay averages 8.27 emulated updates/s. P1–P8 still need equivalent
  qualification.

- **Independent custom opponents (K1):** the owned N64 Blood Bath lobby
  confirmed separate named, Random, and Closed seats and a middle-only
  start. The [GBA seat changes](audits/2026-11-12-skirmish-seat-options.md)
  pass physical button and fresh SRAM checks on Terran, Z10, P10, and
  Showcase. [Native opening actors](audits/2026-11-12-n64-blood-bath-opening.md#sparse-seat-spawn-positions)
  establish the seeded Melee draw for one, two and three opponents on
  Blood Bath, including two delayed launches that changed the locations.
  The source-only selector matches all observed native seeds. The optional
  GBA preview uses its elapsed-time clock and preserves resolved starts in
  SRAM. Four ARM previews pass physical-button save, fresh load and
  restart checks. Real hardware and default promotion remain open.
  Exact normal builds after the preview merged pass the [repeatable
  physical seat and SRAM checks](audits/2026-11-12-skirmish-seat-options.md#repeatable-physical-seat-checks)
  on all four products. Keep full matches and hardware frame rate as
  separate qualification. The [ordinary-profile Blood Bath preview](audits/2026-11-12-skirmish-seat-options.md#ordinary-four-profile-blood-bath-preview)
  matches the last-open CPU edit and passes 24 physical same-build checks
  and 20 cross-build checks. The old ROMs reject Melee saves as intended.
  Keep the flag off while full matches and native elimination are open.
  The stock launch resolves Random race rows before drawing starts; the
  pure sequence matches native Human Random, one/two CPU Random rows,
  and a joint Human Random / two CPU Random capture. The Blood Bath
  preview follows that order and now offers Human Random; Outpost and
  defaults keep named human choices. The Terran ARM preview passes
  physical-button Human and CPU Random save/restart checks, and the
  prior normal build reads those saves.
  The [native Game Type row](audits/2026-11-12-n64-blood-bath-opening.md#single-player-game-type-row)
  cycles Melee and Free for All on Challenger and Blood Bath, with no
  difficulty or team row in either lobby. The [paired native launches](audits/2026-11-12-n64-blood-bath-opening.md#paired-blood-bath-launches)
  show CPUs open allied in Melee and hostile in Free for All; the same
  seed and named seats produce identical starts in both. The shared
  Blood Bath preview now offers both modes and preserves earlier Free
  for All saves. All four preview ARM products pass guarded links and
  physical save/load; on Terran, both modes also pass restart and the
  actual CPU-to-CPU target predicate matches the native distinction.
  A [matched native Random pair](audits/2026-11-12-n64-blood-bath-opening.md#paired-random-launches)
  now verifies the same seed, resolved races, start markers and three
  later RNG states in Melee and Free for All for Human Random and
  CPU Random / Random / Protoss. A second pair matches a named Terran
  human with only the middle CPU set to Random, including the same
  resolved race, starts and RNG state. Other sparse lineups and seeds
  remain unobserved.
  The [native Blood Bath end-rule audit](audits/2026-11-13-n64-blood-bath-end-rules.md)
  traces both modes to defeat at zero buildings and victory at zero eligible
  opponent slots. Defeat transfers the remaining actors to neutral ownership
  and detaches their AI. A [linked-actor audit](audits/2026-11-13-n64-blood-bath-linked-survivors.md)
  traces the loaded passenger exception and the Carrier/Reaver queue reset.
  A passive replay did not reach elimination. [Draft #445](https://github.com/ggu/starcraft-gba/pull/445)
  tests retirement and neutral cargo on HOST. Its final victory now waits
  for deferred retirement, including a surviving Battlecruiser fixture.
  Before ARM enablement, resolve in-flight damage and simultaneous
  outcomes against native evidence. Next: record
  an active elimination, broaden the native Free for All Random opening
  sample, and qualify complete ordinary-input matches on hardware.

- **Final Zerg mission performance (P1):** the
  [paired normal Z10 ARM replay](audits/2026-11-12-z10-battle-cycles.md)
  measures 1.57M and 2.00M mean loop cycles in two battle windows, against a
  561,792-cycle budget. The rectangular point-flood trial changes the means
  by less than 0.13% and regresses the 64×64 custom battle, so it stays out.
  Even simulation alone exceeds budget in both windows. A 24-call instruction
  profile puts about half of elapsed time in rendering; the late window
  samples spend 218k cycles per update on the sprite blitter and 288k on
  defenses. Prioritize exact Cannon power and target queries, then sprite
  drawing and exploration. Preserve the existing 64×64 route fast path.
  [Late Zerg packed walk frames](audits/2026-11-12-late-zerg-walk-words.md)
  reuse the other products' renderer and save 0.81–2.80% in the Z10 battle
  windows with 24 identical sampled frames and palettes.
  The [split terrain cache](audits/2026-11-12-late-zerg-terrain-vram.md)
  frees 16,080 B of Z10 working RAM. Compared with the packed-walk release,
  it saves 1.33–1.49% with matching replay checkpoints and sampled pages.
  [Combined defense indexing](audits/2026-11-12-defense-combined.md) is
  enabled for Z10 after exact campaign and two complete three-AI ARM
  comparisons. It saves 5.23–7.31% in two Z10 windows and adds about 0.31%
  in the two measured custom windows. Other product defaults are unchanged.
  The [single static key scan](audits/2026-11-12-z10-static-key.md) saves
  1.38–1.60% in two fresh Z10 battle windows and 1.25% in a three-AI custom
  battle with matching game and video samples. Local tile repair gave
  little additional campaign gain and regressed custom play compared with
  one scan. [Exact instruction traces](audits/2026-11-12-z10-route-stalls.md)
  attribute the largest stalls to worker depot reachability, including
  36.0M cycles in one update. Finishing partial fields on retry helped one
  peak and regressed another, so it stays off. [Trying the nearest legal
  depot first](audits/2026-11-12-z10-depot-first-nearest.md) is now enabled
  for Z10. It preserves the full physical campaign and both three-AI custom
  outcomes. On top of the current release, it cuts the two campaign-window
  means by 8.72% and 4.54%, and the maxima from 35.18M to 14.84M and
  38.37M to 3.46M cycles. Next: address the remaining point flood, then
  qualify the optimization separately in other profiles.
  [Packed completed-building art](audits/2026-11-12-packed-buildings.md)
  saves another 1.64–2.19% in the Z10 windows, with matching gameplay and
  sampled video, the same RAM limits, and both three-AI outcomes matching.
  The [remaining point-flood investigation](audits/2026-11-12-z10-route-stalls.md#why-the-first-bounded-route-shortcut-changed-combat)
  showed that skipping a logically resident field changes combat even
  when its first step is exact. Deferring only the field bytes preserves
  the full Z10 campaign and both custom outcomes, but is flat on mean
  cycles. The [exact linear FIFO probe](audits/2026-11-12-z10-linear-point-flood.md)
  reduces the 96-column peak by 45.65% and the early window mean by 1.04%.
  The full Z10 win, both complete custom matches, and four paired physical
  custom/SRAM checks match; all four links pass. First Strike and
  Desperate Alliance exercise seven actual fills on their 96×96 maps
  with matching ARM state and video. The shared option defaults off
  pending hardware checks.
  A [Z7 profile](audits/2026-11-13-z7-wide-point-flood.md) separates the
  final 58.7M-cycle result transition from a 37.55M-cycle gameplay
  update, of which 33.95M are point-field fills. The exact 128-column
  FIFO probe reduces that gameplay update to 21.31M; the full Z7 win
  matches all 253 checkpoints and its mean falls 6.43%. Z10 still wins
  with matching state and video, and both full custom outcomes match. All four
  guarded links pass; 128×128 T4/P10 physical prefixes match and exercise
  the new kernel. Qualify on hardware before promoting the option. Treat
  the result transition separately. A new Z7 instruction sample puts
  routine rendering at 701k cycles and simulation at 462k. The shared
  [command icon fix](audits/2026-11-13-command-icon-scaling.md) cuts open-menu
  update cost 22.93% across four matching normal Z7 samples, with no
  material change outside the menu. Both full three-CPU replays and the
  four normal physical custom/SRAM checks match. Prioritize steady map,
  sprite and simulation work over the selection-ring probe (under 0.12%
  mean savings).
  The [packed mineral probe](audits/2026-11-13-static-mineral-word-draw.md)
  cuts Z7's sampled static resource pass by 18.82% and matches ordered
  pixels on Z7 and both custom tapes. Keep it opt-in pending attribution
  of the Blood Bath menu timing tail and hardware checks.
  The [P10 profile](audits/2026-11-13-p10-command-census.md) finds depot
  facility floods account for 41.94M cycles of the largest gameplay update.
  The shared Command census probe cuts the full normal P10 win's mean
  by 7.42% and a sampled trigger update by 34.97% with matching gameplay
  and sampled video; it leaves depot stalls unchanged. The probe stays off
  pending cross-profile and hardware checks. The [paired P10 routing tests](audits/2026-11-13-p10-route-options.md)
  show wide point FIFO saves only 0.15% overall; nearest-first saves
  2.05% and cuts two individual depot stalls by over 93%, with the full
  win unchanged but different cache contents. Keep P10 defaults stable
  until broader checks. The [exact bounded facility FIFO](audits/2026-11-13-bounded-facility-fifo.md)
  cuts three P10 gameplay stalls by 43–57% and its full mean by 1.12%,
  matching the observed route bytes, outcomes and video. Four guarded links,
  the Z7 window and one 64×64 SRAM continuation pass. Keep it opt-in pending
  composed RAM checks and hardware. The [typed save reader](audits/2026-11-13-typed-save-reader.md)
  cuts the P10 result update from 76.50M to 37.51M cycles with the same
  pause/result journals and fresh-load win. Keep it opt-in until broader
  save and hardware checks. Next: reduce facility grid conversion and the
  routine rendering and simulation costs that still dominate wall time.
  [P10 sight profiles](audits/2026-11-13-shared-sight-phase.md) put exact
  row math at 5.75% of ordinary loop cost. Only 48 of 453 sampled moves
  share an identical footprint, so the phase-only probe stays out.
  The [shared packed-row probe](audits/2026-11-13-shared-packed-sight.md)
  matches 43,000 normal P10 calls, with a 1.78% mean improvement and a
  3.04% late-window improvement. Keep it opt-in: it adds 40 IWRAM bytes
  and has not been replayed through victory or on all four cartridges.
  The [three-option composition](audits/2026-11-13-composed-performance.md)
  passes four guarded links and physical custom SRAM continuations. P10
  still wins and runs at 9.62 emulated updates/s (8.60% fewer cycles).
  Both full Z10 custom outcomes match, but the Blood Bath timing window
  regresses (+0.75% mean, +7.41% max). The traced tail is a facility fill
  with little expansion work. A [64-pop scalar prefix](audits/2026-11-13-small-facility-prefix.md)
  leaves the Blood Bath spike and slows the Outpost window, so it remains
  outside the release tree. A [lazy-row prototype](audits/2026-11-13-lazy-facility-rows.md)
  also stays out: it saves short synthetic queries but slows a far one
  by 33% even with matched placement, which would exceed Showcase's
  available IWRAM. [Read-only facility samples](audits/2026-11-13-facility-fill-populations.md)
  show Blood Bath's six fills use 25–165 pops, while four Outpost fills
  use 1,957–3,239. [Touched-span packing](audits/2026-11-13-facility-output-span.md)
  improves the sampled Blood Bath peak by 1.98%, but raises Outpost's
  peak by 4.51% versus the composition and remains outside the release
  tree. The paired Outpost trace attributes its tail to flip wait;
  measure presentation timing before using it.
  The three-option composition also matches the full Z7 win from HOME
  and the full Z10 win after a fresh SRAM load, with little or no speed
  gain in those runs.

- **First campaign controller journal:** [draft #361](https://github.com/ggu/starcraft-gba/pull/361)
  now composes P1 research, Q8 damage debt and current-controller sidecars in
  an explicit HOST-only Flash format. Fresh-process tests retain an actually
  rescued Nexus's origin 1/current controller 7, a paid Forge job and fractional
  HP; the job completes after reload. Genuine 0x43/44 empty and 0x47 paid
  predecessors migrate without retagging or changing legacy bytes. Semantic
  fallback, failed-load/no-device mutation and writer scratch recovery pass.
  Independent review caught and rechecked a malformed foreign legacy-tag
  ownership gap. A follow-up on `codex/controller-flash-z10-20261107` at `b13deb13`
  also passes actual P1 stale-descriptor/competing-epoch and read-failure
  cases without erase/program or live-state publication. The separate HOST-only
  compound-format stage now saves paid Z10 training, inside and fresh-release
  outside children and reloads them in separate processes, retaining source
  origin, current town, payer and fractional HP. At `78a01699`, the v2 compound
  format also preserves a moving child's full waypoints, packed fractional
  position, heading/velocity and other selected fields through a fresh process,
  then matches an uninterrupted continuation through actual impact/retirement.
  This fixes the discovered five/four-bit route-timer mismatch using an explicit
  960-byte flight section, not truncation or old-format reinterpretation. Bad-CRC
  foreign/v1 headers refuse writing. Malformed/fallback, genuine predecessor
  bridges, partial-write recovery, ARM, hardware and other campaigns remain open.
  Independent review also identified pre-save live Yamato reconciliation as a
  failed-save atomicity issue; the staged fix and negative checks remain scoped.
  Normal upgrades stay off.
- **Reachable custom-game performance now measured:** the released `70e9800f…`
  P10 ROM runs map 1 with human Protoss and independently selected Terran,
  Zerg and Protoss AIs through a natural 4,000-update opening. All next 500
  measured updates advance without modal/terminal states. The passive human
  has four units/one building; the AIs have 41 units/16 buildings at warmup end.
  Mean elapsed cycles are 623,589, p50 568,588, p95 815,012 and maximum
  2,247,514. Orders consume 1,663,094 cycles in the worst update; map work is
  comparatively stable. A diagnostic replay at `15b01778` now attributes the worst orders spike
  chiefly to nearest-depot searches after topology invalidates workers' cached
  homes: 1,512,250 nested search cycles within 1,689,397 diagnostic orders
  cycles. Diagnostic mean overhead is 3.96%; nested scopes cannot be summed.
  An owner-list candidate at `2fed51b4` preserves all 500 observed transitions
  and ten sampled full frames, palettes and selected-state/workload snapshots.
  Mean cycles fall 3.87%, p99 26.35% and maximum 22.40%; original worst-frame
  orders fall 42.34%. Actual in-pass construction invalidation and next-pass
  rebuild tests pass. Shared-default promotion is merged in
  [#364](https://github.com/ggu/starcraft-gba/pull/364), with six normal guarded
  links, clean default checks, a prepared unified build and a genuine
  old-writer/new-reader/resave HOST sequence. Current P10 normal bytes exactly
  match the measured candidate; no save fields, actor caps or cadence change.
  `test-unified` still lacks its documented private BC asset header. The normal
  Mac package is `StarCraft-GBA-2026-11-07-a5064cb2`. This is one opening,
  not combat, all race/map combinations or hardware/30-fps qualification.
  A separate reachability-only probe is rejected after only 0.09% mean
  improvement and 0.30% worst-orders improvement. The bounds-mask probe saves 0.58% mean in the P7 combat window but
  worsens the peak by 4.72%; keep it disabled. An algebraic steering probe
  also remains rejected after its ARM code grew. See the
  [current custom/performance audit](https://github.com/ggu/starcraft-gba/blob/1f4f9ca9/docs/audits/2026-11-07-contact-bounds-probe/README.md).
- **Nydus/BC building-capacity contract corrected:** the selected compact
  codec already carries seven building-type bits. The stale six-bit Nydus
  assertion is corrected for that selection; the legacy derived codec retains
  its six-bit guard. Full BC plus Z10 now compiles and fresh-loads type 64
  through the actual HOST journal, with worker cargo and Nydus retained.
  Six current normal links match all previous ROM bytes and guards, and clean
  default/unified checks pass. This does not enable any staged feature or
  change a wire format. See the [contract audit](audits/2026-11-07-nydus-wide-contract/README.md).
- **Campaign ownership before enabling upgrades/ammunition:** [draft #355](https://github.com/ggu/starcraft-gba/pull/355)
  now has separate current-controller identity for the explicitly scoped P1
  and Z9/Z10 HOST paths. Actual P1 rescue/production and late-Zerg town wallet,
  wave, transfer and slot-reuse fixtures pass. Flag-off P10 is byte-identical
  to the normal `317cc67d…` package. Other campaign contexts are fenced, and
  every save/load entry refuses the unpersisted stage. Next: authenticated
  persistence, remaining transfer consumers and supported campaign coverage.
- **Paid Scarab path:** [draft #356](https://github.com/ggu/starcraft-gba/pull/356)
  composes that identity with the paid child service. A scoped authored HOST
  Z10 fixture uses the actual descriptor's town-0 Reaver, explicit empty
  transfers to towns 7 and 2, the correct 15-mineral debits, refused unsettled
  transfers, and child identity. A paid child travels through an actual-map
  clear corridor, deals 100 damage through the real impact path and retires;
  custom/P1 comparators pass. Follow-up [draft #358](https://github.com/ggu/starcraft-gba/pull/358)
  preserves the existing 24 scripted/human unit slots plus committed products
  at payment and allocation. HOST pressure tests cover refusal, paid wait,
  cancellation/resumption and actual Drone/Crystal creation. This is not an ordinary campaign win, arbitrary
  routing, native AI purchase cadence, persistence or ARM qualification. Keep
  normal features off until those gates and shared codec composition pass.
- **Performance target now measured:** an exact normal P10 `317cc67d…` replay
  on the retained P7 combat tape averages 2,306,991 elapsed cycles across 958
  advancing updates. Orders average 1,146,261 cycles. Existing diagnostic
  hooks localize large costs to cloak/detection bookkeeping and movement;
  diagnostic overhead is 2.45%, and its scopes overlap. Prioritize those
  executed paths. The odd-camera tile probe is unreachable for normal P10's
  word-aligned terrain and is rejected. A finder-square hoist saves only
  0.433% in this window. Combined with a source-binding hoist, the candidate
  saves 1.804%, with unchanged P10 RAM guards, all 1,000 observed frame/mode
  transitions and 11 sampled full frames/palettes/state matching. Shared
  promotion is merged in [#359](https://github.com/ggu/starcraft-gba/pull/359),
  with six current guarded links, clean default checks and a prepared unified
  build. It is in the Mac package `StarCraft-GBA-2026-11-07-34c101e0`;
  KODAK is not mounted and has not received this package. None of this closes
  the near-30-fps or hardware gate.
- **N64 evidence:** the controlled Probe run now correlates visible individual
  selection, a unique Nexus-to-Probe reference and rightward movement fields
  in one record. This is scoped evidence for that capture, not a portable
  full ABI. A later register observer preserves all three RAM views and both
  frames byte-for-byte, but GP and raw CPU clock are zero at the bookends.
  They cannot establish an independent game clock. A separately qualified
  post-store observer now records 606 CPU stores, including verified writer
  instructions for this record's movement order, target and position. All
  three RAM views and both frames match the earlier controls byte-for-byte.
  Applying the stores reconstructs the intermediate record exactly, but five
  final bytes remain unexplained; the trace is not a complete history. STOP,
  a second controlled actor, fresh reset and clock/cadence evidence remain open.
  A later bounded run records 157 function-entry events, including the selected
  Probe's order arguments, and confirms actual order/position caller paths.
  Its RAM, frames and all 606 stores match the earlier controls. Nearby globals
  are now traced as spatial scratch, not a game clock; see the
  [caller/scratch audit](https://github.com/ggu/starcraft-gba/blob/5d1943fd4a608023fea0df0da2747df42496a48e/docs/audits/2026-11-07-n64-dispatch-candidates/README.md).
  Subsequent stack and bounded-counter observers preserve all prior RAM,
  frames, stores and entries exactly. Eight selected-actor stack records support
  the caller chain. The counter observer at `52a79f9f` captures 36 stores from
  verified PC `80023954`, decrementing `800a5acc` from 66 to 31 and reproducing
  both later counter views. This establishes a CPU-written periodic countdown,
  not an independent game clock. The first Start attempt reaches GAME PAUSED
  but a second Start opens Options. A corrected ordinary menu intervention at
  `454c23eb` visibly resumes gameplay: the two paused views retain counter 30
  without watched stores, and 20 more validated decrement stores follow resume.
  All prior 37 stores, initial RAM, play and pause frames match exactly. The
  final frame shows the selected Probe in gameplay. Root independently checked
  the record stream and both UI outcomes. See the
  [scoped pause/resume audit](https://github.com/ggu/starcraft-gba/blob/0d40d44e/docs/audits/2026-11-07-n64-counter-pause-plan/README.md).
  This establishes pause-associated suppression and resumed writes for one
  countdown, not an independent clock or speed scaling. STOP, independent actors
  and fresh reset remain open.
  See [draft #354](https://github.com/ggu/starcraft-gba/pull/354) and the
  [scoped store audit](https://github.com/ggu/starcraft-gba/blob/70108527831f1737fc1bfb7cd9299f03c3bc67f0/docs/audits/2026-11-07-n64-store-observer-design/README.md).
- **Normal package and HUD:** #352 merged normal Z9/Z10 HUD page reuse at
  `f1fbf106`; its retained Mac package is
  `StarCraft-GBA-2026-11-07-f1fbf106`. Z9/Z10 EWRAM margin rises by 12,464 B
  from 780 to 13,244 B; IWRAM remains 2,072 B. Six ordinary OFF/ON physical
  pairs cover two custom maps and late campaign scenes, with matching named
  state, full frame and palette through genuine 32 KiB saves and fresh loads.
  Four other raw OFF/ON controls are identical. Default checks passed (1,023
  Python tests, 26 skips, plus compiled checks), as did 12 links. KODAK was
  absent and no SD copy occurred. See the [HUD audit](audits/2026-11-07-zerg-late-hud-page/README.md).
- **Usable upgrades and saving (U3):** Forge and Cybernetics Core research now
  works in the normal Protoss P10 ROM's custom games (human Protoss), on the
  cartridge's existing SRAM with no change to the save record size. Paid
  weapon, armor, shield and Singularity Charge jobs run from the ordinary
  menus; completed levels change damage dealt and taken on the ordinary,
  splash, projectile and impact-queue paths; jobs and levels survive the
  alternating-slot save and a fresh load. Campaign missions, computer
  opponents, the Terran/Zerg/Showcase ROMs and hardware are not covered. See
  the [SRAM research audit](audits/2026-11-11-protoss-research-sram/README.md).
  The paragraph below describes the separate combined research/Q8/Flash
  experiment, which remains disabled in normal products. Its ordinary P10 custom tape
  pays for Forge research, saves, fresh-loads the same wallet/job and completes
  level one. The real Flash backend uses no guest-state writes or polling
  workaround; the feature-off P10 control matches the `317cc67d…` ROM. This is
  an emulated witness, not cartridge or all-mode qualification. HOST coverage
  includes 234 custom roster records and 234 genuine predecessor records;
  all 234 early ordinary-input ARM roster openings pass across both maps,
  three human races and all ordered one-to-three AI races. Saved research
  levels and Q8 debt were zero; this is not active-research or combat proof.
  Six experimental ARM probes pass at combined source, with Z9/Z10 EWRAM
  margin 12,588 B (normal products: 13,244 B). The P10 ROM matches the prior
  physical matrix byte-for-byte; all 28,999 defined symbol entries match.
  Clean default checks at `340ecd44` pass (1,034 Python tests, 26 skips, plus
  compiled checks). See the [combined audit](audits/2026-11-07-q8-custom-combined/README.md)
  and [physical matrix](audits/2026-11-07-q8-custom-physical-matrix/README.md).
  A separate [paid Forge replay](audits/2026-11-07-q8-paid-research-physical/README.md)
  on that exact P10 binary now verifies the natural 100/100 debit, a genuine
  Flash save, identical job/wallet/timer/context immediately after fresh L,
  and level-one completion. Its damage debt stayed zero. This adds an active
  job witness, not nonzero combat fractions or campaign qualification. Later
  controller-only additions have their own focused checks; see [draft #349](https://github.com/ggu/starcraft-gba/pull/349).
  Campaigns, later controller/race combinations,
  repair/Z5 aliases, real failure recovery, runtime stack and hardware remain
  release gates. The separate Scarab stage is not release-qualified.
- **Experimental memory and presentation:** at combined research source
  `8be82458`, the older six ARM probes linked with Z9/Z10 EWRAM margin 132 B;
  this is distinct from the current normal #352 product margins above. In an
  earlier optional combined-feature Z9 comparison, OFF/ON state and full frame
  matched through fresh load, but fresh-versus-uninterrupted pixels differed
  after 60 neutral updates: the tested loader cleared selected Forge UI context.
  That historical strict cross-path presentation check remains unresolved; it
  does not invalidate the merged normal HUD OFF/ON qualification. See the
  [experimental HUD audit](audits/2026-11-07-zerg-hud-memory/README.md).
- **Migration:** the composed phase checker passes 39 process steps covering all
  1,280 unit and 320 legacy building phases, 47 advancing updates, genuine old
  exports and alias/burn preservation. Checksum-valid malformed human/bot
  records are refused through the real loader. These checks do not alone qualify
  every campaign, roster, hardware failure or save transition.
- **[Hybrid N64 control](audits/2026-11-07-n64-hybrid-control/README.md):** an
  official same-process JIT state was restored into interpreter mode with equal
  cache-aware RAM views. Two pure-JIT control snapshots are visually close but
  differ by one in each differing byte. This feasibility result establishes no
  actor identity, clock, fresh-reset interpreter timing or hardware parity.
- **Egg rollout (S1):** the selector probe now has one actual ordinary-input
  Z7-ROM custom guest save/load/birth witness at runtime `6bbc678f`. Natural Pool
  construction and a 50-mineral Zergling purchase produce an Egg; its genuine
  SRAM loads in a fresh guest and yields the same two Zerglings at the same
  persisted-world checkpoint as uninterrupted play. Read-only inspection of the
  authenticated guest journal confirms Zerg human versus one Terran AI. This
  supplements the 19-step six-profile OFF/ON resource checkpoint; it does not
  qualify early campaigns, full pixels, all rosters or hardware. Early stages
  remain HOST-only and normal release flags/#298 remain unchanged.
- **[Native visual evidence](audits/2026-11-07-n64-visual-only-jit/README.md):** the bounded GPU interpreter probe reaches startup.
  Separately, the official CPU/RSP-JIT visual-only frontend at `60ade910` now
  completes the retained 8,617-step ordinary custom schedule, all three requested
  checkpoints and GPU checks in one successful run. Independent checks verified its manifest,
  13 source pins and three raw frames; all three decoded images were inspected. They
  show a completed Pylon, supply 4/17, and the compact 3×3 build grid selecting
  Pylon 100, Gateway 150 and Nexus 400. First/third raw frames exactly match
  historical captures; the middle raw hash differs. This reestablishes that
  visual context, not power-boundary, remembered-fog, damage or timing parity.
  This frontend rejects RAM capture; N1 actor/clock/cadence gates remain open.

The latest prepared Mac package is `StarCraft-GBA-2026-11-07-a5064cb2`:
T10, Z9, P10, Showcase and optional Z10 preview. All five ROM headers, SRAM
signatures, lengths and read-back checksums pass; P10 is exactly the measured
`0ec0ebeb…` candidate. T10/Showcase retain their preceding normal bytes because
those vectors do not use the nearest-depot service. KODAK is not mounted, so no
SD copy occurred. Research/ammunition/Flash experimental stages remain excluded. The full campaign 29/30 and near-30-fps gates
below remain open. Keep source-only public artifacts and local Actions policy.

## Current real-device feedback and integration

The November 6 report is confirmed as **Protoss10 custom gameplay**. The exact
build used is unconfirmed; e983 is the last recorded KODAK package. Source
merges do not update an already copied ROM.

- **Merged shared fixes:** #304 aligns B-menu icon-row highlights and prices;
  #305 enables ordinary producer L cancellation/full unit refunds; #306 gives
  all active unified-custom players 50 minerals/zero gas; #308 preserves
  paused-menu resource refusal cues; #309 continues established mineral jobs
  to eligible nearby known fields; #311 uses the source-shaped Pylon field
  for placement, production and cached/uncached Cannon power. Separate
  Egg/Carrier/Scarab lifecycles are unchanged; the visual field overlay is off.
- **Earlier validation and package:** the prior normal Mac package was
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
  **The last recorded KODAK package is e9838086**; its present contents have
  not been verified. Earlier Mac packages remain.
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
  open. [Standalone integration #344](https://github.com/ggu/starcraft-gba/pull/344)
  connects the transport and journal in a [test-only proof](audits/2026-11-07-flash-integrated/README.md):
  HOST models and unforced mGBA exercised full 28,656-byte slots and preserved
  the legacy 32 KiB; clean default checks passed. Game backend enablement,
  cartridge hardware behavior and real SRAM migration remain unqualified.
  No save format or release behavior changes with this experiment.
  **November 7 continuation:** [draft #336](https://github.com/ggu/starcraft-gba/pull/336)
  has a combined descendant at `8127f7af`, merged with main `fb2e414f`.
  A separate HOST campaign journal uses `0x47`: authored P1 Forge/Core jobs,
  genuine exact-847d predecessors, malformed-newer fallback and inactive
  Terran/Zerg saves have fresh-process fixtures. The Q8 combat configuration
  is separate and refuses that active-job journal: passing both configurations
  does not establish a saved fractional-combat game. Its actual-engine fixtures
  cover shield armor/spill, fractional HP, Siege/Archon/Interceptor/Scarab hits,
  Storm, construction, burn, repairs and colony morph HP clamps. A test-only
  misaligned pointer cast initially produced recoverable UBSan diagnostics;
  it is corrected, and strict reruns stop/scan for all sanitizer diagnostics.
  The strict combined run passed all focused cases and clean boundary/doctor/
  build/full default checks (447.810 seconds) at `8127f7af`; normal P10 again
  matched the current `317cc67d...` release byte for byte. All 593 prepared
  inputs, isolated Scarab facts and tested heads stayed unchanged. Audit-only
  successor `ca767732` records 17 successful commands, prior failures and
  limits. Full optional unified/Terran graphs still lack private preparation;
  current feature-on ARM and simultaneous saved fractional combat are not claimed.
  Normal research flags remain off. Fractional-state persistence, legacy
  phase conversion, current-controller/enemy policy, feature-on ARM/RAM/stack,
  ordinary saved gameplay and hardware performance remain release gates.
  A fixed 400-byte tail cannot cover all worst-case larger records. Corrected
  profile accounting shows Terran's actual 288-byte tail fits its 296-byte
  measured slack by eight bytes; this is not a working codec. P10 typed rows
  have promising bounded encodings, while the straightforward ZP joint-rank
  scheme remains short of bits. A HOST-only bridge from the existing Flash
  transport to the actual game codec is being implemented separately; it is
  not enabled or qualified. See the current draft's [combined audit](https://github.com/ggu/starcraft-gba/blob/codex/bounded-research-20261107/docs/SC64-RESEARCH-Q8-COMBINED-AUDIT-20261107.md) and
  [wire feasibility branch](https://github.com/ggu/starcraft-gba/tree/codex/q8-inline-wire-audit-20261107).
  Category navigation and the power-field overlay still need ordinary-input,
  framebuffer and dense-scene checks.
- **Other feedback:** death sounds, large death effects, remembered
  resource/fog presentation and an ordinary-input native elimination
  witness remain open.
  AI difficulty choices would extend the two native lobbies checked above.
  Current custom defeat counts the last live owned
  building. Delivered death pixels/timelines were authenticated; their large
  source frames require a paired native presentation check, not guessed scale.
  The [source-only geometry audit](audits/2026-11-07-custom-death-geometry/README.md)
  found correct unit-specific routing in the inspected examples. Probe's
  selected body rectangle is 16×15 versus peak cropped death frame 51×53;
  these are frame rectangles, not alpha-exact bounds or native scale proof.
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
- **Read-only native discovery:** the separate
  [observer branch](https://github.com/ggu/starcraft-gba/tree/codex/n64-actor-readonly-observer-20261107)
  records one bounded Terran custom attempt at runtime `d03052c2`: two 8 MiB
  cache-aware samples and three frames within a 64 MiB output cap. Root rechecked
  21 input/output hashes and viewed the initial/final frames. Natural mining
  (66 to 98 minerals) confounds the attempted Move, and sampled CPU clock is
  zero. Five weak candidates, four spaced 312 bytes apart, are not authenticated
  actors or cadence. No movement/STOP, independent clock, writer-PC or timing
  optimization follows. Reusable-wrapper admission and cleanup review is separate
  from the recorded run; no new gameplay behavior is enabled.
- **Early Egg combined HOST qualification:** [draft #346](https://github.com/ggu/starcraft-gba/pull/346)
  consolidates Z1–Z4/Z6 staging and the Z5 N/A source census with main
  `fb2e414f`. At `59179efc`, all 41 focused steps passed with strict
  sanitizer scans, fresh-process pairs and five genuine old-save readers;
  root independently checked all logs, the 594 private inputs, old saves and
  receipt hash. Z4's persisted CreateUnit retry remains an explicit GBA
  capacity policy. Synthetic denial fixtures are explicitly synthetic;
  normal build flags are unchanged. This does not activate #298 or qualify
  full default/unified/ARM, ordinary campaign wins, arbitrary old saves,
  Z8/Z10 campaign gates or hardware performance.
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
  ordinary-input win. Normal flags, ARM, remaining trigger reservations,
  ownership transfers, the full profile matrix and common activation remain open.
  A separate [Z3 HOST stage](audits/2026-11-07-z3-egg-host/README.md)
  passed actual-bank admission, denied-product validation, fresh paired birth,
  genuine old-tag fallback and a cross-owner slot boundary. Z4 CreateUnit
  reservation has a [bounded HOST retry experiment](audits/2026-11-07-z4-egg-host/README.md)
  with source-tag and fresh-process tests; standalone profile, ARM, ordinary
  play and N64 behavior remain open. A [Z6 HOST stage](audits/2026-11-07-z6-egg-host/README.md)
  adds authenticated PUNI, persisted future-create reservations and fresh
  Egg/old-save checks; broader scheduler and standalone-profile gates remain
  open. The [Z5 source census](audits/2026-11-07-z5-egg-applicability/README.md)
  found no ordinary producer/Drone seed in its initial roster or authored
  create/transfer actions, and a genuine old-tag save passed a 180-update fresh
  HOST continuation. This is not all-save, ARM or N64 qualification.
  The constructed late captured-Hatchery concern was not promoted: no reachable
  transfer path was found, and relaxing only admission would leave saves invalid.
  The [integrated HOST matrix](audits/2026-11-07-early-egg-integration/README.md)
  passed 41 strict-sanitizer, fresh/old-save and source-boundary steps at
  `59179efc`; normal product activation and ARM/ordinary-play gates remain open.
  The Z2 successor `fd9f56ea` used runtime `2ac2eabe`; root independently
  rechecked all 592 dependencies, the permission hash and both save hashes.
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
- Latest verified normal Mac package: `StarCraft-GBA-2026-11-07-34c101e0`:
  Terran 1–10, Zerg 1–9, Protoss 1–10, plus Showcase. The last recorded KODAK
  package was `20261106-e9838086-campaigns`; its present contents are unverified.
  Content coverage remains **29/30**;
  this is not a claim of complete native mechanics or campaign parity.
  P10 and Showcase include the shared aura optimization; T10/Z9/Z10 retain
  their qualified `f1fbf106` ROM bytes. All five 32 MiB headers, SRAM markers
  and package checksums pass read-back. Research/Flash, paid Scarab and Egg
  experiments remain excluded. The package README has Protoss-custom device
  checks for power, construction audio, menus, cancellation and mining.
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

The [stock 200 supply limit](audits/2026-11-12-stock-supply-cap.md) now
applies to shared production and the HUD, including AI owners and legacy
profiles. Existing units and reservations survive provider loss. The
[independent race ledgers](audits/2026-11-12-race-supply-ledgers.md)
also match the separate native T8 cap/provider/usage families. The
original P10 opening now trains both Terran and Protoss without
spending the other race's cap. Separately measure whether the GBA actor
pool can represent armies at the stock cap.
**Ghost and Nuclear Strike (P0):** the [N64 source and blast audit](audits/2026-11-12-ghost-nuclear-strike.md)
records native T8 attachment and supply-first refusal. The shared opt-in
probe trains Ghosts and paid missiles, launches from ordinary GBA controls,
and uses the owned Ghost, Silo, missile, dot, warning and impact assets.
It has authenticated Silo and flight journals for queued, armed, painting
and descending missiles and remains off in default builds. P10 generates
separate actor tables for the world, silo and active-strike probes;
local checks reject tables generated for a different mode.
Finish the native timing comparison, Ghost abilities and AI and complete
the persistence and ARM-input checks before enabling it.

Other core rules to prioritize after this:

- **Production:** replace the normal custom Hatchery queue with stock
  Larva and simultaneous Eggs for humans and AI. The isolated Egg checks
  do not establish shared save or production behavior.
- **Tech and combat:** finish cross-cartridge and AI research, then
  extend omitted spells and ammo, including Defiler, Scarabs and
  Carriers on the remaining cartridges. Keep the exact damage fractions
  and actual windup separate from projectile drawing.
- **Custom games:** allow each opponent slot to be closed, Random or a
  named race independently, as observed on N64. Difficulty and native AI
  behavior need their own checks. Confirm native win and defeat triggers
  before changing the current last-building rule.

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
| C2 | P0 / in progress | Complete shared Reaver/Scarab behavior. Normal campaign Reavers still use a generic 100-damage attack; draft #356 has scoped HOST paid movement/impact with current town ownership. Custom production and release composition remain staged. | Source/native ammunition and attack-event checks; paid training, capacity, launch, flight, terrain/target loss, splash, death/slot reuse, save/load and AI purchases across applicable products. No generic free shot as a substitute. |
| S1 | P0 / in progress | Finish shared Egg/Guardian production. The [opt-in late service](audits/2026-11-06-current-native-qualification/late-eggs.md) now has four actual ARM campaign Egg/SRAM continuations; normal activation across products and Guardian gameplay remain unfinished. | All real profile codecs and journals, interleaved tag contexts, old raw-36 Overlords, bad-newer fallback, atomic rejection, paired births, trigger reservations and Carrier/Egg capacity boundaries. Then ARM/RAM/stack, ordinary controls and genuine SRAM continuation before common activation. |
| S3 | P0 / open | Qualify saves throughout each combined campaign, including early Protoss missions under the P10 product. [One early physical P1–P7 save point each](audits/2026-11-06-early-protoss-save/README.md) now reloads with matching selected-state continuation on the pre-#323 `ff94a2ea` P10 ROM; P1 also has a matching mid-mission controller-tail continuation, but its historical tape no longer wins on the packaged product; P2/P7 remain especially early and no winning continuation is established. | Fresh-process ordinary save/load and continued wins across all campaign entries on current combined flags, with source-owned effects, actors, clocks and objectives preserved. Reproduce and fix each rejection; do not infer arbitrary-save support from one point. |
| C3 | P1 / partial | All 30 mission controllers now win on current source with ordinary input (HOST). Z10 has its first win (`tests/sc64_z10_campaign_run.c`: a Sunken Colony fortress with Hydralisks through the first waves, then a Lair, Spire and 34 Mutalisks to the Temple, escorting the crystal Drone; frame 58,795, with a fresh-process save continuation to the same win). The [Z10 ARM replay](audits/2026-11-12-z10-arm-qualification.md) now reproduces that win and 59 matching checkpoints after a fresh SRAM load. Z10 also passes all ten HOST campaign starts/saves and the 234 custom startup/save lineups. [Carried Z7](audits/2026-11-12-z7-normal-physical.md) wins from HOME on the normal Z10 cartridge at frame 194,998 with 253 matching observations after the route-pure controller starts its paid air plan earlier. Its full replay averages 9.35 emulated updates per second. The November 13 Z10 KODAK cartridge also wins after a fresh SRAM load at frame 185,926 and one ordinary A press to reselect the Hatchery; all five remaining observations match. [Carried Z8](audits/2026-11-12-z8-normal-physical.md) wins from HOME on the same normal Z10 cartridge at frame 45,468, with 19 matching observations, both Nexuses destroyed and Kerrigan at 400 HP. [Carried Z9](audits/2026-11-13-z9-normal-physical.md) wins from HOME on that cartridge at frame 72,628, with 32 matching observations and a live Drone carrying the crystal back to the hive after the countdown. Several wins are timing-sensitive (Z6, Z8, Z10, P8). [Carried Z3](audits/2026-11-12-normal-campaign-replay-gaps.md) wins twice on HOST with identical route-pure tapes and wins from HOME on the normal Z10 ROM at frame 136,588 with 465 matching physical replay observations. [Carried Z4](audits/2026-11-12-z4-normal-physical.md) also has a route-pure HOST tape and a normal Z10 win at frame 34,984 with 42 matching observations on the nearest-first runtime. [Carried Z5](audits/2026-11-12-z5-normal-physical.md) wins at frame 48,347 on the same normal Z10 runtime, with 55 matching observations and separate physical acknowledgments for authored text and the source pause. [Carried Z6](audits/2026-11-12-z6-normal-physical.md) wins on the normal Z10 ROM at frame 56,913 with 65 matching observations, including Kerrigan boarding and leaving the Overlord. [T1 and T2](audits/2026-11-13-t1-t2-normal-physical.md) win from HOME on the exact KODAK Terran cartridge at frames 10,120 and 29,366 with all retained scalar checkpoints matching and authored pages acknowledged. [T3 and T4](audits/2026-11-13-t3-t4-normal-physical.md) also win from HOME on the exact KODAK Terran cartridge at frames 54,000 and 8,911, with all recorded checkpoints and authored acknowledgments verified. [T5](audits/2026-11-13-t5-normal-physical.md) now wins from HOME on the normal Terran T10 cartridge at frame 122,281 with 150 matching observations, both heroes alive and no hostile buildings. P3–P6 still have mutating route observers. | Keep the distinct Z9 save path; qualify a neutral carried Z7 save cut or persist the selection safely; Temple/crystal text and results screens; apply the route-purity helper to the remaining controllers and add controller-free replays; keep all 30 winning as the engine changes. |
| N1 | P1 / in progress | Validate N64 actor records, game clock and event cadence before behavior-changing timing optimizations. The [matched T1 Marine STOP/no-STOP replay](https://github.com/ggu/starcraft-gba/blob/5445fb3d/docs/audits/2026-11-07-n64-stop-control/README.md#2026-11-12-matched-replay-without-stop) has identical RAM and frames at both pre-press boundaries. C-Down clears the move order and target by step 7,230; with no press the same Marine is farther along and still has the destination pending. The [matched native Start trial](https://github.com/ggu/starcraft-gba/blob/d44739a695afe28ef1ac9070c4d140029cd26bbf/docs/audits/2026-11-07-n64-stop-control/README.md#2026-11-12-matched-start-pause-control) opens Game Paused while retaining the pending move and target. The [matched menu Resume trial](https://github.com/ggu/starcraft-gba/blob/f2898c66202c4c66a6ffd75086c856a92468877f/docs/audits/2026-11-07-n64-stop-control/README.md#2026-11-12-matched-options-and-resume-controls) continues the same move; a second Start opens Options on the N64, while GBA field controls use Start as a Resume shortcut. A RAM counter freezes and advances with Resume, but its purpose is unproven. | Trace actual writes, compare idle and other actors, and establish an independent game clock before using it for timing. Keep snapshots bounded and read-only, with no MMIO side effects. The paired samples do not establish the precise stop tick or a selected-unit pointer ABI. |
| P1 | P1 / in progress | Reduce representative busy-frame cost toward 30 fps. The [2026-11-10 instruction-level profile](audits/2026-11-10-busy-combat-profile/README.md) found route floods at a third of custom-battle cost. Reusing evicted route fields from the spare bank, keeping fields across structure changes, an interrupt page flip, a cheaper placement cursor and cached tracer values cut three pinned 140-unit custom windows by 33–49% in mean emulator cycles, with HOST traces identical to the base in all 54 custom lobbies, and the P7 campaign window by 4%. A second round (see the audit's [worst-update section](audits/2026-11-10-busy-combat-profile/README.md#round-two-the-worst-updates-2026-11-12)) cut the slowest Zerg update by 15% and the Protoss one by 5% (cheaper depot slot scan and goal-cell test, incremental walkability update) with identical traces; the mean barely moves. Those windows are still two to three times the budget. | The [dense point-flood probe](audits/2026-11-12-dense-point-flood.md) ([#389](https://github.com/ggu/starcraft-gba/pull/389)) saves 1.76–3.32% mean and 13.36–22.27% peak loop cycles across the three paired ARM custom windows, with matching sampled workloads. All nine ROM verify targets pass, the 64×64 Z1 campaign wins in matching ARM replays, and Z8 produces identical SRAM and a matching fresh-process win. The 54 bounded HOST lobby trace pairs match. It is on for eligible maps in the canonical builds. Next: address depot floods after structure changes, Arbiter bookkeeping, target-index work, and terrain and unit drawing. Exact-source pairs on the pinned windows with trace identity; confirm on GBA hardware. |
| C4 | P1 / open | Replace campaign AI scalar approximations where verified source/native behavior differs. Late-Zerg waves use averaged groups/gaps; arrival, rebuilding and transport tactics are not equivalent to script commands. | Source script interpretation plus N64 observations of actual production, wallets, launch groups, targeting and rebuilds. Reproduce a concrete mismatch before changing AI; retain mission objective semantics. |
| U1 | P1 / partial | Scourge/Spire source restrictions and shared picker correction are merged in #290. Continue the broader campaign PUNI/UPGR/TECx audit, especially P10. | Preserve current controlling ownership, nonspending denial and full supported custom catalog. Apply remaining source permissions with per-owner tests and save compatibility; do not infer complete availability parity from this one product fix. |
| U3 | P0 / partial | Shared Forge/Core research. Done: the Protoss P10 ROM's custom games (SRAM, integer combat; [audit](audits/2026-11-11-protoss-research-sram/README.md)). Open: campaign missions (needs the generated source-policy header), computer-opponent research, Terran/Zerg/Showcase ROMs, Storm and other paths that bypass shield armor, hardware. Q8 fractions and Flash stay staged and unused. | Authenticated owner and payer identity; source initial/max levels; atomic paid producer jobs, power/cancel/death/capture/reset behavior, active-job saves and legacy migration; weapon/armor/shield/range effects on every relevant path; ordinary building menus and cross-product campaign/custom/Showcase checks. |
| U2 | P1 / partial | Carrier/Interceptor behavior. **Done on the Protoss ROM** ([details](CARRIER-PROJECTILES.md#current-status-2026-11-12)): Carriers train and fly in custom games and in P7 to P10; the P10 Gantrithor is a Carrier-class parent of the same shared service (full hangar on rescue, launch, return, rebuild on the ordinary command, parent and target death, owner change, actor cap, in-process and fresh-process saves, a pre-change P10 save loads with an empty hangar); Protoss computer opponents build a Fleet Beacon and Carriers and pay for their Interceptors through that service (stateless, cap of four, one per four Zealots/Dragoons; four `match-carrier` matrix cases). **Open:** the Terran, Zerg Z9/Z10 and Showcase ROMs compile none of it, so a Protoss player there has no Carriers; the [report](CARRIER-PROJECTILES.md#carriers-in-the-other-roms) finds Showcase needs no save-format change, Terran and Zerg need a variable-length projectile packet and a shot cap. Also open: the Gantrithor's starting hangar is an assumption (no source data), capture by Mind Control is not in P10 so owner change is rescue only, no ARM replay of a Gantrithor fight, no hardware timing. | Showcase first (flags, fact tables, appended IDs; the P10 save layout), then Terran/Zerg custom-game generation with a bounded packet, each with fresh-process saves and the full profile matrix. Settle the #212/#210 codec designs first; do not merge those drafts. |
| S2 | P0 / in progress | Maintain one shared capability and persistence contract across products. The stale Nydus guard now follows the selected codec's existing seven-bit capacity; no wire or cap change. P1 HOST research/controller saving is composed in #361. Ammunition, remaining controller contexts, repair, detection and morphs still need shared persistence. | Per-product and per-mode inventory from actual flags/call sites, lossless versioned representations, source-owned permissions, lifecycle/death/capture checks and every applicable save codec. No Showcase-only engine fix or mission-objective predicate used as a service gate. |
| K1 | P1 / partial | Qualify complete supported custom games for all race combinations and 1–3 AIs. Validate building/unit progression, economy, targeting, victory and long-game stability. The [2026-11-10 planner audit](audits/2026-11-10-custom-ai-reliability/README.md) fixed the opponent planner's stalls and economy failures on HOST: of 288 planner-driven matches (a mirrored-human fixture, both maps, every race mix, 1–3 AIs) 281 now reach the engine's victory or defeat instead of 271, all 144 on Blood Bath, and the AI matrix adds full-match, prerequisite, spending-audit and save/resume cases. Open: an ordinary-input human who wins late games, the ARM matrix beyond three sampled lobbies per ROM, hardware, and K2/K3 below. Zerg opponents remain much weaker than Terran and Protoss (serial larva queues). | Both maps, each human race, homogeneous and mixed AI races, actual tech progression and save/load. Track global pool/queue pressure and ensure human production is not silently starved (measured: with three AIs at their shares the human keeps at least an equal share of unit and building slots). |
| K2 | P1 / fixed in #386 | [#386](https://github.com/ggu/starcraft-gba/pull/386) lets short-range ground attacks contact the blocked footprint edge and falls back to a perimeter field when the regular route cannot plan a step. Runtime `ecaa865` passes 640 Z7 sanitizer cases. The mirrored HOST Outpost fixture (Z7, Terran vs three Zerg AIs) goes from unresolved at 216,001 updates on base to Terran defeat at 34,091. The corrected ordinary-input Z8 controller wins on Z9 in HOST and mGBA at frame 45,468, with matching fresh-process save continuation. All nine ROM variants verify. The pinned Z9 ARM battle's mean loop cost rises 0.17%, still far above budget. See [the replay and performance audit](audits/2026-11-12-melee-perimeter.md). | Broaden exact N64 contact-distance evidence and continue K1/K3. The older Gateway placement fixture fails on control and candidate before depot logic; update its setup separately. |
| K3 | P2 / open | Custom-game end states and worker rally. A game ends only when the human or every AI has no structure, so a world in which no owner can ever act again stays open (six of 288 audit matches, all four-owner Outpost games with every field mined out). Separately, a new worker and a resumed cargo carrier go to the nearest mineral field anywhere, including another owner's base while the owner's own second base has ore (about 1% of worker spawns). | Decide the native end-of-match behavior from N64 observation before adding any rule. Owner-aware rally in the shared spawn path with campaign controllers rerun. |
| M1 | P0 / in progress | Keep expanded mechanics within GBA memory and global actor capacity. Z10 has reached the 240-actor pool, preventing its former 70-Hydralisk launch goal. | Unchanged safety reserves, ROM/EWRAM/IWRAM and stack checks, child/queue/trigger capacity accounting and sustained busy play. Document adaptation limits; do not present catalog capacity as native-scale simulation. |
| B1 | P2 / open | Finish combat-rule and attack-event fidelity: fractional HP/shields, armor/size/upgrade effects, Firebat collateral, appear-at-target wind-up, shield-hit effects and specialized weapons. | Verify current code first; compare source/N64 events, not only appearance. Exercise allies, invincibility, shields, deaths and impact saturation with persistence and clipping. See [combat audit](ENGINE-FIDELITY-AUDIT.md). |
| B2 | P1 / open | Unify shared projectile flight and source attack events. Current follow/bounce traces use fixed endpoints while damage stays bound to a live target; stationary/melee paths can apply damage before depicted wind-up. | Native moving-target/release observations; bounded shared flight/event state, consistent visual/damage impact, separate shots, verified bounce rules, death/slot reuse/overflow, genuine save continuation and crowded ARM cost across products. See [visual/combat audit](audits/2026-11-06-n64-priorities/visual-combat.md). |
| V1 | P2 / open | Audit remaining source sprites, animations and placement/order effects, including each supported unit/building, damaged buildings, corpses, resource cargo and construction poses. | Paired N64/GBA observations, correct source frames/anchors/palette/timing, no missing assets, scene/HUD clipping and crowded effect/VRAM reuse checks. Existing fixes must be checked before reopening old reports. |
| V2 | P2 / in review | Blocky fog/terrain boundaries. Branch `codex/soft-fog-finish-20261112` (supersedes draft #146) draws explored-but-not-visible terrain through a palette-derived shade map and softens fog edges with world-anchored dithered masks; unknown cells stay opaque black and sight rules are unchanged. On in the Terran, Zerg, Protoss and Showcase ROMs with all linker guards passing (IWRAM end unchanged; EWRAM end +4.1 KB Zerg, +9.3 to +9.6 KB others (mostly the shaded-tile cache); a 256-byte shade map in OBJ VRAM between the projectile arena and the terrain cache). Evidence class: authored approximation. One N64 frame (unexplored terrain a flat dark purple of luminance about 11/255, smooth wide gradient from lit terrain, no hard tile edges) supports soft boundaries; colors and widths are not N64 values, and whether the N64 distinguishes explored-but-not-visible from unexplored was not established. Cost on the pinned busiest 1,000 custom-battle updates: loop mean +3.2% Terran, +3.3% Zerg (16-slot tile cache), +3.1% Protoss (the draft was +31% to +35%; shade-only without edge masks +2.0% to +2.5%); simulation cycles unchanged. | Open: cost is above the 1% aim (strip composing in scrolling frames, per-frame signatures); native preview and real-LCD check of the dither; a paired N64 observation of explored versus unexplored; campaign, SRAM and hardware qualification. See [the soft-fog audit](audits/2026-10-29-soft-fog/README.md). |
| Q1 | P2 / open | Resolve controls/advisor PR regressions and polish N64-informed UX: non-destructive deselection, queue cancellation/refunds, alert suppression, camera bookmarks, settings and results. | Ordinary-input campaign/controller matrix and updated replays, visible hints matching input, native alert/control observations where relevant, persistence and no paid-order surprises. |
| R1 | P1 / ongoing | Publish only qualified source, merge ready PRs, and package versioned ROMs with accurate descriptions. | `./dev release check/package/copy/verify` ([tools/sc64_release.py](../tools/sc64_release.py); see [Development](DEVELOPMENT.md#release-check-package-and-sd-copy)) now records one revision-stamped build/verify/HOST result per product (default, T10, Z9, P10, Showcase, optional Z10), versioned file names, `MANIFEST.json`, `SHA256SUMS`, a README with the source revision and the [hardware checklist](HARDWARE-CHECKLIST.md), and a no-overwrite, no-delete SD copy with read-back hashes. Still manual: prepared ARM/replay qualification beyond those steps, filling in the checklist on a device, safe ejection, and describing what is on a card only after a verified copy. No unnecessary Actions. |

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
   gameplay checks. The [Z10 spatial defense probe](audits/2026-11-12-defense-spatial-probe.md)
   saves 2.85–5.98% full-loop cycles with matching campaign checkpoints. The
   [combined Z10 probe](audits/2026-11-12-defense-combined.md) saves 4.74–7.50%;
   seven opt-in ROMs link and the Z10 stack canary leaves 8,948 bytes
   untouched above its guard. Z10 wins at frame 58,795 from ordinary SRAM;
   24 physical custom starts cover both maps and all three AI counts on the
   three race products and Showcase. Z10 also matches two complete three-AI
   defeats, one per map. P10 host campaign/save and 234 lineup checks pass;
   the normal P10 cartridge also wins from SRAM with 23 matching observations.
   The full Terran host campaign suite passes with both probes.
   The separate defense/sight/walk combination saves 8.18–13.05% in Z10
   with matching pixels and state; finish the remaining cartridge paths.
   The [packed sight-row probe](audits/2026-11-12-packed-sight-rows.md)
   saves 2.63–2.85% with the current split cache, with matched state and
   pixels. Packed sight is now the Z7–Z10 default, and the current normal Z9
   ROM wins the carried Z8 mission from legacy SRAM. After the per-race
   supply changes, both normal Z10 ROMs win from fresh
   physical SRAM with 61 matching raw and named checkpoints and 24 matching
   rendered and visibility samples. Mean loop cost falls 2.80–3.14% in the
   two latest campaign windows; the earlier 64×64 custom window is 0.93%
   faster with matching sampled state. Continue profiling the large stalls.
   Historical inclusive scopes are not promised savings. The N64
   order gate is a research lead, not permission to
   skip cooldowns, movement, pre-handlers or entire unit updates.
4. Retry Z10 through ordinary controls after the relevant mechanics are usable;
   qualify genuine save continuation and affected carried missions. Preserve the
   existing campaign coverage rather than tune difficulty around a substitute
   weapon or injected victory. Independently qualify early Protoss campaign
   save continuation under the combined P10 product.
5. Use a separate bounded reference task for moving-target projectiles and
   stationary/melee release events, then implement a coherent shared lifecycle
   (B2). Construction finish effects, shield feedback and Egg animation follow
   the gameplay and persistence blockers above.

The custom Egg service is an explicit preview across all four delivered
products, with legacy Z7 separately checked. Its static frame and selected-Egg
HUD work; pulse/birth/shadow animation and a producer job list remain V1 work.
The 30 available campaign entries do not change the near-30-fps and parity requirements.

Native dialogue timing remains a concrete UX mismatch: critical outcome
deferral is merged, but removal of synthetic holds remains staged after P4/P6
controller failures. The [native Blood Bath opening](audits/2026-11-12-n64-blood-bath-opening.md)
now reaches play with each named human race against a mixed Zerg, Terran
and Protoss lineup. The native and normal GBA Z10 HUDs agree on 50 starting
minerals and race-specific supply (Terran 4/10; Zerg and Protoss 4/9).
Minerals rise with neutral input in all six runs. Native computer rows can
independently choose Random or Closed; the N64 also starts a match with
only the middle computer row occupied. In both native Blood Bath modes,
Right on the final Random CPU skips Closed and moves to Zerg. Keep one
to three CPUs and match the edit behavior in the ordinary-profile
preview. The GBA now has independent seats as well. The native Melee selector uses a launch-time seed and can move
the human and all opponents even with the same lobby. Equal-speed timing,
AI wallets and full-match behavior remain unverified.

## Current work and open PRs

| Work | State and next gate |
| --- | --- |
| [#349](https://github.com/ggu/starcraft-gba/pull/349), combined research Flash experiment | Draft head `0b913a66`: all 234 early ordinary-input custom ARM roster cases pass, plus HOST current/predecessor rosters and a paid Forge save/load/completion on runtime `d7e7554e`. Those early roster saves have zero research/debt. Later six-profile composition retains the exact P10 probe binary. Clean default checks at `340ecd44` pass (1,034 Python, 26 skips, plus compiled checks); focused later controller additions pass separately. Campaign/controller coverage, aliases, failure recovery, stack and hardware remain open; normal flags off. |
| Shared HUD memory | Normal late-Zerg page reuse is merged in #352 and packaged at `f1fbf106`, adding 12,464 B of EWRAM margin on current Z9/Z10. The separate research probe's named-state save continuation passes, but its cross-load selected Forge/HUD presentation differs; that experimental presentation gate remains failed. Do not confuse the failed research composition with the qualified normal page reuse. |
| [#355](https://github.com/ggu/starcraft-gba/pull/355), current campaign controller | HOST-only P1/Z9/Z10 stage with distinct current controller and retained origin. Actual rescue/production and town wallet/wave/transfer fixtures pass; flag-off normal P10 matches. Save/load is refused; persistence and remaining campaigns/consumers remain open. |
| [#361](https://github.com/ggu/starcraft-gba/pull/361), P1 controller Flash | Draft head `449560c0`, qualified HOST runtime `30e5e782`. Actual P1 rescued origin/current identity, paid research and fractional HP survive fresh-process Flash load; genuine 43/44/47 migration, malformed/foreign record fallback and writer ownership/scratch tests pass. Stale-epoch end-to-end, Z10 Scarabs, ARM, hardware and other campaigns remain open. |
| [#356](https://github.com/ggu/starcraft-gba/pull/356), paid Z10 Scarab | HOST-only controller/wallet composition exercises actual-map child travel and a real 100-damage impact. Custom/P1 comparators pass; follow-up [#358](https://github.com/ggu/starcraft-gba/pull/358) passes scripted reserve-pressure, paid wait/cancel/resume and Drone/Crystal creation checks. Arbitrary routing, codec composition, saves, ARM gameplay and ordinary Z10 victory remain open. The earlier [integration plan](audits/2026-11-07-z10-scarab-stage/README.md) remains historical context. |
| [#359](https://github.com/ggu/starcraft-gba/pull/359), shared Arbiter aura optimization | Merged as `34c101e0`. Combined finder-bound and source-binding hoists save 1.804% mean elapsed cycles in one qualified P10 combat window, preserving the observed timeline/pixels/state and RAM guards. Six current normal links, clean default checks and prepared unified build pass; optional full unified/Terran checks lack a prepared private header. Packaged on Mac; no global 30-fps or hardware claim. |
| [#354](https://github.com/ggu/starcraft-gba/pull/354), bounded N64 observer | Draft. Controlled Probe selection/movement and unique-record evidence is followed by a qualified post-store run with 606 verified events and unchanged RAM/frame controls. Movement order/target/position writer instructions are identified for this capture. Intermediate record reconstruction matches, five final bytes do not. Full writer coverage, independent clock/cadence, STOP and another actor remain open. |
| [#297](https://github.com/ggu/starcraft-gba/pull/297), late-campaign Egg integration | Runtime `f32356c6`, test-only `7166def0`: opt-in Z8–Z10 producer/journal/owner/trigger reservation integration, eight sanitizer targets and four matching-product predecessor cases passed. Actual Z10-product Z8/Z9/Z10 and Z9-product Z9 pause-save/export/fresh-load/natural-birth cases passed. All six normal ROM hashes/RAM ends remain identical to `6ba`; clean default checks pass (1,000 Python tests, 26 skipped, plus compiled/auxiliary suites). Both public preview recipes reproduce the actual guest-qualified ROM hashes at publication `4f0d7080`. See [qualification and limits](audits/2026-11-06-current-native-qualification/late-eggs.md). Common normal activation, Guardian, full campaigns, stack and performance remain open. |
| [#295](https://github.com/ggu/starcraft-gba/pull/295), shared production preview | Merged as `34c1b38f`; whole tree equals reviewed publication `b57351af`. The [Egg-only custom candidate](audits/2026-11-05-shared-egg-custom/README.md) at `6ba76f8e` no longer depends on Guardian. Six normal ROM/RAM checks and clean default tests pass; all five opt-in GBA builds and HOST controller/journal/owner suites pass. The exact Terran ROM has physical-button ARM, genuine fresh-process SRAM and a separate static-frame/HUD visual check. Known pre-cargo Z6/Z7 reader regressions were reproduced and fixed without changing the authenticated inner token. Separate preview commands are available; default service remains off. Campaign integration, remaining guest products, long matches and performance are not thereby qualified. |
| [#290](https://github.com/ggu/starcraft-gba/pull/290), Scourge picker | Merged as `73cc74a5`. Runtime/test head `b3d6f8ef` passed viable producer/worker/site sanitizer controls, selected-product journal checks, six new ROM/RAM builds, full clean-Git default tests (989 Python, 26 skipped, plus compiled suites) and fresh-process loading of an actual prior-code pre-cargo HOST journal. Documentation successor/merge preserve identical runtime/build/test objects. No arbitrary guest-save or hardware claim. |
| [#293](https://github.com/ggu/starcraft-gba/pull/293), normal save-context hardening | Merged as `74922d57`; whole Git tree equals qualified `7123176e`. Normal Z7/Z9 custom and Z9 mission controls reject unsupported Eggs, preserve raw-36 Overlords and older-valid fallback, and retain a genuine prior Z7 paid-Egg journal. All six ROM/RAM checks and clean default checks pass (989 Python tests, 26 skipped, plus compiled suites); pins unchanged. A disk guard stopped the first six-build run before Z10; lossless archival permitted the complete retry. |
| Z10 controller | Wins since [#372](https://github.com/ggu/starcraft-gba/pull/372) (HOST, ordinary input, frame 58,795): Sunken fortress, Lair and Spire, Mutalisks, and a Drone carrying the crystal by the east route. An ARM replay of the win is still open. |
| [#262](https://github.com/ggu/starcraft-gba/pull/262), queue cancellation | Held: Z1/Z3/Z4 and T5/T6 ordinary-controller regressions, plus current-control UX/refund review. |
| [#265](https://github.com/ggu/starcraft-gba/pull/265), advisor alerts | Finished and merged as [#374](https://github.com/ggu/starcraft-gba/pull/374). Alerts play only with an audio bank regenerated by `tools/sc64_audio_pack.py`; older banks stay silent. |
| [#212](https://github.com/ggu/starcraft-gba/pull/212), [#210](https://github.com/ggu/starcraft-gba/pull/210), Carrier/Defiler | Both closed unmerged. Carriers now work on the Protoss ROM through the P7 path ([#380](https://github.com/ggu/starcraft-gba/pull/380), [plan for the other ROMs](CARRIER-PROJECTILES.md)). The Defiler and Dark Swarm pieces remain on branch `codex/shared-defiler-carrier-draft-20261031` and need a fresh port to the current base, default off. |
| [#146](https://github.com/ggu/starcraft-gba/pull/146), fog | Finished as [#381](https://github.com/ggu/starcraft-gba/pull/381) (merged with the current base, memory/VRAM ownership re-derived, ARM cost cut from about +33% to about +3.2%, pixel and simulation-identity checks). Open: the 1% cost aim, native preview, hardware check. |
| [#315](https://github.com/ggu/starcraft-gba/pull/315), late-Zerg packed grid | Merged as `738223a4`; scoped tests and emulator timing are recorded above. The later #323 package retains this capability, with a different binary; its speed has not been remeasured. Strict current Z7 winning validation, broader campaign/skirmish and hardware qualification remain open. |
| [#318](https://github.com/ggu/starcraft-gba/pull/318), research journal groundwork | Merged as `f34004af`, default-off. Full default suite and current flag-off composition passed; feature-on current-base and functional research qualification remain open. The default-off groundwork alone did not change package bytes; #323 is included in the newer normal package. |
| SD delivery | The November 13 KODAK folder holds verified T10, Z10, P10 and Showcase builds from runtime `cbba1c5`. Their card files match the local SHA-256 values; older ROMs and saves remain available under their original filenames. |

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
