# StarCraft GBA goal and prioritized TODOs

Updated: 2026-11-20. This is the living backlog for the race-ROM/N64-parity goal.
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

- **Current cartridge campaign saves.** [P6 on normal P10](audits/2026-11-20-current-p6-save.md)
  reloads from frame 64,800 and wins with Zeratul and Tassadar alive. All
  113 checkpoints match, including 40 after the reload. The same cartridge
  loads the earlier frame-121 journal and matches its 120-update continuation.
  [P6 trigger and burrow changes](audits/2026-11-20-p6-runtime-cost.md)
  reduce full-route cycles by 16.66% with the same SRAM and victory
  checkpoints. The route still averages 15.75 calls per emulated second.
  Continue applying the normal-ROM gate to the remaining
  [campaign entries](audits/2026-11-13-normal-campaign-matrix.md).

- **Result journal under combat load.** The [completed-journal check](IMPACT-EVENTS.md#save-and-load)
  reproduced stale SRAM after a normal P10 victory with nine pending
  impacts. The shared writer now saves the completed result with the
  final world. All four HOST vectors check victory and defeat with a full
  queue, including the rejection of oversized saves during active play
  on Z10 and P10. The [Blood Bath retirement preview](https://github.com/ggu/starcraft-gba/pull/566)
  also passes the pause and victory boundary on Terran and Protoss ARM
  from prepared fights in both skirmish modes. Keep the preview opt-in
  pending full ordinary match and native retirement checks.

- **P10 cycle budget (2026-11-20)**. The [released early collision rejection](audits/2026-11-19-p10-steering-budget.md)
  matches gameplay in P7 and three-AI Blood Bath. P7 saves 1.74% mean
  cycles. Keep testing point-route continuation under load. Both replay
  windows exceed the 30 fps budget.
  The [P10 detector range check](audits/2026-11-18-p7-current-mgba.md#composition-with-selection-ring-depth)
  saves another 0.63% mean on P7 after composing with selection-ring
  depth. The next large savings need to come from orders, route work,
  and draw time in busy games.
  [Current recurring-order tests](audits/2026-11-20-p7-recurring-orders.md)
  cut P7 mean cycles 1.20% by rejecting distant candidates earlier in the
  primary collision scan. The paired three-AI Blood Bath windows are
  effectively flat. The shared recipe passes all four ARM links, the
  T4 victory, a Showcase scene and the dense Z7 prefix. Showcase has
  72 bytes below the IWRAM link guard. Keep the optical cache and
  combined primary/fallback mask trials out of the normal builds.
  [The composed resumable point-route trial](https://github.com/ggu/starcraft-gba/pull/576)
  remains opt-in. A [compressed range read](audits/2026-11-16-resumable-frontier-budget.md#cached-range-read-on-a-compressed-wide-grid)
  cuts a forced 128-by-128 stall from 1,086,782 to 252,139 cycles. All 86
  real Z9 snapshots exceed its graph cap. Keep the shortcut in the
  experiment and prioritize exact and nearby-root completion on real
  terrain and repeat readers. The [target-index retention and
  fused rebuild comparisons](audits/2026-11-12-target-index-bulk.md#retention-and-fused-setup-on-dense-z7)
  match the sampled battle but raise p95. Keep those trials out of release.

- **Frontier Skirmish (P1):** an independently authored battle profile
  now starts Transit Yard and Stone Gardens with one to three computer
  opponents. The host checks cover all six setups through mining,
  production construction, pause/save, title reload, and restart.
  The three-opponent host match also reaches an AI army attack. Normal
  ARM inputs start each setup, save at update 900, and reproduce
  the sampled world and AI state on a fresh-process load. The first
  240 updates measure 19.76–20.76 emulated updates per second across
  the six setups. Both three-opponent openings average about 560,000
  draw cycles and 110,000 simulation cycles. The timing work and the
  current set of complete ARM win replays for both maps are recorded below.

- **Frontier factions (P1):** each of the three cartridges defaults to
  its own faction. Skirmish setup offers all player factions, both maps,
  and one to three independently configured CPUs. Foundry Riflers,
  Rovers Skimmers, and Bulwark Wardens have different reach, damage,
  cadence, movement, health, and silhouettes. The AI uses faction-specific
  worker and attack thresholds. The factions share support units and
  structures. All 54 cartridge, human faction, map, and CPU-count setups
  started using ordinary buttons under host sanitizers and in mGBA.
  On all three cartridges a save with a non-default human faction
  resumed in a fresh emulator process and restarted with the same setup.
  Each cartridge loaded an earlier fixed-player save and rejected another
  cartridge's new save even when the player faction matched its default.
  Foundry also loaded a save from the original single-roster cartridge.
  All 54 starts and those save checks passed again on `f5e60854`.
  An additional sanitizer run on `f804475f` exercised all 1,404 setups
  across the three cartridges, including every CPU faction permutation
  in Free for All and CPU Team. Ordinary inputs started, saved, loaded,
  and restarted each match with the same map, factions, and mode.
  The first 240 gameplay updates measured 29.50–29.57 updates per second
  in mGBA, up from 19.75–20.75 in the earlier qualification.
  In two earlier normal-input ARM matches, a passive Foundry player lost
  to Rovers on Transit Yard (game tick 5,584) and to Bulwark on Stone
  Gardens (5,524). Add distinct faction support units and production choices.
  The [pause comparison](audits/2026-11-18-skirmish-pause.md) records
  mining before and after a native N64 custom-game pause and an
  exact paused-clock and mineral check on all three Frontier host builds.

- **Frontier CPU Team (P1):** the title's Mode row lets players keep the
  default Free for All or have the computers fight on one team. The CPUs
  retain separate forces and banks. This follows the opponent relationships
  observed at launch in the
  [paired N64 Blood Bath runs](audits/2026-11-12-n64-blood-bath-opening.md#paired-blood-bath-launches).
  Paired ARM runs on both Frontier maps sampled orders every 60 game updates
  from ticks 1,980 to 3,960. Free-for-all CPUs sent attacks toward other
  computers. In CPU Team all three sent attacks toward the human base.
  The paired three-CPU Team openings averaged 29.09–29.25 updates per
  second through tick 3,960 in mGBA. All 54 CPU Team setups started, with
  the first 240 game updates averaging 29.56–29.57 updates per second.
  One save on each cartridge loaded in a fresh process and matched the
  next 120 updates before a restart kept the team. Three saves from
  before the mode was added loaded as Free for All.

- **Frontier victory replays (P1):** each of the three player factions
  has a normal-button win on both maps against one CPU. Both maps also
  have mixed two- and three-CPU wins. The stored inputs use Foundry's
  title defaults, and the replay tool translates setup for Rovers and
  Bulwark. Each case starts at the title, builds and trains a force, issues
  attack orders, and checks the result screen and exact game tick. In the
  three-CPU matches, Foundry beat independently controlled Rovers, Bulwark,
  and Foundry on Transit Yard at game tick 14,727 and on Stone Gardens at
  tick 13,912.
  These full mGBA replays averaged 29.05 and 29.40 updates per second
  after the first 100 updates. Physical GBA SP timing remains open.
  The two-CPU Team cases reached victory at tick 14,649 on Transit Yard
  and tick 12,980 on Stone Gardens at 28.68 and 28.85 updates per second.
  On Stone Gardens, Foundry beat three allied Bulwark CPUs at tick 15,510
  by protecting its miners with one early barracks and attacking with a
  larger force later. The same field input reached victory on Foundry,
  Rovers and Bulwark with identical opening state and at 28.53 updates
  per second in mGBA. A corresponding Transit Yard replay remains open.
  To replay the checked-in inputs, build Foundry
  and pass the physical observer from `tools/sc64_headless_replay.c` to
  the optional test.

  ```sh
  make frontier-foundry
  python3 tools/frontier_win_replay.py /absolute/path/to/sc64-headless-replay --output /tmp/frontier-victories
  ```

  Use a fresh output directory. Add `--case transit-yard-foundry`
  to run one match. The result file includes elapsed emulator cycles and
  average updates per second from the setup checkpoint to the result.
  To replay on another cartridge, build its ROM and select it explicitly.

  ```sh
  make frontier-rovers
  python3 tools/frontier_win_replay.py /absolute/path/to/sc64-headless-replay --cartridge rovers --case stone-gardens-three-cpu-team --output /tmp/frontier-rovers-team
  ```

  `--cartridge` accepts `foundry`, `rovers`, or `bulwark` and defaults to
  Foundry. On Rovers and Bulwark the tool adjusts the title inputs for that
  cartridge's starting faction while preserving field input timing.
  On `50283403`, all twelve cases reached the same victory tick on each
  of the three cartridges. The Rovers and Bulwark witness adjusted the
  title inputs for the cartridge's default faction and kept every field
  input at its original update relative to match start. The opening
  game state, actors, resources, and random seed matched Foundry.
  All 36 full matches averaged 28.68–29.48 updates per second in mGBA.

- **Frontier four-player targeting (P1):** when all four corners are
  occupied, break exact AI target-distance ties toward each attacker's
  clockwise neighbor. In both maps' mixed three-CPU opening, the
  northeastern CPU chose the human at tick 2,160 even though the
  southeastern CPU was equally near at 47 planner cells. The new rule
  chooses the southeastern CPU in that tie. The nearest hostile target
  still wins when distances differ. The eight one- and two-CPU replay
  victory ticks stayed fixed. Larger waves, including the rule that
  counts queued fighters toward the dispatch threshold, remain open.

- **Frontier CPU mining recovery (P1):** idle CPU workers look for
  reachable ore within 192 pixels of a completed command center they own.
  In twelve long defensive matches driven by identical normal inputs
  on both versions, the CPU exhausted its four local patches around
  game ticks 18,260–18,482. The old policy sent nine workers to other
  corners. The new policy assigned none to a distant patch. The
  sanitizer fixture covers both maps and one to three CPUs, including
  reload after depletion, an unfinished hall, and a reachable local
  patch. All twelve existing victory inputs kept their result ticks and
  averaged 28.68–29.48 updates per second in mGBA. The paired long mGBA
  runs also found nine remote assignments before the fix and zero after.
  On each map a fresh load after local depletion matched the live game
  for the next 120 updates. CPU expansion after local exhaustion remains
  open.

- **Frontier frame time (P1):** clear the byte visibility grid with
  aligned stores, remember explored circle centers until a world load or
  restart, and draw uninterrupted battlefield grass/fog as row spans.
  In two normal-button ARM replays, each 240-update opening and combat
  window starts after updates 100 and 3,950 respectively.

  | Match | Opening updates/s | Combat updates/s |
  | --- | ---: | ---: |
  | Transit Yard, 2 CPUs | 22.31 to 29.89 | 20.60 to 23.67 |
  | Stone Gardens, 3 CPUs | 21.50 to 29.69 | 20.43 to 22.48 |

  About 54,000 simulation cycles and 63,000 draw cycles were removed per
  opening update. Sixteen sampled world and framebuffer checkpoints
  matched across the baseline, each change alone, and the combined
  build. The original terrain oracle covered 512 map, camera and fog
  cases. The combined build finished both replays with the same victory
  ticks, produced identical mid-match SRAM, and matched 120 updates
  after loading each build's save in the other. The cache adds 2 KiB
  of working RAM. The measurements are from mGBA. Route and rendering
  cost in active combat still need profiling and physical GBA timing
  remains open.

- **Frontier busy combat (P1):** an instrumented ARM build attributed
  about 41,000–98,000 cycles to point searches and 304,000–315,000 to
  the HUD including its minimap in sampled combat and later movement.
  The 64-entry point cache preserves the forward search result and its
  topology invalidation. The Frontier minimap samples the same tile
  centers and draws a 24×24 raster before the existing markers.
  Together they add about 1.1 KiB of working RAM and save
  about 176,000–181,000 draw cycles per sampled combat update.

  | Match | Busy updates/s | Later movement updates/s |
  | --- | ---: | ---: |
  | Transit Yard, 2 CPUs | 23.67 to 28.18 | 25.48 to 29.77 |
  | Stone Gardens, 3 CPUs | 22.48 to 28.63 | 24.22 to 29.04 |

  These are 240-update mGBA windows beginning after updates 3,950 and
  5,200. After the first 100 updates, complete matches averaged 28.90
  on Transit Yard (previously 24.50) and 28.78 on Stone Gardens
  (previously 22.69). Both reached the original victory tick. Twelve
  gameplay and framebuffer checkpoints matched. Mid-match SRAM matched
  byte for byte, and both builds continued each other's saves for
  120 matching updates. A 3-VBlank update still appears at the 95th
  percentile in both combat windows. Repeat timing on GBA SP hardware
  and cover more high-load lineups.

  On `50283403`, eight 240-update windows across the mixed three-CPU
  Free for All and two-CPU Team wins averaged 28.50–29.80 updates per
  second in mGBA. The median update was 33.49 ms and five windows had
  a 50.23 ms 95th percentile. The worst sampled update was 83.71 ms at
  Stone Gardens game tick 5,341, with 957,144 cycles inside simulation.
  Two instrumented runs matched the release's world state around the
  spike. The orders pass used about 926,000 cycles, including about
  854,000 for movement. The point route and direct corridor probes used
  about 2,200 cycles combined.

  A follow-up attributed the movement spike to one walkability rebuild.
  Initialize the authored terrain with the map's rectangle spans and
  apply the existing building footprints. This cut the measured rebuild
  from 801,870 to 75,375 cycles with identical world checkpoints. The
  normal ROM's tick 5,341 dropped from 83.71 to 33.49 ms. Its 240-update
  window improved from 29.56 to 29.74 updates per second, and its worst
  update dropped to 50.23 ms. The full-grid oracle covers both maps,
  overlaps, unfinished buildings, removal, replacement, and clipped
  footprints at the map edges. Seven other sampled windows kept their
  reported average rates. Continue profiling the remaining combat stalls
  and measure on GBA SP hardware.

  In the Stone Gardens CPU Team match, the remaining repeat spike came from
  a point-route search on each new starting tile. The wall time of the
  largest update dropped from 62.25 to 33.49 ms after checking whether the
  shortest horizontal-then-vertical route is clear before starting the
  full search. Simulation dropped from 506,796 to 97,449 cycles at that
  update. The 240-update window rose from 28.96 to 29.86 updates per second
  in mGBA. Its 95th percentile fell from 50.23 to 33.49 ms and its worst
  update fell to 37.88 ms. All sampled worlds and walkability grids matched
  on both maps, and an independent breadth-first search checked 2,968
  authored and randomized query pairs plus directed edge cases. The four
  Transit Yard windows kept their rates or improved. At game tick 5,163,
  both builds saved identical SRAM through the pause menu. Each accepted
  the other's image in a fresh emulator process and matched the next 120
  updates. Physical GBA SP measurements remain open.

- **Original-art skirmish fog (P1):** clip sight rows before visiting tiles
  and use exact integer spans for explored tiles. In a paired ARM opening
  of 1,000 updates, mean simulation cost dropped from 101,614 to 65,583
  cycles. Mean complete update cost moved from 563,907 to 562,503 cycles
  because the main loop often waits for presentation. The comparison
  matched 14 state and video checkpoints, the SRAM image, and the
  continuation when each ROM loaded the other's save. A separate
  1,000-update first-wave sample matched 11 checkpoints and lowered mean
  simulation cost from 64,085 to 40,374 cycles. Complete update cost held at
  561,660 cycles.

- **Original-art skirmish routing (P1):** the point and depot searches now
  keep coordinates in the queue and record the first step during expansion.
  A save with 40 units and simultaneous detours around the terrain barrier
  exercised 640 ARM updates. Worst simulation cost dropped from 816,755
  to 397,055 cycles, and mean complete update cost fell from 855,881 to
  833,058 cycles (19.60 to 20.14 emulated updates per second). All 17
  state and video checkpoints and the final SRAM image matched. In a
  separate normal opening, updates over the 561,792-cycle budget fell from
  four to zero out of 1,000. In the 40-unit scene, drawing still averages
  505,994 cycles and 607 of 640 updates miss the budget. A read-only trace
  of 80 of those updates measures 335,110 cycles from draw setup to the HUD,
  171,023 cycles for the HUD, and 165,902 cycles from presentation entry to
  the next input sample. The first figure includes clearing, the map,
  field feedback, and audio observation. Compare how often the HUD panels
  actually change before trying to reuse them. A separate build with
  `GBA_PRESENT_DEADLINE` retains the 28 checkpoints and both saves and
  lowers the dense mean to 754,606 cycles (22.23 emulated updates per
  second). Dense p95 is essentially flat. During the opening, the flip
  phase shifts about 100–130 cycles per update before frame 430 waits an
  extra 200,000 cycles at presentation.

- **Original-art skirmish frame pacing (P1):** the Mode 4 Outpost builds
  count hardware VBlanks and require two video frames between flips. A
  page finished during an eligible blank can present in that blank.
  The ARM 40-unit sample averages 754,572 cycles (22.23 emulated updates
  per second) and the opening averages 561,350, with no over-budget
  update in that 1,000-update sample. Both saves and all 28 gameplay and
  video checkpoints match the polled baseline. The emulator trace saw
  2,000 consecutive opening flips on line 160 exactly two video frames
  apart. All 640 dense flips occur on lines 160–226, with intervals from
  two to four video frames. Dense combat remains above budget. In 80
  traced updates the minimap costs 89,410 cycles, the command panel
  59,082, and the resource strip 21,056 on average. The resource strip
  changes in 3 of 40 consecutive frame pairs, and the lower-left area
  changes in 2. The minimap area changes in every pair.

- **Original-art minimap drawing (P1):** batch adjacent terrain cells
  with the same shade while keeping the unit, building, and viewport
  overlays. Mean draw cost in the 40-unit replay falls from 506,161 to
  462,920 cycles and the complete update averages 700,949 cycles
  (23.94 emulated updates per second). The normal opening still runs at
  29.89 emulated updates per second. Both saves and all 28 checkpoints
  match the preceding build. Reuse the stable HUD panels next and check
  display timing on GBA hardware.

- **Carrier skirmish parity:** the normal Protoss ROM has the Carrier and
  Interceptor services. The standard Terran, Zerg and Showcase builds still
  lack working Carriers. The [Showcase probe](CARRIER-PROJECTILES.md#showcase-carrier-probe)
  fits the projectile, impact and Yamato queues. Ordinary ARM controls paid
  for a Carrier and an Interceptor, attacked a Hydralisk, resumed from a save
  with a live shot, then moved the Carrier away and reloaded the game with
  its Interceptor docked. The ordinary Showcase ROM rebuilt byte for byte.
  The [Terran probe](CARRIER-PROJECTILES.md#terran-custom-carrier-runtime-probe)
  now verifies the same paid production, active shot and docked save on ARM.
  The [Z10 probe](audits/2026-11-16-z10-custom-carrier-envelope.md) verifies
  those same gates using a custom journal that retains all 100 projectiles
  and 32 pending impacts. The [shared selector](CARRIER-PROJECTILES.md#shared-release-selector)
  builds byte-identical ARM previews for all three and leaves the Protoss
  build command unchanged. The [HOST matrix](CARRIER-PROJECTILES.md#campaign-and-lobby-host-matrix)
  now checks ordinary Terran/Z10 campaign boot, save and load on all ten
  entries per ROM, both skirmish maps, lobby lineups and Showcase scene
  save isolation. The [ARM gate](CARRIER-PROJECTILES.md#pinned-arm-release-gate)
  confirms an opening save and fresh-load continuation in all 20 Terran/Z10
  chapters, and twelve mixed and sparse skirmish saves with restarts across
  both maps. All three previews also converted one regular Transport1
  custom save with the old bank intact and restored the new compact bank.
  The [sampled ARM stack gate](CARRIER-PROJECTILES.md#sampled-arm-stack-use)
  measured title load, active Interceptor combat and pause/save. The
  smallest SYS margin was 5,880 bytes and all three had 120 bytes of
  conservative IRQ margin. The [combined follow-up](CARRIER-PROJECTILES.md#carrier-with-the-regular-route-lookup)
  links all three with the regular lookup, matches 192-update battles and
  complete saves against the earlier previews, and exercises malformed
  newer compact journals on ARM. In one [mixed Blood Bath ARM match](CARRIER-PROJECTILES.md#mixed-blood-bath-battle-from-blank-sram),
  the Protoss computer paid for a Carrier and an Interceptor. The Z10
  preview kept 101–104 live units in the sampled fight and restored a save
  with a live shot and four pending impacts. It averaged 1.01 million
  emulated cycles per update against a 561,792-cycle 30 fps budget.
  The 1,000-update continuation matches 43 watched frames and the exact
  final save payload after a fresh mid-battle load. Its [cycle split](CARRIER-PROJECTILES.md#where-the-mixed-battle-spends-its-cycles)
  finds 648k cycles in game update and 359k after update per loop on
  average. A [separate order breakdown](audits/2026-11-17-z10-order-tail.md)
  attributes 995k cycles of the worst zero-build update to work after
  actor orders. The [narrow projectile profile](audits/2026-11-17-z10-order-tail.md#follow-up-retirement-revalidates-the-whole-pool)
  traced that cost to full-arena validation during retirement. Live
  retirement now checks the links it changes; incoming and saved data
  still get full validation. A separate [route breakdown](audits/2026-11-17-z10-order-tail.md#why-a-zero-build-update-rebuilt-route-fields)
  found two actual floods the ordinary build counter did not count.
  Nearby route lookups now reuse a retained gradient when the current
  grid still proves its exact path. Next, split counted route builds into
  actual floods, retained field adoption and facility restarts, and profile
  the dense worker/target path. The mixed battle still misses 30 fps.
  The [weapons bounds change](audits/2026-11-17-render-first-update.md)
  removes the cold tracer scan: first update 3.00M → 1.80M cycles,
  with sampled pixels and the final SRAM matching. Sustained play is
  still about 1.01M cycles per update. Terran and Showcase now also
  reach paid Protoss AI Carrier combat from blank SRAM with three AIs.
  Both restore their compact battle bank byte for byte with active shots;
  the 192-update windows averaged 871k and 1.36M cycles respectively.
  Next: verify longer busy windows and human purchases in the combined
  builds, and qualify full pools, older journals and hardware before
  activation. Keep roster and saved IDs stable.

- **KODAK delivery (2026-11-13):** Terran T10, Zerg Z10, Protoss P10 and
  Showcase built from runtime `cbba1c5` were copied to
  `Games/Development/StarCraft-2026-11-13`. All four 32 MiB files were
  read back from the mounted card and matched their build SHA-256 values;
  the card folder has `COPY-VERIFIED.txt` and `START-HERE.txt`. The files
  include the shared command-menu speedup. The November 14 package replaces
  that build on the Mac with the shared wide point FIFO (merged as
  `8935d56`), verified across all four ROMs. The November 16 package at
  `cb210bc` adds row-local steering and fixes stale collision after a Drone
  starts a building. The regular `002a591` package adds the wide routing
  lookup. All four ROMs were checked against their SHA-256 values and
  are in Downloads with a quick test guide. The November 17 separate
  Carrier previews add the regular lookup and are also in Downloads.
  KODAK was not detected during staging, so the older
  card copy and all saves remain untouched. Next: test B-menu scrolling,
  save/load and battle speed on a physical GBA SP.

- **One ten-mission cartridge per race:** the default build and release
  profiles are Terran T10, Zerg Z10 and Protoss P10. All ten Zerg controllers
  win under the Z10 flags; its 234 custom startup/save cases and 18 complete
  ordinary-input skirmish runs are covered separately. Explicit `zerg-z9`
  keeps its original ROM identity and save path. These results establish
  campaign availability and selected play paths; frame budget, broader save
  coverage, and original N64 behavior remain separate gates below. The
  [campaign evidence matrix](audits/2026-11-13-normal-campaign-matrix.md)
  separates exact KODAK wins, earlier normal builds and remaining checks.
  [Z3](audits/2026-11-13-z3-current-home.md) and
  [Z4](audits/2026-11-13-z4-two-normal-roms.md) win on both November 13
  Z10 revisions. [Z5](audits/2026-11-13-z5-kodak-replay.md) and
  [Z6](audits/2026-11-13-z6-kodak-replay.md) also win on the KODAK Z10
  ROM with Kerrigan alive, including the Z6 transport route.
  [Z8](audits/2026-11-12-z8-normal-physical.md#exact-november-13-kodak-cartridge)
  and [Z9](audits/2026-11-13-z9-normal-physical.md#exact-november-13-kodak-cartridge)
  now win from HOME on that ROM, with both Nexuses destroyed in Z8 and
  the Drone delivering the crystal in Z9. [Z10 also wins from HOME](audits/2026-11-13-z10-exact-home.md)
  on that ROM, matching all 61 shared world checkpoints with the
  earlier save continuation. [Z7 now wins uninterrupted from HOME](audits/2026-11-12-z7-normal-physical.md#uninterrupted-home-on-the-exact-kodak-cartridge)
  with 253 matching observations, including five shared with the SRAM
  continuation. All ten Terran and all ten Zerg missions now have
  complete HOME wins on their exact KODAK cartridges. Continue Protoss
  replays, broader save checks, and spot checks on newer runtime builds. The normal
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
  Its replay averages 8.27 emulated updates/s.
  [Homeland](audits/2026-11-13-p7-carried-normal-home.md) wins on both
  November 13 P10 revisions and the nearest-first candidate. [P1 and P2
  now win from HOME on staged P10](audits/2026-11-13-p1-p2-staged-prefixes.md#fresh-controller-routes)
  with their rescued heroes alive. [P5](audits/2026-11-14-p5-p6-normal-home.md)
  now also wins on staged P10, with Tassadar rescued and the authored pages
  verified. The [November 20 P10 save replay](audits/2026-11-20-current-p5-p8-save.md#p5-choosing-sides)
  loads the pause journal at frame 2,700 and reaches the same extraction win.
  P6 also wins on staged P10 with Zeratul and Tassadar alive and
  all 113 checkpoints matched. [Both missions also win on the exact KODAK
  P10 ROM](audits/2026-11-14-p5-p6-normal-home.md#exact-kodak-p10-replays).
  [P4](audits/2026-11-13-p3-p4-home-preparation.md#p4-full-staged-home-win)
  wins on both staged P10 and the exact KODAK cartridge with Raynor and
  Tassadar alive and 146 matching checks. [P3 now wins on the packaged
  wide P10 ROM](audits/2026-11-13-p3-p4-home-preparation.md#wide-p10-packaged-rom-p3-victory)
  after opening a second mineral line. All 47 checks match, and a separate
  pre-expansion pause save reloads with matching state through 241 updates.
  All 30 missions now have a complete ordinary-input win on a normal ROM
  in mGBA. Check remaining older/current cross-build and save continuations.
  [Standalone P6](audits/2026-11-19-standalone-p6-impact-saves.md) passes ROM
  verification and saves up to 22 pending impacts. Its full host target
  still needs a controller route that avoids the loss at frame 53,613.
  The [old P8 route](audits/2026-11-13-p8-staged-route-loss.md)
  loses Fenix in the latest normal P10. The [updated controller](audits/2026-11-13-p8-host-hero-retreat.md)
  now [wins on the staged ROM](audits/2026-11-13-p8-host-hero-retreat.md#complete-home-win-on-staged-p10)
  with both heroes at full health, 41 matching observations and twelve
  authenticated in-game pages. The [November 20 P10 ROM wins after fresh SRAM load](audits/2026-11-20-current-p5-p8-save.md#p8-the-trial-of-tassadar)
  from the verified pause save at frame 30,825, matching all 41 checkpoints
  and retaining both heroes. Its journal and regenerated controller tape
  match the prior verified route byte for byte. Continue with other save
  states and missions on the current release.

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
  The [November 18 Terran terminal pair](audits/2026-11-12-skirmish-seat-options.md#current-terran-arm-terminal-and-save-checks)
  reaches identical defeats from HOME and fresh SRAM within each mode. Two
  human survivors keep their owner on GBA. Keep the flag off while active
  human wins, terminal checks on the other products and native elimination
  are open.
  The [native T1 fresh-load witness](audits/2026-11-13-n64-t1-save-selection.md)
  clears the selected Command Center, consistent with the normal GBA
  selection reset; native Z7 remains untested.
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
  and detaches their AI. The [native cadence audit](audits/2026-11-13-n64-blood-bath-result-cadence.md)
  finds that the stock dispatcher evaluates all eligible owners before
  publishing results. Watch its real countdown rather than counting video
  frames when testing a direct last-building kill. A [linked-actor audit](audits/2026-11-13-n64-blood-bath-linked-survivors.md)
  traces the loaded passenger exception and the Carrier/Reaver queue reset.
  The [last Command Center lift](audits/2026-11-13-n64-blood-bath-airborne-last-building.md)
  confirms an airborne building still counts. GBA skirmish needs a
  shared Terran lift/land service with compatible save validation and
  correct depot, add-on, collision and air-target behavior.
  A passive replay did not reach elimination. [Draft #445](https://github.com/ggu/starcraft-gba/pull/445)
  tests retirement and neutral cargo on HOST. Its final victory now waits
  for deferred retirement, including a surviving Battlecruiser fixture.
  [Draft #566](https://github.com/ggu/starcraft-gba/pull/566) qualifies the
  opt-in ARM build. The Terran Free for All replay transfers a surviving
  Overlord on CPU retirement at frame 13,905. The Melee replay transfers
  both human survivors on defeat at frame 11,706. Fresh saves match in
  both cases. The draft now requires a later result pass for victory
  after retirement. Four HOST profiles pass pause/save and fresh load
  between retirement and victory. Qualify the ARM boundary and native
  countdown. An opt-in Battlecruiser survivor transfer also passes mixed
  cargo and Yamato HOST save tests. Empty and docked Carrier groups
  now pass the P10 HOST transfer and save tests. Qualify compact saves,
  queued and launched fighters, native technology and shot behavior,
  the other coupled survivors, and simultaneous outcomes before enabling.
  The [native result observer](audits/2026-11-14-native-result-opening.md)
  now matches the archived opening under unchanged file limits and
  confirms the 31-decrement cadence and owner scan order. No live results
  were published in that opening. A separate ordinary-input replay
  verified placement, construction and completion of a Barracks, then
  queued, selected and moved the resulting Marine. The next bounded replay
  reaches and destroys a hostile Hatchery using a normal Marine attack.
  An authenticated continuation selects the surviving Extractor at
  750 HP, so the Hatchery was not the last building. A new fork trains
  three Marines, scouts the Zerg base, and attacks its Hatchery after
  matching all 24 archived frames and six read-only guest-state samples.
  Clean frames show 980, 719 and 468 HP. A neutral-input continuation
  matches those frames and catches 217 and 11 HP, then the destroyed
  Hatchery while enemy buildings and mobile units remain. Read-only native
  rows now confirm the selected Hatchery leaves owner 0's chain, the count
  falls from six to five, and no result byte changes. The next target is
  the still-full 750 HP Extractor. The [scanout diagnostic](audits/2026-11-14-native-result-opening.md#localize-the-rejected-n64-scanout)
  rejects an intermittent raw-frame error and locates it before palette
  conversion and capture. One [serial host command replay](audits/2026-11-14-native-result-opening.md#serial-host-rdp-command-calibration)
  matches all prior frames and guest samples, but its cursor misses the
  Marines. A [traced serial repeat](audits/2026-11-14-native-result-opening.md#traced-serial-replay-rejects-an-early-frame)
  confirms that the raw gate closes the transport on an earlier
  scanout mismatch. A [bounded mapped readback](audits/2026-11-14-native-result-opening.md#fence-mapped-readback-contains-the-mismatch)
  finds the differing RGB bytes already present at the readback fence.
  A [clean binding replay](audits/2026-11-14-native-result-opening.md#clean-readback-verifies-the-buffer-and-fence-binding)
  verifies the queued buffer and fence despite the same wrapper label
  offset. A [same-image strip comparison](audits/2026-11-14-native-result-opening.md#same-image-strip-copies-match-the-clean-readback)
  matches both copies and the archive in one clean run; retain the probe
  if the fault recurs. The later 21,308 handoff remains untested.
  Recalibrate with bounded cursor-only input after exact replay,
  then verify live Marine selection and the exact Extractor attack
  frame before B. Continue toward the last structure and published
  result under the same limits.
  Distinguish in-flight damage and simultaneous outcomes, then qualify
  complete ordinary-input matches on hardware.
  [Terran versus one Zerg CPU on Outpost](audits/2026-11-13-outpost-human-win.md)
  now wins from HOME on the KODAK ROM and staged Terran ROM with all 95
  observations matching. The policy puts idle builders back on minerals,
  trains Marines, Firebats and Medics, and defends before pushing. It
  averages 27.29 emulated updates/s but peaks at 14.04M cycles. Expand to
  Blood Bath and to two and three independent opponents. The [named lineup
  controller](audits/2026-11-13-named-skirmish-matches.md) now records
  complete HOST losses with one, two and three chosen CPUs, including a
  Terran mirror, two duplicate CPUs and a reversed mixed lineup. The
  three-CPU Blood Bath result matches all 25 checks on the staged Terran
  ROM at 20.93 emulated updates/s. Check the other two on ARM and develop
  a winning Blood Bath route. Native automatic
  [SCV resumption](audits/2026-11-13-n64-scv-construction-resume-plan.md)
  and the outcome of surviving actors still need native witnesses. A
  [direct Move attempt](audits/2026-11-17-native-scv-foundation-interruption.md)
  left the original SCV at the scaffold during construction and the same
  Barracks completed at the control cut. The authenticated active SCV had
  no displayed Stop before or after reselection. The [next native aim](audits/2026-11-17-native-west-scout-aim.md)
  kept the three Marines selected and put the cursor just above the minimap
  border. Verify an interior hit before clicking to scout the enemy. Worker
  loss and foundation resumption remain unobserved.

- **Protoss route stalls (P1):** the normal P10 cartridge now uses the
  same nearest-first depot selection as Z10. [Normal P10 qualification](audits/2026-11-13-p10-nearest-normal.md)
  matches Eye of the Storm from HOME and a fresh pause save, and 101
  observations in a custom match with up to four depots for one owner.
  Homeland also wins with 59 matching snapshots on the promoted ROM.
  Three gameplay stalls fall by 80–95%; full P10 averages 8.99 emulated
  updates/s, up from 8.80. The [typed save byte reader](audits/2026-11-13-typed-save-reader.md#default-builds-with-nearest-first-p10-routing)
  is now default on P10 and Showcase, the two products using typed rows.
  The P10 result update drops from 76.51M to 37.49M cycles with matching
  world, video and SRAM. Showcase writes identical two-bank journals and
  keeps corrupt-bank fallback. Next: reduce sustained movement and render
  costs; these save timings do not improve the ordinary update budget.

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
  The [partial terrain capture probe](audits/2026-11-13-terrain-dirty-capture.md)
  copies only validated dirty bands back into the split cache. Host
  raster and fallback checks pass; keep it off until matching ARM
  video and timing establish its benefit during actual play.
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
  A [paired worker/depot trace](audits/2026-11-17-z10-worker-grid.md) now
  ties a remaining spike to destroyed-building invalidation: seven of the
  twelve observed depot fills restarted a partial search for the same
  depot. Retaining one frontier matched the paired ARM checks but was
  slightly slower overall, so it stays out of the shared builds.
  Next, measure the owner rank scan with a depot-only list that preserves
  slot order and the same reachability calls. For rendering, test the
  existing partial terrain capture before expanding it. The [dense P10
  pair](audits/2026-11-13-terrain-dirty-capture.md) preserved exact gameplay,
  video and SRAM, but added 0.10% to the mean loop cost; leave it off.
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
  mean savings). On exact KODAK Z7, the 25,000-frame stretch after
  frame 75,289 averages 6.80 updates/s. The [dense stock stage sample](audits/2026-11-12-z7-normal-physical.md#uninterrupted-home-on-the-exact-kodak-cartridge)
  finds movement orders take 2.61M of 3.77M cycles per active update.
  Route-build calls average 3.14M orders cycles versus 1.03M without a
  build. [Stock breakpoints and a paired wide FIFO run](audits/2026-11-13-z7-wide-point-flood.md#dense-window-on-the-exact-kodak-build-vector)
  find four point fills in each of three slow calls. The FIFO cuts mean
  active loop cost 19.56% and orders 28.26% with matching checkpoints.
  The wide FIFO is enabled in all four normal products after matching
  raw endpoint state, four guarded links and four paired SRAM checks.
  Using pointers inside the bounded FIFO removes two instructions per
  enqueue and saves a further 3.03% on the dense Z7 replay without more
  working RAM. Four paired SRAM checks and T4/P10 prefixes match.
  Physical GBA timing remains open. [Dense route measurement](audits/2026-11-14-z7-point-flood-retention.md)
  finds no same-seed aliases among 448 stock point fills. One retained
  field matches 51 misses in a point-only model, but the ARM copy-based
  probe adds 6,152 EWRAM bytes and slows the wide FIFO by 0.55% with
  byte-identical sampled state. Leave copy-based retention off. On the
  [current pointer ROM](audits/2026-11-13-route-linear-word-pack.md#dense-z7-on-the-pointer-fifo),
  point fills take 736k cycles per active call, including 489k in FIFO
  expansion and 135k in packing. Aligned word output saves 0.52% of
  the loop with matching fields and no extra RAM; keep it opt-in.
  [Pointer-only one-field retention](audits/2026-11-14-z7-point-flood-retention.md#one-plane-pointer-retention-on-the-current-fifo)
  now saves 2.85% mean loop and 8.29% p95 cycles on the same dense tape
  while keeping route oracles and scratch bindings unchanged. Composed
  builds and four paired SRAM continuations pass. T4 and P10 prefixes
  match but exercise no retained hits. The [borrowed scratch tail](audits/2026-11-14-z7-point-flood-retention.md#borrow-the-route-queue-tail-november-16)
  brings the cost down to at most eight EWRAM bytes and saves 1.25% mean
  and 4.32% p95 cycles on dense Z7. Independent source review found no
  queue, range or save ownership blocker. On the current 128×128 Z9
  replay it avoids 462 of 3,162 wide floods (0.53% lower mean, 1.66%
  lower p95). Z8 and the facility FIFO composition also match gameplay
  checkpoints, including forced facility overflow under sanitizers.
  All four current Blood Bath save/load pairs and the full P10 physical
  win match. The [current composition](audits/2026-11-14-z7-point-flood-retention.md#coexistence-with-the-shipping-small-map-frontier)
  preserves the small-map facility frontiers and enables the scratch tail
  in the shared builds. Dense Z7 saves 1.19% mean and 8.82% p95, with
  effectively flat means in busy P7 and three-AI Blood Bath. Keep testing
  hardware timing and prioritize the count and cost of remaining wide fills.
  The
  [current instruction trace](audits/2026-11-15-z7-contact-cost.md) puts
  524k cycles in uncached collision masks in three no-flood samples,
  about half those samples' order cost. The next replay shows that
  44% of visited actors share the mover's exact coordinates. Rejecting
  those early cuts mean dense Z7 cycles by 3.99%, with matching state.
  The cheaper row-local memo keeps answers after unrelated movement
  and cuts a further 6.51% mean overall and 11.05% in orders on this
  window. It also fixes stale collision when a Drone starts a building.
  The packed axis probe was slower. The [resumable route budget](audits/2026-11-16-resumable-frontier-budget.md)
  puts full point fills at 25.95% of the old dense Z7 loop and 4.95%
  of sampled Z9. Observed origins need about 39% and 2% of FIFO work,
  with at most 52 pending entries at first discovery. The paired Z9 probe
  continues declined searches in the same ordered grid. Busy p95 improves
  11.3%, but by itself the worst call still costs 2.0% more. The same
  audit tracks a byte lookup
  that cuts the wide grid setup loop from 14 Thumb instructions to eight
  and its isolated mGBA cycles by 34.6%, with identical HOST route and
  fallback receipts. The paired dense Z7 replay matches 131 checkpoints
  and both raw route endpoints, cutting the active mean by 1.39%, p95 by
  2.79% and the worst update by 2.85%. Composing the two probes on Z9
  preserves all sampled world and route state. That busy interval's p95
  falls 13.35%, and the worst call now falls 1.86% against the release.
  Byte lookup alone preserves the same state and cuts the worst call
  by 3.99%. The cold prefix improves p95 but gives back 497,335 cycles
  at the peak. The standalone lookup now ships across the four regular
  builds. Both maps pass matching save/load in eight paired skirmishes,
  and the full Protoss 10 victory matches with effectively unchanged
  timing. Use `CANONICAL_ROUTE_LINEAR_GRID_LUT8=0` for comparisons.
  Keep cold-prefix continuation off and test the lookup on hardware.
  The [target index fast link](audits/2026-11-12-target-index-bulk.md#reuse-validated-cells-and-occupancy-bits)
  reuses validated cells and occupancy bits. The paired Z9 campaign
  prefix matches raw world and route state and uses 1.32% fewer cycles
  on average with bulk rebuild and fast link. Z10 keeps the same EWRAM
  and uses 160 fewer IWRAM bytes. The
  [canonical P7 qualification](audits/2026-11-12-target-index-bulk.md#canonical-release-qualification)
  covers all four cartridges, both maps, and fresh save compatibility.
  The first
  [mixed Z10 Carrier battle](CARRIER-PROJECTILES.md#where-the-mixed-battle-spends-its-cycles)
  matches the prior world and byte-identical 32 KiB save image;
  mean cycles fall 0.78%. This is emulator evidence only.
  Dense play remains well short of 30 fps. In the
  [Z10](audits/2026-11-12-z10-battle-cycles.md) and
  [busy P10](audits/2026-11-06-current-p10-busy/README.md) samples,
  simulation alone already exceeds the 561,792-cycle frame budget.
  Prioritize routing and simulation. The
  [Mode 0 experiment](VALIDATION.md#p7-mode-0-tile-renderer) traded draw
  savings for VBlank copies and did not improve whole-game frame rate.
  The [packed mineral probe](audits/2026-11-13-static-mineral-word-draw.md)
  cuts Z7's sampled static resource pass by 18.82% and matches ordered
  pixels on Z7 and both custom tapes. Keep it opt-in pending attribution
  of the Blood Bath menu timing tail and hardware checks.
  The [P10 profile](audits/2026-11-13-p10-command-census.md) finds depot
  facility floods account for 41.94M cycles of the largest gameplay update.
  On the draw-order release, a [blocked-worker trace](audits/2026-11-20-p10-blocked-depot.md)
  locates 4.89M cycles in one full depot search. The exact wide FIFO
  cuts the full update from 7.54M to 3.61M cycles with matching field
  bytes and world state. Both full HOME replays win with matching sampled
  state. Late combat peak drops from 7.56M to 5.42M cycles, and the whole
  replay mean is effectively flat.
  The next stall at P10 call 45,231 spends 3.43M cycles in six distinct
  point floods. Directly writing three empty fields cuts the resumed
  update from 5.42M to 4.79M cycles. The current canonical composition
  enables both shortcuts across all four builds. Full P10 wins match all
  sampled world and framebuffer states, and late peak falls from 7.55M
  to 4.77M cycles. Both T4 and T10 wins and the Z7 campaign prefix match
  their recorded gameplay checks. P10 still averages 10.49 emulated
  updates per second. Traversal of the other three fields takes 2.09M
  cycles in the measured update. Prioritize sustained simulation and draw
  cost, and verify physical GBA timing.
  The shared Command census cuts mean cost in current normal P8, P9, P10,
  and Z8 replays with matching outcomes. Fresh HOME reload matches P10
  through victory; both current products match paired Blood Bath saves.
  Enable it in the two current cartridges that carry the affected campaigns.
  Depot stalls and physical GBA timing remain open. The [paired P10 routing tests](audits/2026-11-13-p10-route-options.md)
  show wide point FIFO saves only 0.15% overall; nearest-first saves
  2.05% and cuts two individual depot stalls by over 93%, with the full
  win unchanged but different cache contents. Keep P10 defaults stable
  until broader checks. The [exact bounded facility FIFO](audits/2026-11-13-bounded-facility-fifo.md)
  cuts three P10 gameplay stalls by 43–57% and its full mean by 1.12%,
  matching the observed route bytes, outcomes and video. Four guarded links,
  the Z7 window and one 64×64 SRAM continuation pass. A current P10
  rerun with a smaller wide-only FIFO matches the win but raises mean
  cycles by 0.21%, with only 24 IWRAM bytes to the Showcase guard. Keep
  it off. The [typed save reader](audits/2026-11-13-typed-save-reader.md)
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
  The [linear route word-pack probe](audits/2026-11-13-route-linear-word-pack.md)
  matches scalar output and overflow behavior under sanitizers. It
  targets packed output writes in the expensive facility and wide point
  fills. Exact P10 ARM comparison cuts three sampled stalls 1.42–2.35%
  with unchanged working RAM, but mean cost only falls 0.027%. Keep it
  off and prioritize fewer route rebuilds over output-store tuning.
  The [cursor query probe](audits/2026-11-13-p10-field-cursor-cost.md)
  matches the same P10 prefix and cuts mean cycles 1.23%, with unchanged
  IWRAM and BSS. Keep it off until all products and a composed battle
  replay are checked; focus larger gains on simulation and route stalls.
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
- **P10 shared Egg and research SRAM (S1):** the [compound P10 probe](audits/2026-11-12-p10-research-egg-sram.md)
  preserves both a paid Zerg Egg and Forge research in a single skirmish
  save without increasing the record. The current port inherits the normal
  P10 typed reader. Host sanitizers and physical-button ARM replays pass for
  the mixed save and migration from both research-only and Egg-only journals.
  Keep the separate build opt-in pending broader campaign and older-format
  checks, stack headroom, busy-match measurements and physical hardware.
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

## Release and measurement baselines

- Canonical branch: `codex/source-release`. The source-permitted Scourge/Spire
  correction in #290 and save-context hardening in #293 are retained. #295
  adds the explicit shared Egg preview; its normal product flag remains off.
- The November 7 normal Mac package was `StarCraft-GBA-2026-11-07-34c101e0`:
  Terran 1–10, Zerg 1–9, Protoss 1–10, plus Showcase. The last recorded KODAK
  package in this November 7 check was `20261106-e9838086-campaigns`.
  This package carried 29 of 30 missions. The later November 13 cartridge
  audits verify all ten Terran and all ten Zerg missions on their exact
  builds. The T10, Z10, and P10 sources expose all 30 mission entries. The
  campaign replay and save checks below track verification by ROM revision.
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
- The [repeat-command save audit](audits/2026-11-18-saved-repeat-command.md)
  traces a normal Terran Outpost reload that changed 22 attack-move orders
  to move orders. Preserving only the remembered command keeps selection
  clear and wins from a fresh SRAM load on the unchanged tape. All four
  ARM products pass sampled fresh loads. A human Blood Bath win, wider
  campaign SRAM replay and hardware timing remain open.
- The [shared SC64 menu controls](RACE-CAMPAIGNS.md#menus-and-session-settings)
  add title Options, a custom map preview, and a field pause page for objectives,
  settings and returning to title. Cursor Speed and Menu Sound are session
  settings. The full native map and mode catalog, Game Speed and the second
  selectable save slot remain open.
- The [current-release P7 comparison](audits/2026-11-18-p7-current-mgba.md)
  at `0e536f26` measures 8.10 updates/s on the normal P10 build and 8.28
  with the terrain-cache preview in the same mGBA combat window. All 11
  sampled world and display checkpoints match. Unit orders still take
  about 985,000 cycles per advancing update. Matched order diagnostics
  attribute 392,902 cycles to movement and 269,588 to Arbiter cache work.
  Both builds also won the carried P7 mission from HOME at frame 55,892
  with 59 matching named checkpoints. Prioritize exact improvements in
  movement and Arbiter work in the campaign builds.
- [Post-menu contact and detection changes](audits/2026-11-18-p7-current-mgba.md#contact-and-detection-on-the-post-menu-build)
  reduce mean P7 cost from 2.073M to 2.011M cycles with matching sampled
  worlds and display. Normal Terran, Z10, P10 and Showcase links pass the
  existing memory guards. Full contact masks still merit attention, and
  the isolated 908,000-cycle point-field fill needs separate tail work.
  Keep physical GBA timing and peak-stack checks open.
  [Reusing packed candidate coordinates](audits/2026-11-18-p7-current-mgba.md#reuse-the-collision-candidate-load)
  reduces the same mean to 2.002M cycles. A speed-plane hoist had
  negligible mean benefit and raised the sampled peak.
- [Terrain validation #567](audits/2026-11-18-terrain-fast-validate.md)
  is enabled on the canonical cartridges. The carried P7 physical-input
  window averages 1.959M cycles with the terrain change. [Spatial defense
  queries](audits/2026-11-12-defense-combined.md#qualification-on-the-terrain-default)
  bring the same window to 1.921M, with matched world and display samples.
  All four canonical cartridges share the Pylon lookup. Zerg and Protoss
  campaign passes with target lists search nearby cells. Keep prioritizing
  unit orders and repeated mission world scans, and measure stack usage
  and worst updates alongside mean cycles.
- [Bulk target rebuild and fast insertion](audits/2026-11-12-target-index-bulk.md#canonical-release-qualification)
  lower the same P7 mean from 1.921M to 1.901M cycles and free 160 IWRAM
  bytes in each canonical cartridge. The composed Outpost replay wins
  after a fresh load without changing the controller tape. The full P7
  victory matches all 59 checkpoints. The normal P10 build also wins
  [P2 and P3 after fresh loads](audits/2026-11-18-saved-repeat-command.md#early-protoss-campaign-continuation)
  at the two previously divergent pause cuts.
  [Fusing P7 world checks with the mission census](audits/2026-11-18-p7-current-mgba.md#callback-based-census-experiment)
  increased mean cost 1.79% and stays outside the release.
  [HUD census composition](audits/2026-11-18-p7-current-mgba.md#compose-minimap-dots-during-the-hud-census)
  matches sampled pixels and saves 0.19–0.23% mean in P10 Homeland
  and three-AI Blood Bath. Dense Z7 is flat overall and spends about
  690 more cycles in the HUD. Keep the normal renderer and use the
  current background and orders profiles for larger savings.
- [Checked projectile acquisition](CARRIER-PROJECTILES.md#checked-live-projectile-acquisition)
  speeds up the shared Interceptor firing path. A staged fight on the
  normal P10 engine averages 1.158M versus 1.113M guest cycles with exact
  world, projectile, display and save samples. An ordinary three-CPU
  Blood Bath fight on the compact Z10 preview drops mean cost 4.72%
  and p95 cost 11.08%, with matching fresh loads. The older P7 tape
  leaves the arena empty and gives identical timings. The full ordinary
  fixed-format P10 victory also matches. Target the remaining route burst
  at Blood Bath frame 17,128, when 12 builds share a single update.
- The [interleaved depot flood audit](audits/2026-11-17-z10-order-tail.md#interleaved-depot-floods-after-a-building-dies)
  found three partial depot fields restarting after six cells opened.
  Keeping each pending queue within the orders pass cuts the worst compact
  Blood Bath update from 5.644M to 2.621M cycles with matching route counters,
  sampled fields, display and fresh SRAM continuation. Mean savings are
  0.41%. The regular Z10 match drops mean cost 3.65% and worst-update
  cost 32.51%; that vector also moves the orders loop into IWRAM. All
  30 campaign starts, 72 skirmish starts and eight cross-loads pass on
  the four regular cartridges. Keep profiling average cost and wider battles.
- The [post-#572 sustained P7 audit](audits/2026-11-18-p7-current-mgba.md#sustained-combat-after-572)
  measures 1.902M guest cycles per active update. The camera moves in 912
  of 957 adjacent samples. A vertical cache rotation saves 0.62% overall
  and 31,444 cycles per vertical partial restore. The two-axis rotation
  raises p95. Two collision memo alternatives produce zero additional
  hits on 36,650 requests.
  Prioritize the cost of the first collision lookup and repeated terrain
  work during both axes of camera movement.
- The [first-lookup breakdown](audits/2026-11-18-p7-current-mgba.md#work-inside-the-first-collision-lookup)
  visits about 20 actor nodes per mask. About 77% fail the distance check
  after reading actor facts, and only 2% fail the flight layer. Packed
  nodes raise mean cost 0.35%. Deferring admission of a fresh steering
  mask until the mover stays put saves 0.97% on P7 and 0.39% in regular
  Z10 Blood Bath. Dense Z7 saves 0.17% mean with a 0.10% p95 increase.
  Full P7 matches victory, dialogue and 59 checkpoints at 8.31 emulated
  updates per second. The 30 fps target remains open. The diagonal
  fog-strip probe saves 103 cycles overall and stays local. Prioritize
  recurring route and order work and larger terrain draw costs.
- The [detector range probe](audits/2026-11-18-p7-current-mgba.md#detector-range-rejection)
  skips exact distance calculation when one inclusive gap exceeds the
  range. It saves 0.58% in P7 with matching sampled state and pixels.
  Keep it in qualification for the other prepared profiles.
- A [native CPU purchase](audits/2026-11-12-n64-blood-bath-opening.md#terran-cpus-first-observed-purchase)
  corroborates the paid Terran worker opening in Blood Bath Melee.
  The shared opening cuts show 50 minerals and an empty queue for
  every owner. The Terran CPU spends 50 on a queued SCV before any
  owner adds a building. Capture first purchases for Zerg and Protoss
  before comparing the later AI build order.
- The [native Wasteland group retarget](audits/2026-11-07-native-actor-differential/README.md#november-18-wasteland-group-movement)
  follows five selected Marines in ares. Two ordinary Move commands assign
  destinations on both axes, while the GBA's initial five destinations lie
  on one row. [Four Blood Bath SCVs](audits/2026-11-07-native-actor-differential/README.md#blood-bath-workers-and-the-minimap)
  instead receive one common destination per command, with different
  intermediate waypoints. A matched follow-up gives the same SCVs common
  targets at both short and long field distances. Compare troops and
  workers from matched starts before changing shared formation.
- The [N64 game-speed comparison](audits/2026-11-18-native-game-speed.md)
  matched two fresh Challenger openings and changed only the Game Speed
  slider. The same 2,700 observer steps after resume earned 128 minerals
  at the starting setting and 184 at the higher setting. The full set of
  seven positions gives 7, 9, 12, 15, 18, 22 and 24 gameplay advances in
  61 resumed observer steps. Use this ordering for a GBA Game Speed option,
  with display pacing and long gameplay timing qualified independently.
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
The [Covert Ops draft](https://github.com/ggu/starcraft-gba/pull/411)
restored both painting and descent on ARM from prepared Terran saves,
matched the uninterrupted blast, and rejected a malformed newer record
without changing SRAM. A separate blank Blood Bath run paid for its
Barracks and Refinery, then kept three SCVs on minerals while mining gas.
The same match built and fresh-loaded a Factory, then a Starport and
Science Facility, preserving the earlier bank. The latest replay built
the Depot and Academy in parallel, saved after the Depot and attached
Covert Ops completed, then completed the Academy and paid 25 minerals
and 75 gas through the ordinary Barracks Ghost order. A test assertion
stopped the replay before the Ghost was born. A fresh title-L run restored
the unfinished Academy and its assigned SCV, and completed the Academy.
The exact repeat spent 25/75 to queue a Ghost and saved it. Another fresh
title-L run restored that active queue, trained a Ghost at frame 12,415,
and saved and fresh-loaded the live actor while preserving the earlier
queue bank. A separate defended fork fresh-loaded a second Command Center
with a clear Silo socket, four Marines in a Bunker and two gas gatherers.
It saved a completed Starport, but lost the new Science Facility before
completion. Fresh-load the Starport bank, defend the next Science site and
qualify the Silo, missile and paid research from that match.
Finish native timing, other lobbies, combined journals and AI before
enabling it.

Other core rules to prioritize after this:

- **Terran construction:** verify ordinary replacement-SCV behavior on N64.
  The GBA computer can assign a new SCV to a paid, abandoned foundation;
  the human repair action requires a completed structure. Add a shared
  recovery command if confirmed, preserving payment, progress and ownership
  through save/load and preventing duplicate builder assignments.
- **Zerg production:** replace the normal custom Hatchery queue with stock
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
| S3 | P0 / open | Qualify saves throughout each combined campaign, including early Protoss missions under the P10 product. [One early physical P1–P7 save point each](audits/2026-11-06-early-protoss-save/README.md) now reloads with matching selected-state continuation on the pre-#323 `ff94a2ea` P10 ROM; P1 also has a matching mid-mission controller-tail continuation, but its historical tape no longer wins on the packaged product; [Regular P10 on November 17](audits/2026-11-17-protoss-early-replays.md) now proves a P1 fresh load through victory. P2/P3 win uninterrupted. P2 reload resets the remembered menu row and misses a Gateway; P3 reload has 100 fewer minerals at the next checkpoint. The [November 18 repeat-command release](audits/2026-11-18-saved-repeat-command.md#early-protoss-campaign-continuation) wins both saved routes on normal P10 from the same cuts. [Four current cartridges](audits/2026-11-17-save-ui-product-matrix.md) pass 32 custom-game fresh loads, and Races/Unified pass the opt-in host fixture. The candidate also wins Z1 and falls back to a valid older P3 bank after a malformed newer UI record. T1 saves and loads before the old input tape diverges. The [regenerated T1 route](audits/2026-11-17-save-ui-product-matrix.md) reaches Barracks with one fewer menu move and wins in both HOST builds. Both rebuilt ARM ROMs now win after the pause reload on their own tapes with the same named victory state and all 11 checkpoints. [Current P10 Homeland](audits/2026-11-13-p7-carried-normal-home.md#november-20-normal-cartridge-and-fresh-sram) now wins from a fresh load at frame 8,845, with all 49 remaining checkpoints matching uninterrupted play. The normal loader clears the selected Gateway and retains its four queued units. Continue qualifying other campaign save points with the normal loader and the current controls. | Fresh-process ordinary save/load and continued wins across all campaign entries on current combined flags, with source-owned effects, actors, clocks and objectives preserved. Reproduce and fix each rejection; do not infer arbitrary-save support from one point. |
| C3 | P1 / partial | All 30 mission controllers now win on current source with ordinary input (HOST). Z10 has its first win (`tests/sc64_z10_campaign_run.c`: a Sunken Colony fortress with Hydralisks through the first waves, then a Lair, Spire and 34 Mutalisks to the Temple, escorting the crystal Drone; frame 58,795, with a fresh-process save continuation to the same win). The [Z10 ARM replay](audits/2026-11-12-z10-arm-qualification.md) now reproduces that win and 59 matching checkpoints after a fresh SRAM load. Z10 also passes all ten HOST campaign starts/saves and the 234 custom startup/save lineups. [Carried Z7](audits/2026-11-12-z7-normal-physical.md) wins from HOME on the normal Z10 cartridge at frame 194,998 with 253 matching observations after the route-pure controller starts its paid air plan earlier. Its full replay averages 9.35 emulated updates per second. The November 13 Z10 KODAK cartridge also wins after a fresh SRAM load at frame 185,926 and one ordinary A press to reselect the Hatchery; all five remaining observations match. [Carried Z8](audits/2026-11-12-z8-normal-physical.md) wins from HOME on the same normal Z10 cartridge at frame 45,468, with 19 matching observations, both Nexuses destroyed and Kerrigan at 400 HP. [Carried Z9](audits/2026-11-13-z9-normal-physical.md) wins from HOME on that cartridge at frame 72,628, with 32 matching observations and a live Drone carrying the crystal back to the hive after the countdown. Several wins are timing-sensitive (Z6, Z8, Z10, P8). [Carried Z3](audits/2026-11-12-normal-campaign-replay-gaps.md) wins twice on HOST with identical route-pure tapes and wins from HOME on the normal Z10 ROM at frame 136,588 with 465 matching physical replay observations. [Carried Z4](audits/2026-11-12-z4-normal-physical.md) also has a route-pure HOST tape and a normal Z10 win at frame 34,984 with 42 matching observations on the nearest-first runtime. [Carried Z5](audits/2026-11-12-z5-normal-physical.md) wins at frame 48,347 on the same normal Z10 runtime, with 55 matching observations and separate physical acknowledgments for authored text and the source pause. [Carried Z6](audits/2026-11-12-z6-normal-physical.md) wins on the normal Z10 ROM at frame 56,913 with 65 matching observations, including Kerrigan boarding and leaving the Overlord. [T1 and T2](audits/2026-11-13-t1-t2-normal-physical.md) win from HOME on the exact KODAK Terran cartridge at frames 10,120 and 29,366 with all retained scalar checkpoints matching and authored pages acknowledged. [T3 and T4](audits/2026-11-13-t3-t4-normal-physical.md) also win from HOME on the exact KODAK Terran cartridge at frames 54,000 and 8,911, with all recorded checkpoints and authored acknowledgments verified. [T5](audits/2026-11-13-t5-normal-physical.md) now wins from HOME on the normal Terran T10 cartridge at frame 122,281 with 150 matching observations, both heroes alive and no hostile buildings. [T6](audits/2026-11-13-t6-normal-physical.md) now wins on exact KODAK Terran at frame 23,536 after replacing a host-only menu write with physical keys and isolating route queries. [T7](audits/2026-11-13-t7-t9-normal-physical.md) wins from HOME at frame 3,931 with Kerrigan alive and the Psi Emitter delivered. [T8](audits/2026-11-13-t8-normal-physical.md) wins from HOME at frame 139,216 with all 272 observations matched. The T7/T9 audit also verifies [T9](audits/2026-11-13-t7-t9-normal-physical.md#t9-new-gettysburg) through victory at frame 47,266 with all 51 observations matched, Kerrigan alive and all hostile Protoss buildings destroyed. [T10](audits/2026-11-13-t10-normal-physical.md) wins from HOME at frame 3,361 on the same Terran cartridge, with all six checkpoints matched, the Ion Cannon destroyed and Raynor alive. P3–P6 use isolated route observers. On the [November 17 regular P10](audits/2026-11-17-protoss-early-replays.md), P2–P6 win from HOME and P1 wins after a fresh load. | Keep the distinct Z9 save path; qualify a neutral carried Z7 save cut or persist the selection safely; Temple/crystal text and results screens; finish latest-ROM and fresh SRAM replays; keep all 30 winning as the engine changes. |
| N1 | P1 / in progress | Validate N64 actor records, game clock and event cadence before behavior-changing timing optimizations. The [matched T1 Marine STOP/no-STOP replay](https://github.com/ggu/starcraft-gba/blob/5445fb3d/docs/audits/2026-11-07-n64-stop-control/README.md#2026-11-12-matched-replay-without-stop) has identical RAM and frames at both pre-press boundaries. C-Down clears the move order and target by step 7,230; with no press the same Marine is farther along and still has the destination pending. The [matched native Start trial](https://github.com/ggu/starcraft-gba/blob/d44739a695afe28ef1ac9070c4d140029cd26bbf/docs/audits/2026-11-07-n64-stop-control/README.md#2026-11-12-matched-start-pause-control) opens Game Paused while retaining the pending move and target. The [matched menu Resume trial](https://github.com/ggu/starcraft-gba/blob/f2898c66202c4c66a6ffd75086c856a92468877f/docs/audits/2026-11-07-n64-stop-control/README.md#2026-11-12-matched-options-and-resume-controls) continues the same move; a second Start opens Options on the N64, while GBA field controls use Start as a Resume shortcut. A RAM counter freezes and advances with Resume, but its purpose is unproven. The [November 18 native actor replay](audits/2026-11-07-native-actor-differential/README.md#november-18-reproduction-from-retained-inputs) recovered the original observer sources and reproduced three frames and both RAM captures from retained inputs; the callback log differs. | Trace actual writes, compare idle and other actors, and establish an independent game clock before using it for timing. Keep snapshots bounded and read-only, with no MMIO side effects. The paired samples do not establish the precise stop tick or a selected-unit pointer ABI. |
| P1 | P1 / in progress | Reduce representative busy-frame cost toward 30 fps. The [2026-11-10 instruction-level profile](audits/2026-11-10-busy-combat-profile/README.md) found route floods at a third of custom-battle cost. Reusing evicted route fields from the spare bank, keeping fields across structure changes, an interrupt page flip, a cheaper placement cursor and cached tracer values cut three pinned 140-unit custom windows by 33–49% in mean emulator cycles, with HOST traces identical to the base in all 54 custom lobbies, and the P7 campaign window by 4%. A second round (see the audit's [worst-update section](audits/2026-11-10-busy-combat-profile/README.md#round-two-the-worst-updates-2026-11-12)) cut the slowest Zerg update by 15% and the Protoss one by 5% (cheaper depot slot scan and goal-cell test, incremental walkability update) with identical traces; the mean barely moves. Those windows are still two to three times the budget. | The [dense point-flood probe](audits/2026-11-12-dense-point-flood.md) ([#389](https://github.com/ggu/starcraft-gba/pull/389)) saves 1.76–3.32% mean and 13.36–22.27% peak loop cycles across the three paired ARM custom windows, with matching sampled workloads. All nine ROM verify targets pass, the 64×64 Z1 campaign wins in matching ARM replays, and Z8 produces identical SRAM and a matching fresh-process win. The 54 bounded HOST lobby trace pairs match. It is on for eligible maps in the canonical builds. Next: address depot floods after structure changes, Arbiter bookkeeping, target-index work, and terrain and unit drawing. Exact-source pairs on the pinned windows with trace identity; confirm on GBA hardware. |
| C4 | P1 / open | Replace campaign AI scalar approximations where verified source/native behavior differs. Late-Zerg waves use averaged groups/gaps; arrival, rebuilding and transport tactics are not equivalent to script commands. | Source script interpretation plus N64 observations of actual production, wallets, launch groups, targeting and rebuilds. Reproduce a concrete mismatch before changing AI; retain mission objective semantics. |
| U1 | P1 / partial | Scourge/Spire source restrictions and shared picker correction are merged in #290. Continue the broader campaign PUNI/UPGR/TECx audit, especially P10. | Preserve current controlling ownership, nonspending denial and full supported custom catalog. Apply remaining source permissions with per-owner tests and save compatibility; do not infer complete availability parity from this one product fix. |
| U3 | P0 / partial | Shared Forge/Core research. Done: the Protoss P10 ROM's custom games (SRAM, integer combat; [audit](audits/2026-11-11-protoss-research-sram/README.md)). Open: campaign missions (needs the generated source-policy header), computer-opponent research, Terran/Zerg/Showcase ROMs, Storm and other paths that bypass shield armor, hardware. Q8 fractions and Flash stay staged and unused. | Authenticated owner and payer identity; source initial/max levels; atomic paid producer jobs, power/cancel/death/capture/reset behavior, active-job saves and legacy migration; weapon/armor/shield/range effects on every relevant path; ordinary building menus and cross-product campaign/custom/Showcase checks. |
| U2 | P1 / partial | Carrier and Interceptor behavior. **Regular Protoss:** paid production, AI, flight and saves in custom games and P7–P10, including the shared Gantrithor service in P10. **Showcase, Terran and Z10 previews:** ordinary ARM controls paid for both units and saved and loaded both a live projectile and a docked Interceptor. The [ARM gate](CARRIER-PROJECTILES.md#pinned-arm-release-gate) qualified campaign openings, sampled custom lobbies, one generation of custom SRAM migration and short stack samples. All three combined previews now have exact computer purchase debits and active-shot ARM saves from blank three-AI Blood Bath matches. The [Z10 run](CARRIER-PROJECTILES.md#mixed-blood-bath-battle-from-blank-sram) also has a 1,000-update continuation through a fresh reload. **Regular Showcase, Terran and Zerg:** service remains off. | Human purchases and sustained battles on the combined previews, full-pool saves, broader journal fallback and physical GBA stack and timing. Protoss: Gantrithor rescue stock is assumed; ARM Gantrithor combat replay and hardware timing remain open. |
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
   The [batched target-index probe](audits/2026-11-12-target-index-bulk.md)
   saves 0.86–0.89% in two earlier normal Z10 windows with matching state
   and drawn pages. The probe stays off pending current cartridge and
   longer custom-match checks.
   The [Z7 census probe](audits/2026-11-13-z7-validated-census.md)
   matches sampled gameplay and pixels with 0.34% lower steady loop cost.
   Keep it off while larger sustained movement and render costs remain.
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
- Pick up the [world depth overlap cases](audits/2026-11-19-world-depth.md)
  when extending the framebuffer replay harness. The N64 Challenger
  overlap click selected the foreground Overlord over a Hatchery.
  [Draft #587](https://github.com/ggu/starcraft-gba/pull/587) applies the
  same depth rule to cross-kind opaque hits. The HOST fixture and
  ordinary P10 and Terran ARM replays match that selection. Showcase scene
  navigation and Z10 Blood Bath save/load also pass. Qualify additional
  poses and compose with live flight depth before enabling it by default.
- Add [fixed-camera fog and HUD-boundary coverage](audits/2026-11-19-world-depth.md#cache-and-clip-regressions)
  to the world-depth replay harness. Keep the pixel checks for both capture
  modes and the legacy static-clip fixture.
- Use the [dynamic depth harness cases](audits/2026-11-19-world-depth.md#dynamic-depth-harness)
  for the next rendering qualification. Start with lift, landing and the
  mid-flight save boundary, then cover Interceptors, detached effects and
  picking against the same opaque overlap oracle.
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
