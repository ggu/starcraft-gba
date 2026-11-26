# StarCraft GBA goal and prioritized TODOs

Updated: 2026-11-26. This is the living backlog for the race-ROM/N64-parity goal.
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

- **Disjoint cycle budgets.** The two flagged Outpost samples reconcile using the guest loop timer. Measure simulation and actor-rendering changes together against the 30 fps budget across products, idle and combat; verify saves and both display pages. The Zerg, Terran and Protoss replay ceilings and their limits are in the [cycle-budget audit](audits/2026-11-26-disjoint-cycle-budgets.md).

- **Packaged race ROM save compatibility.** The `51c7f18f` package loaded
  26 earned campaign victories (T1–T10, Z1–Z6 and P1–P10). All observed
  result worlds, SRAM re-exports and chapter advances matched the standard
  cartridges used to earn them. Extend the comparison as the remaining Zerg
  journals become available.
  [Results and reproduction](audits/2026-11-26-packaged-campaign-results.md).

- **Custom Game defense targeting.** Skipping cells that contain only friendly
  units reduced mean advancing-update cost by 2.08% in the crowded Zerg
  Blood Bath replay (18.00 to 18.35 updates/s). Sparse Terran timing was
  effectively flat. Validate the merged build on hardware and keep prioritizing
  ground orders and actor drawing against the remaining frame budget.
  [Paired timing and Terran placement measurements](audits/2026-11-26-z10-triple-zerg-pacing.md).
  The [normal release comparison](audits/2026-11-26-skirmish-release-pair.md)
  passes 108 ordinary lobby and fresh HOME pairs across all four products.
  Three Blood Bath Melee pairs draw different valid starts. The exact
  packaged Z10 replay from the retained battle save matches its gameplay
  and bitmap data while reducing mean advancing cost by 2.08%.
  New physical whole-match wins qualify Terran Outpost versus one Zerg
  and Protoss Blood Bath Melee versus Terran and Zerg, including fresh
  HOME reload of each victory.

- **Busy three-opponent performance.** The release's six recorded battle
  windows measured 11.29–16.73 loop updates/s. The original controls and
  camera are reproducible from the match traces. Prioritize ground orders
  and depth-sorted actor drawing in the slow Zerg windows, then measure the
  other races on both maps. Reusing the HUD supply counts across six
  256-update battle replays saved at most 0.20% of total cycles and cost
  0.08% in Zerg Outpost. Both bitmap pages and the palette matched at all
  1,542 samples. Keep the existing HUD and prioritize orders and drawing.
  Compare total guest cycles, actual bitmap
  changes, save continuation and on-device pacing. See the
  [GBA measurements and native N64 field count](audits/2026-11-26-three-opponent-battle-pacing.md).

- **Packed Pylon, Gateway and Forge rendering.** The opt-in renderer saved
  0.64–3.29% of guest cycles across six regular P10 battle windows and
  matched all sampled bitmap pages. Check busy Showcase play and handheld
  presentation before enabling it by default.
  [Cartridge evidence](audits/2026-11-26-protoss-base-building-draw.md).

- **Zerg weapon lookup.** The Zerg release build omitted the indexed weapon
  art lookup used by the other three products. The shared setting reduced
  cycles in all six Zerg skirmish windows with the same art on both pages.
  All six full matches and the Zerg finale reproduced their victory saves.
  Hardware timing remains open. See the
  [Zerg10 lookup audit](audits/2026-11-26-z10-weapon-lookup.md).

- **Route field work.** In an 80-update Zerg Blood Bath sample, the 44
  dense point floods all used distinct goals on the same walkability grid.
  Serving the observed direct and nearby-root requests took 122,980 full
  flood dequeues. Offline FIFO prefixes reached the observed origins in
  25,706 dequeues. Test whether a bounded resumable implementation can
  deliver a whole-frame gain while preserving every direction, secondary
  query, topology invalidation and memory guard.

- **Refinery economy and native timing.** The opt-in FIFO service admits one
  worker per site, serves separate sites concurrently and restores queue order
  from SRAM on all four products. All 18 race/map/opponent-count cells have
  ordinary-input cartridge wins with fresh pause continuations and result
  reloads. With identical starting positions and controller strategies, all
  three release Blood Bath wins became losses on the trial. Native service
  lasted 37 guest ticks across 18 fully observed visits. The GBA timer took
  1.98 emulated seconds in a light Z1 sample and 3.09 in busy P10. Improve
  busy-frame pacing and compare native elapsed time before enabling the
  change by default. Back up pretrial SRAM for rollback. [Native and GBA
  timings](audits/2026-11-26-gas-saturated-fifo.md), [three-opponent
  battles](audits/2026-11-26-gas-fifo-skirmishes.md), [one- and two-opponent
  battles](audits/2026-11-26-gas-fifo-small-skirmishes.md).

- **Campaign playthroughs with the trial economy.** All 30 retained victories
  advanced or restarted their finales and reloaded new pause saves. A blank
  Terran campaign cleared T1–T10 from blank SRAM with ordinary result
  advances and fresh result reloads. The actual T2 save carried through
  strict T3 and T4 comparisons, a retained paid-input T5 route, then strict
  T6–T10 comparisons. T5's uninterrupted HOST tape diverged and was not
  treated as a match. [Carried trial evidence](audits/2026-11-26-fifo-terran-early-campaign.md).
  P9→P10 and Z10 also cleared from retained predecessor victories. Track
  first sustained divergences from the release plans as the earlier missions
  are played. [Terran](audits/2026-11-26-fifo-terran-campaign.md),
  [Protoss](audits/2026-11-26-gas-saturated-fifo.md),
  [Zerg](audits/2026-11-26-z10-fifo-normal.md).

- **Terran campaign after the Custom Game Larva merge.** The standard
  cartridge cleared T1–T10 from blank SRAM. T5 used the committed paid
  air route. Each subsequent mission loaded the preceding victory; T10
  retained all ten completion bits (0x3ff0) after a fresh boot. Performance
  remains open: the T4 main loop averaged 24.60 updates/s on this route.
  [T1–T5](audits/2026-11-26-terran-linked-opening.md),
  [T6–T7](audits/2026-11-26-terran-t6-t7-linked.md) and
  [T8–T10](audits/2026-11-26-terran-t8-t10-linked.md).

- **Standard Protoss cartridge after the Custom Game Larva merge.** P1 cleared
  from HOME on cartridge SHA-256
  `967e2b35152df87ac05f98d4ee0ccd81f3c1634066503306257eb7450296d4ff`.
  The older recording diverged at call 6,000. A new controller recording
  matched every 1,000-call checkpoint and the final state at call 28,253
  (frame 27,764, zero enemy buildings, zero faults). The last dialogue
  acknowledgement exposed victory immediately before the recorded neutral
  release. A fresh boot loaded the same world and re-exported identical SRAM
  (`a5183059b513536784634e374ce4ab56cba0ecca3abdbe38037ea46415ae7475`).
  P2 carried that save to victory at frame 39,217. Its 42 checkpoints
  and fresh cartridge reload passed. P3 carried the P2 result to victory at frame
  48,801, with its primary objective complete and all three completion bits retained.
  The same save then carried P4 to victory at frame 84,220, with both
  heroes in extraction and a byte-identical fresh result readback. P5 then
  cleared at frame 5,057 and retained all five completion bits after a fresh
  load. P6 followed at frame 51,746, retaining the first six completion
  bits after a fresh load. Homeland then won from the P6 result at frame
  41,224, with all three heroes alive and seven completion bits after a
  byte-identical fresh readback. P8 continued at frame zero and won at
  frame 93,065. The Stasis Cell was destroyed, both heroes survived, and
  all eight completion bits restored from a fresh boot. P9 and P10
  then won from the preceding result at frames 101,649 and 53,225.
  The final fresh readback retains all ten completion bits (0x3fc0000c)
  and re-exports identical SRAM.
  [Standard Protoss finale continuation](audits/2026-11-26-protoss-standard-finale.md).
  [Homeland replay and evidence](audits/2026-11-26-protoss-p7-linked/README.md).
  [Linked Protoss replay and evidence](audits/2026-11-26-protoss-opening-continuation.md).
  Use
  [the pinned-input verifier](../tools/sc64_early_protoss_home_replay.py)
  to reproduce the HOME-to-result and fresh-load checks.

- **Campaign save continuity on the finite-gas cartridges.** The packaged
  Protoss10 cleared P9 and P10 from a retained P8 victory, with fresh result
  saves after both. The older P9 tape diverges on both sides of the gas
  change. [Route, SRAM hashes and repeatable check](audits/2026-11-26-protoss-finale-chain.md).
  The release Terran cartridge cleared T8, T9 and T10 from a retained T7
  victory. All three results reloaded with identical SRAM.
  [Linked Terran replay](audits/2026-11-26-t8-finite-gas-route.md).

- **Custom Game pack correction.** The `b206b062` race binaries used an
  older 32-patch Blood Bath pack; Showcase used 56. All four cartridges
  regenerated from `d39bd015` use 56 source patches (styles 176/177/178:
  19/17/20). Tested older saves restored and matched for 512 updates; 72
  lobby combinations and all 30 saved-result transitions passed. The product
  build gate checks the header actually selected by include precedence.
  Hardware pacing remains open.

- **Zealot native vitals.** All 30 original campaign maps select the
  ordinary Zealot defaults. Two completed Zealots in retained native
  Blood Bath states had 80 HP and 80 shields. The shared rule uses
  80/80 for raw 65. Older saves transfer HP above 80 into shields.
  On the integrated race cartridges, fresh mGBA loads restored 51
  Zealots across two Terran 9 saves, 30 in a Zerg 10 save, and 47
  across three Protoss saves. All eight tested saves, including two
  without Zealots, could be saved and reloaded again. The 72 skirmish
  lobby combinations launched with the expected map, race and seats.
  The retained Protoss 7 late assault won at frame 80,810 and its
  victory save survived a fresh load. Terran 9 matched all 51 prior
  checkpoints from the Terran 8 victory handoff, won at frame 47,266
  and reloaded its victory save byte for byte. Zerg 10 won from an old
  frame-60,123 save on the cartridge in mGBA. Its old full-mission tape
  first differed at frame 8,354 and later restarted the mission. A new
  host controller played a replacement route. From HOME in mGBA, the
  Zerg cartridge replayed its 202,274 button updates, matched all 97
  checkpoints, delivered the crystal and won at frame 198,954. A fresh
  HOME load restored the victory and re-exported identical SRAM. The
  same route from a retained Zerg 9 victory matched all 97 checkpoints,
  retained all ten Zerg completion bits and reloaded byte for byte.
  Separately, all 30 retained campaign victories advanced to the next
  chapter or retried their finale. Each resulting pause save reloaded
  in a fresh mGBA process with the same state and SRAM bytes.
  With the combat optimization from #893 included, all 30 transitions
  and 72 lobby launches passed again. On two busy Zerg skirmish saves,
  1,024 further updates matched both display pages, actors and world
  state at all 129 samples per map. Pause saves matched byte for byte.
  Whole-update guest cycles fell 0.50% on Outpost and 0.12% on Blood Bath.
  These short fixed-camera windows do not establish 30 fps on hardware.

- **Blood Bath resource layout.** Repacking the verified N64 map restores
  56 mineral patches, with 32 containing 1,500 minerals and 24 containing
  3,000. The original pack stopped after assigning eight patches per
  starting site. The first 32 indices retain their previous coordinates
  and amounts. New games on all three race ROMs and Showcase matched
  the source coordinates, amounts and three mineral sprite types in mGBA.
  Fresh loads restored mined amounts and continued play. Old 32-patch
  saves re-exported byte for byte on all four ROMs and matched the
  original builds for 512 further updates. In three-CPU matches with each
  human race, workers mined a restored patch within 4,500–6,500 updates.
  All three matches ran for 12,000 updates. Over the final 500 updates,
  mGBA guest cycles corresponded to 24.55 updates/s on Terran, 17.99 on
  Zerg and 19.60 on Protoss. These are different battle states. Continue
  profiling late matches and verify frame cadence on hardware. All 72 lobby
  launches across the four cartridges, both maps, three human races and
  one to three independently chosen CPU seats reached gameplay. The
  running matches retained the chosen races and occupied seats.
  Regenerate the private skirmish header with
  `tools/sc64_skirmish_pack.py` before building. Use `--cover` for
  cover-enabled profiles and their existing generated output path.

- **Next four-player N64 map.** Power Lines (008/055) has four connected
  starts under the importer’s ground-walkability test, 74 mineral patches
  and ten geysers on a 128×96 map. Its 361,216 bytes of deduplicated
  terrain, tile indices and walk flags fit all four current ROMs. Before
  enabling it, extend the Terran four-geyser limit and the Zerg custom
  save format’s ten-bit coordinates and 64×64 fog. Check all four starts
  with ground units and AI expansion in mGBA. Spaced Out (008/050)
  fails the ground-connectivity test and needs transport-capable opponents.

- **Gas depletion.** The owned Blood Bath scenario assigns 10,000 gas to
  each of its four geysers. In the N64 emulator, selecting a geyser in a
  fresh Blood Bath game showed 10,000 gas. Selecting a nearby mineral
  field after workers started harvesting showed 1,492 minerals. The
  skirmish importer now keeps each source site’s full 32-bit gas quantity
  in both the map descriptor and private report. Regenerating Blood Bath
  preserved the four existing coordinates and recorded 10,000 gas at each.
  The [N64 and GBA gas audit](audits/2026-11-26-n64-gas-pickup.md)
  records natural eight- and two-unit cargo and the shared worker
  implementation. Rebuilding a preplaced refinery, preserving its balance
  through save/load, the 30 chapter transitions and four product stack
  budgets are verified. Terran 10, Protoss 10, Zerg 7 and Zerg 10 all reached victory
  with finite reserves and fresh-loaded their results. The four candidate
  cartridge builds and their HOST match authors now enable the same rule.
  Each race won on both maps against one, two and three opponents in
  ordinary-input mGBA matches. All 18 pause continuations reached the same
  victory and all terminal saves reloaded. The three-opponent Zerg Blood
  Bath win used three allied Zerg CPUs. A win against the mixed Terran,
  Zerg and Protoss alliance and hardware timing remain open.
  The conditional native AI bonus still needs an observed qualifying case.

- **Packaged Terran campaign.** All ten missions now have individual
  opening-to-victory runs on Terran10 `5f758095` (ROM SHA-256
  `1779e2ef70d4858a599cce2a32249fcaff9fc129e80e30a26fad425879975960`).
  The final outstanding replay, T8, matches the historical checkpoints
  through input 143,260, finishes at frame 139,216 and restores the victory
  in a fresh emulator. T5 uses new attack orders and its own pause saves.
  T9 reaches victory with the old inputs and 13 divergent checkpoints.
  Every chapter's victory journal re-exports byte for byte. The private
  ten-mission receipt index hashes to
  `c632a63424a53882bb092bc01911a3ff2ebb0d7ec3dc8e9ec8542c29856a2937`.
  On a continuous save, T2, T3 and T4 win at frames 29,366, 54,000
  and 8,911. The T5 opening reloaded at frame 19,419 with the four
  completion flags (0xf0). That run cleared T5 at frame 145,471 with
  both heroes alive and all enemy buildings destroyed. Fresh SRAM
  re-exported byte for byte with all five bits (0x1f0). The same save
  cleared T6 at frame 23,536, T7 at 3,931 and T8 at 139,216. Each
  victory reloaded and re-exported byte for byte. T8 retains all eight
  flags (0xff0). Starting with that T8 victory journal, T9 matched the
  packaged physical reference through victory at frame 47,266. T10 then
  matched all six checkpoints and won at frame 3,361. A fresh load
  retained all ten Terran bits (0x3ff0) and reproduced the final SRAM
  (SHA-256 `49c9e531cd692502487592025127543b57d6d7c2d692f69b648a7bc25865ce6a`).
  Pressing A retried T10, and a frame-61 pause save reloaded with all ten
  bits. This completes the continuous ten-mission runs on all three race
  cartridges in package `5f758095`. T9 averaged 17.74 emulated updates/s
  and T10 averaged 18.52, including presentation waits and menu input.
  On the finite-gas release, a newly authored T8 tape was replayed from
  a retained T7 victory in mGBA. All 467 checkpoint ticks matched without
  wallet or objective exceptions; T8 won at frame 285,946 and its result
  reloaded with identical SRAM. Its actual victory save advanced to T9,
  where a newly authored finite-gas tape matched all 54 checkpoints and won
  at frame 49,486. Its victory advanced to T10, which matched all six
  checkpoints and won at frame 3,361 with completion mask `0x3ff0`.
  Both results reloaded and re-exported identical SRAM. This carried
  T7-victory-to-T10 segment is documented in the [finite-gas T8 audit](audits/2026-11-26-t8-finite-gas-route.md).
  Late-campaign speed and hardware frame pacing remain priorities.

- **Zerg campaign after the Custom Game Larva merge.** Z1 cleared by two
  ordinary routes from blank SRAM on the standard Z10 cartridge. The
  continuous HOST-matched route handed its actual victory to Z2, which
  delivered the Chrysalis and reloaded with both completion bits. The linked
  Z3 paid route won at frame 68,639, with the Chrysalis at 170 HP. A fresh
  boot re-exported identical SRAM and A advanced to Z4 with all three bits.
  Z4 carried that result through victory and a fresh SRAM reload at frame
  64,147. Z5 then matched all 14 recorded stage worlds and won at frame
  18,974. The old Z6 route ended without victory. Paid reinforcements and
  a replacement assault cleared Z6 at frame 212,428 and its fresh result
  reloaded with all six bits. Z7 continued from that save, matched all 206
  same-cartridge observations and won at frame 267,585.
  The previous Z8 recording diverges at call 9,696. The optional
  `--gather-west` controller cleared Z8 from HOME with Kerrigan at full
  health. The earned Z7 save also cleared Z8 at frame 70,229 with all
  29 checkpoints matching. Continue the earned chain through Z9 and Z10.
  [Z3–Z6 carried results and Z8 route](audits/2026-11-26-zerg-linked-middle.md).
  [Z1 replay and Z2 handoff](audits/2026-11-13-z1-normal-home.md).

- **Zerg campaign progression.** Z1 now wins from HOME on the packaged
  Zerg ROM. The opening tape reaches a frame-984 pause, then 17 normal
  controller segments load saves made in this run. Victory at frame
  49,270 re-exports byte for byte on fresh HOME and matches the earlier
  victory journal (`6122561e55f2ab688b55a6217f09b22188b53fe826113860bdfff04bd90992eb`).
  Advancing to Z2, saving and reloading preserves the Z1 bit at frame 101.

  Continuing that Z1 save, Z2 delivers the Chrysalis at frame 10,209.
  Z3 uses 32 controller segments and wins at frame 68,639 with the
  first three completion bits (16,387). The Z3 victory journal SHA-256
  is `277b5e01e2e8293b7c5c719d7ea84e24f9f71a508c8bdd13b3cfbd985ab8534d`.
  From this result, Z4 rebuilds the Spawning Pool, upgrades to Lair,
  produces 20 Mutalisks and destroys the final Command Center at frame
  64,147. Z5 follows on the same save and wins at frame 18,974 with
  all five completion bits (114,691). Each chapter victory re-exports
  byte for byte after loading it in a fresh emulator.

  The same save clears all 52 Z6 controller segments and wins at
  frame 177,044. After a fresh load, Z1–Z6 remain marked complete
  (245,763) and the re-exported SRAM hashes to
  `f64dd51818b990c66e9c45d077d2ae89ee51049eea8af1f01045b9b21fc1f288`.
  The separate Z5 HOME recording remains active after 3,000 additional
  neutral inputs beyond the end of its tape.

  From the six-chapter save, Z7 also wins at frame 267,585 with all
  206 observations matching. A fresh load preserves all seven chapter
  flags (507,907) and reproduces the victory SRAM byte for byte. The
  same save then clears Z8 at frame 86,969, matching 36 checkpoints.
  A fresh load preserves all eight completion flags (1,032,195) and
  re-exports SRAM with SHA-256
  `2b9cbe6a24cb73e26ebd42bf3da87b8b7c14f3f05bd6366a699ec698bcc88497`.

  Independently, Z7 wins from HOME on this ROM at frame 267,585 with all
  206 checkpoints matching. The fresh-loaded victory advances to Z8.
  The Z8 field save restores at frame 107 with the Z7 completion bit and
  re-exports byte for byte. Z9 now wins from the crystal-spawn save on
  the packaged ROM. A Mutalisk diversion north of the return location
  let the Drone deliver the crystal from the south. Loading victory at
  frame 59,957 advances to Z10. The Z10 frame-101 pause save restores the
  Z9 completion flag and re-exports byte for byte. Starting at that
  Z9 victory, Full Circle plays through all 28 checkpoints to victory
  at frame 63,523, with both final chapter flags (3,145,728) and zero
  unsupported events. A fresh process restores the campaign and world,
  re-exporting the same SRAM. A on the finale result retries Z10 and
  keeps both flags through a pause save and fresh load.

  The continuous Z1–Z8 save now clears Z9. The crystal spawned at
  frame 46,365 and a Drone delivered it at frame 57,787. The victory
  journal re-exported byte for byte (SHA-256
  `71e4d60f9e6a6f99c84ce3a7fb00d31a77c0fa6e8eba7e3af252c52e150e4b97`).
  Z10 resumed at frame 101 with all nine prior flags (2,080,771).
  Full Circle then won at frame 63,523 with all 28 checkpoints matched,
  zero unsupported events and all ten flags (4,177,923). A fresh load
  restored the victory state and re-exported identical SRAM. Pressing A
  retried Z10. Its frame-101 pause save reloaded with all ten flags.
  This verifies the complete continuous Zerg campaign on the November
  25 package in mGBA. Recheck affected chapters on later engine builds.
  The authenticated Z9 scenario allows player 3 to research Ventral
  Sacs. On the newer source merged in #874, Z9 passed paid research,
  mid-job reload, boarding, loaded-cargo reload and unloading in mGBA.
  The continuous campaign run above used the earlier package and a
  ground delivery.

- **Packaged Protoss campaign.** P2 now wins from HOME on the packaged
  Protoss10 ROM with regenerated controller inputs. The 38,435-input run
  reaches victory at frame 36,388 with zero unsupported actions. A fresh
  HOME load restores the victory world and completion flag and re-exports
  the same SRAM bytes.

  P6 wins at frame 92,986 after all 105
  checkpoints, six briefing pages and 17 dialogue pages match the
  regenerated route. The full run averages 16.81 emulated updates/s.
  It saves and reloads during play, and the victory SRAM re-exports
  byte for byte after fresh HOME
  (`b01855f7314bdc8f5eb017caefa434f6821cc488e8df54bc30832b818bcef031`).
  From that P6 victory, P7 completes at frame 56,695 with all three
  heroes alive and both objective Nexuses destroyed. The same route
  also wins from HOME. Advancing the linked run to P8 and reloading
  its frame-43 save preserves both P6 and P7 completion bits.
  Separately, the P1 save clears P2 and P3, with matching checkpoint
  observations and byte-identical victory readbacks. Its P4 frame-101
  field save restores the first three completion flags (4,194,316).
  The same save then clears P4 at frame 81,129, P5 at frame 5,057
  and P6 at frame 92,986. All three victory saves restore and re-export
  byte for byte, carrying all previously completed chapters. The P6
  readback SHA-256 is
  `e3622fdc9c1796b205a5b44e1a4c4a6d72ecd1233d7e0427cabf0e11a8f46be1`.
  The continuous P1 save then clears P7 and P8. After P8 at frame
  98,849, fresh SRAM retains all eight flags (264,241,164), SHA-256
  `dd39755042242391d0a78f825565e6fe9409d1872c741d57e683801aa41fd121`.
  This journal cleared P9 at frame 134,481, then P10 at frame 53,225
  with all ten bits (1,069,547,532). Each of the ten victory journals
  was independently opened from HOME again on the packaged ROM. Every
  mission number and cumulative flag matched, and every SRAM re-export
  matched its input byte for byte. Final victory SRAM SHA-256 is
  `bad33ae45804fd192f1dfdf439cb52a1e9eb064fa7fb30f9e1b88a010a42cb32`.
  Retrying P10 and reloading its frame-11 pause preserved all ten flags.
  The separate P6–P7 save also clears P8 at frame 98,849. Its fresh-loaded
  victory retains all three completion bits (234,881,024). Continuing
  into P9 reproduces all 57 checkpoint observations and wins at frame
  134,481. A fresh process restores P6–P9 complete.

  A regenerated P8 controller tape
  plays The Trial of Tassadar from HOME to victory on the packaged
  Protoss10 ROM. After 100,630 inputs the stasis cell is gone, all three
  trigger runners are free of faults, and victory registers at frame
  98,849. Fresh HOME restores the result and re-exports identical SRAM
  (`b8624a22d8e77fccd3942ef7f647c67d41c5dd9f16e29d49d418e0bec2445ef4`).
  Advancing to P9 and saving and loading at frame 101 preserves the
  P8 completion bit. The tape SHA-256 is
  `8cd63fc978c64ca7e639cd89432f88c439d1de5bc655be1792b431cdfd1c04d0`.
  Continuing from that P8 victory save, P9 wins at frame 134,481 and P10
  at frame 53,225. Both played through the normal chapter advance with
  controller input on package `5f758095`. All 57 P9 and 22 P10 checkpoint
  observations matched the recorded frame and result state. Each victory
  restored its world fingerprint, mission runtime and completion flags
  in a fresh mGBA process and re-exported byte-identical SRAM. The final
  mask is `0x38000000` (P8–P10). P3 also wins from HOME at frame 47,661
  and preserves its completion bit through a fresh-process save reload.

  P4 also wins from HOME on this package at frame 81,129 after a
  regenerated normal-input controller builds an escort and clears the
  extraction area. Both required heroes survive. A fresh process restores
  the victory and re-exports identical SRAM
  (`2ee0921b6ed24c222ffaa55113ff65fb04f9a4908c80c11d5f5ec16c73814304`).
  The earlier tape diverged by 100 minerals at input 18,000.

  P10 completes from HOME on the same Protoss10 package with a
  regenerated controller. The old tape first differs by 100 minerals at
  input 7,095. The new run matches all 22 checkpoints, reads ten briefing
  pages and 14 dialogue pages, and reloads its in-game save at frame
  31,156. The Overmind is destroyed at frame 53,225 with zero unsupported
  actions. The victory remains stable for eight neutral inputs after a
  fresh HOME load and re-exports byte for byte
  (`4bb1ae15b1eb691cfaf1d09e7c10fe17510b5ffe6975a67bb75fd54ce6a3dc49`).
  The 54,493-input run averages 12.91 emulated main-loop updates/s.

- **Combined campaign qualification.** All 30 original missions now have
  ordinary-input victories on recent normal ARM builds. Z7, Z8 and Z10
  finish on the padded-journal Z10 build. The vitals build repeats the
  [full Z7 win](audits/2026-11-12-z7-normal-physical.md#november-25-vitals-release-replay)
  through all 206 named checkpoints and repeats the full Z8 route and
  late Z10 suffix with identical victory journals. The [selection-ring
  release](audits/2026-11-12-z7-normal-physical.md#november-25-selection-ring-release-replay)
  repeats all 206 Z7 checkpoints through victory. The combined collision
  and production build repeats the full Z8 win and confirms a byte-identical
  victory re-export from fresh HOME. The [adaptive Pylon builds](VALIDATION.md#qualification-with-the-queue-and-contact-changes)
  repeat full Terran 2 and Protoss 5 wins with fresh save and result loads,
  a full Zerg 8 win with fresh victory readback, and the late Zerg 10 win.
  Wasteland and Desperate Alliance also pass full runs on the Pylon
  release, including fresh pause and result loads.
  The [fogged-viewport build](audits/2026-11-25-fogged-tracer-viewport.md)
  repeats Terran 1–4, Protoss 1 and 5, and Zerg 2 and 8 from HOME,
  with fresh victory readbacks on all eight runs.
  [Terran 6–10](audits/2026-11-25-visible-drawing-and-campaigns.md#exact-campaign-checks)
  also pass with fresh pause and result loads, bringing Terran to nine
  exact wins on that ROM. The archived T5 route opens pause and hits
  the restart shortcut at input tick 20,403. A P7 continuation stays
  active past the old tape's late input boundary with the three
  heroes alive and the enemy Nexus standing.
  The [T5 air continuation](audits/2026-11-25-visible-drawing-and-campaigns.md#combined-supply-and-paired-outline-qualification)
  goes from an ordinary frame-88,779 pause to a fresh-load-verified
  victory at frame 143,281, with both heroes alive. It matches all 13
  stage journals on the supply and paired-outline builds. Advancing to
  T6 on the supply build also produces a byte-identical frame-101
  pause and HOME reload. On the packaged `5f758095` Terran ROM, a new
  opening-to-victory T5 route restores its own pause saves throughout. The
  replay reaches the first pause at frame 19,419 with identical save bytes;
  the frame-21,519 save differs from its historical counterpart. The release
  route reaches frame 88,779, then replays the 13 retained input segments.
  Three additional selection and attack sequences finish the six remaining
  buildings. A fresh HOME process restores victory at frame 145,471 with
  both heroes alive, zero enemy buildings, and byte-identical SRAM
  (`0a327a1841ff9a75cc1a0bd127323e8eaf8e91db344655280fd44c3543332393`).
  The paired-outline build repeats [Z7 from HOME through all 206
  named checkpoints](audits/2026-11-12-z7-normal-physical.md#november-25-combined-supply-and-outline-release-replay)
  and wins at frame 267,585. It also replays T3 from HOME to victory
  at frame 54,000, including a pause restore. The recovered P7 assault wins
  at frame 80,828. Z8 repeats the full HOME replay to frame 86,969 and
  Z10 wins from the late frame-60,123 pause. All four restore the
  victory journal byte for byte through fresh HOME.
  Re-qualify the remaining missions on the combined Terran, Zerg and
  Protoss package. Continue native parity and GBA SP timing.
  [Native Protoss supply](audits/2026-11-25-waiting-training-supply.md#native-protoss-follow-up)
  now has paired idle refusal and paid waiting at 9/9. The packaged
  P10 build repeats idle refusal, fresh-load queue persistence and
  resumption after Pylon completion. On native Challenger, selecting a
  Probe at 9/9 exposes its Build command. C-left opens the build card
  with a 100-mineral Pylon selected. A enters placement. Moving the
  green preview toward the geyser turns it red. Confirming there shows
  “You can’t build there” and keeps supply at 9/9. Confirming the green
  preview on a second run starts a blue warp-in. The HUD still shows 9/9
  at step 14,330 and reads 10/17 by step 16,330 with no further orders.
  This agrees with the GBA check that paid waiting production resumes
  on Pylon completion. At step 17,330 the native HUD reads 11/17 after
  the second waiting order. See the [native placement and recovery
  observations](audits/2026-11-25-waiting-training-supply.md#restore-native-supply-with-a-pylon).

  The packaged Terran ROM at `5f758095` restores a retained pause and
  victory save from earlier builds for each of T1–T10 (20 fresh emulator
  processes). Each pause restored the expected mission and frame and ran
  eight more updates. Each victory restored the mission completion bit
  and held the terminal frame through eight updates. All 20 SRAM exports
  matched their inputs byte for byte. The private receipt SHA-256 is
  `f8920c61db85532b944c56f51fb638a1275fb3741f6b8d8a0131fcef504235d4`.
  T1, T2 and T4 also replay from the ordinary campaign menu through
  victory on these exact ROM bytes, with a mid-mission pause and fresh
  HOME reload of each victory. Their terminal frames are 10,120, 29,366
  and 8,911 respectively. The earlier full T3 run on the same bytes
  ends at frame 54,000. T6, T7 and T10 also finish with fresh result
  reloads at frames 23,536, 3,931 and 3,361 respectively.
  On the same cartridge, all nine intermediate victory screens advance to
  the expected next mission with A. After acknowledging opening dialogue,
  each next mission can be paused, saved, and restored through HOME with
  the same mission, frame, prior completion bit and byte-identical SRAM.
  The T10 victory screen retries T10 with A and returns to the title with B.
  The chapter-transition receipt SHA-256 is
  `de52a012fd670f66f3a9b78d6c21c4c779777026b76358af128d4ba315011ebc`.
  Each transition used an individual mission victory save.

  On the packaged Protoss ROM, P1 reaches victory at frame 15,305 through
  16 matched checkpoints. On the packaged Zerg ROM, Z2 reaches victory
  at frame 10,209 with objective flags 17. Z2's input replay checks the
  terminal objective state. Both victory saves reload byte for byte in a
  fresh emulator process. All nine intermediate Protoss chapter transitions
  also reach the expected next mission on this package. Each following
  mission saves and reloads with the same frame and completion mask.
  Z1–Z9 pass the corresponding transitions. The three finales retry
  the final chapter with A and preserve its completion bit through a new
  pause save. The new T5 victory also advances into T6 and reloads at
  frame 101. On T9 the strict historical comparison first diverges at
  input 15,000 (198 minerals and 70 gas versus 498 and 270). The same
  buttons still finish the mission at frame 47,266 with a fresh pause
  reload during play and a byte-identical victory readback. The diagnostic
  replay recorded the differing checkpoints and kept the terminal objective
  and save checks active. Repeat these checks from retained victory SRAM with
  `python3 tools/sc64_chapter_transition.py --help` for the required paths.
  Continue Z9 and the remaining opening-to-victory routes. The original
  P2 inputs leave the mission active, and the historical Z3 cursor
  checkpoint diverges at input 7,269.

- **Busy match pacing.** The [November 24 KODAK builds](audits/2026-11-24-kodak-package-checks.md#three-cpu-cartridge-pacing)
  start at 29.85–29.92 emulated updates/s across six three-CPU cases
  and fall to 14.14–17.90 in the last measured window. Every sampled
  busy update exceeds two VBlanks. The exact Protoss 10 late sample
  spends 396,340 guest cycles per update on depth-ordered actor
  drawing and 152,204 on movement orders. The full packaged Z7 replay
  averages 12.83 emulated main-loop updates/s over 270,585 inputs,
  including presentation waits and UI. For a reproducible campaign
  performance target, use input ticks 136,697–141,598. Both endpoints
  are in play and the simulation advances on 4,866 of 4,901 updates.
  Mean cost is 2,106,169 guest cycles/update (7.97 updates/s); the
  player's unit count stays at 69 and the observed enemy count falls
  from 158 to 157. This is the slowest adjacent checkpoint pair with
  at least 1,000 updates and at least 98% simulation advancement.
  A fresh SRAM restore of this interval identified repeated integer division
  while drawing the command panel border. Sampling its border incrementally
  reduced mean guest cycles from 1,963,913 to 1,113,925 on the four open-menu
  updates in the first 64. The other 60 averaged 1,606,884 before and 1,609,231
  after. Over all 4,901 updates the improvement was 0.22% (8.221 to 8.240
  emulated updates/s). Both builds reached the same endpoint and matched at
  all 72 sampled world and video checkpoints. Paired pause screens matched
  across all four products, as did victory screens on the three race builds.
  In a read-only census on the menu-renderer build, the route-build
  counter advanced on 3,681 of 4,866 advancing updates. Orders averaged
  1,578,347 cycles on those updates versus 552,404 on the other 1,185.
  On the same interval, a read-only instruction trace counted 3,024
  calls to the linear field builder for 32 distinct destination cells.
  All calls saw the same 2,048 walkability bytes. Two destinations
  accounted for 347 calls each. Guest clocks, world and both display
  pages matched the uninstrumented control. These fills consumed
  2,240,666,503 guest cycles, 22.45% of the window. For the two
  most frequent destinations a fill averaged about 735,000–737,000
  cycles. Investigate exact reuse on an unchanged grid, preserving
  the existing cache eviction and path decisions. Measure the entire
  update including lookup and invalidation costs.
  Physical GBA timing remains open.
  [Linking the hottest ROM functions into the unused IWRAM stack
  reserve](audits/2026-11-25-iwram-hot-placement.md) saves 6.87–8.46%
  in five late three-CPU pairs and 2.95% in Protoss Blood Bath, with
  matching state, pixels and saves. Each ROM's stack bound is checked
  at build time. Late updates still exceed two VBlanks. Measure the
  relinked ROMs on hardware and in full campaign replays.
  On the sprite-row release's paired three-CPU P10 Outpost cut,
  automatic targeting takes 131,032 inclusive cycles per update
  and movement takes 161,711 within orders. The fogged viewport
  reduces tracer drawing to 20,305. A separate verified visible
  three-CPU Terran Blood Bath cut spends about 235,488 cycles per
  update in `move_toward`, 9,885 in automatic targeting, and 12,406
  in tracer drawing on matched frames. Prioritize movement there.
  The draft monotone route witness lowers mean frame cycles by 2.25%
  here and 0.58% in a separate P10 Outpost cut. The Z10 release has
  no active monotone probe. Keep the witness in draft while checking
  Z10 integration, long-run pacing and saves. Enabling the monotone
  probe alone on Z10 adds 56 IWRAM bytes and exceeds the stack guard
  by 8 bytes. It also bypasses point-field cache insertion on successful
  probe misses. Review the resulting route behavior before enabling it.
  Keep targeting as a separate priority for Outpost.
  The [qualified Z10 Blood Bath win against three Zerg CPUs](audits/2026-11-26-z10-triple-zerg-pacing.md)
  spends 296,616 cycles per advancing update in orders, its largest
  single measured stage; actors take 180,057. The overall 920,546-cycle
  mean exceeds the nominal 30 fps budget by 361,305 cycles. Prioritize
  collision and movement inside orders and actor drawing; the measured
  depot-validation inlining trial saves only 0.356% over its 512-update
  window and needs broader verification.
  [Sharing the byte lane across thin selection edges](audits/2026-11-25-visible-drawing-and-campaigns.md#drawing-matching-one-pixel-selection-edges)
  lowers the visible P7 and Z9 frame means by about 0.9–1.0%
  with matching pixels. [Pairing opposite rows on the supply release](audits/2026-11-25-visible-drawing-and-campaigns.md#combined-supply-and-paired-outline-qualification)
  saves a further 1.51–1.54% in selected-army samples. Fixed P10 and
  panned Z10 Custom Game samples are unchanged. Keep profiling the
  larger draw and order costs in dense matches.
  The paired-outline race builds pass 144 physical-button Custom Game
  starts, pause saves, and fresh continuations. The tested cases cover
  every occupied CPU-slot pattern and repeated computer races.
  [One-CPU Blood Bath](audits/2026-11-22-blood-bath-melee-victory.md#november-25-packaged-terran-verification)
  now wins on the packaged Terran ROM after an ordinary mid-match
  save and fresh continuation. The victory itself reloads byte for
  byte. Repeat full matches on Outpost and with two or three CPUs.
  Continue hardware timing.
  [Whole two- and three-CPU matches](audits/2026-11-25-custom-game-full-matches.md)
  now win and replay on the race cartridges, each with a mid-battle
  save continued in a fresh process; its table lists the open cases.
  With Larva and Eggs all 18 lobbies win and replay, including Zerg on
  Blood Bath against three Zerg CPUs
  ([Larva and Egg audit](audits/2026-11-25-custom-larva-eggs.md#custom-game-full-matches)).
  [Runs with one or two CPUs on the same cartridges](audits/2026-11-24-kodak-package-checks.md#pacing-with-one-or-two-cpus)
  put late one-CPU Blood Bath near 30 emulated updates/s. Late
  one-CPU Outpost drops to 26.13–28.25, and every late two-CPU
  update exceeds two VBlanks. Preserve the one-CPU
  pacing while tracking both maps at two and three CPUs.
  [Late moving-camera pairs](audits/2026-11-24-kodak-package-checks.md#late-camera-movement)
  cover all six three-CPU configurations. Mean cycle changes span
  −3.75% to +4.92% with matching sampled world state. Every fixed
  and moving update still exceeds two VBlanks, so keep busy field
  and simulation costs ahead of camera-specific changes.
  The [shared contact scan comparison](audits/2026-11-24-contact-axis-bias.md#verified-kernel-placement-and-late-battle-inputs)
  matches all 229 masks from a late three-CPU field. The speed-optimized
  ROM kernel uses 0.41% fewer cycles with the table and 4.06% fewer
  with arithmetic. Retain the ARM and size-optimized comparisons.
  Check whole-cartridge peak timing across the race and Showcase builds.
  The [shared Carrier frame roster](audits/2026-11-21-carrier-frame-roster.md)
  cuts mean Blood Bath guest cycles by 4.58% and the Outpost Run busy
  window by 3.42%. The Blood Bath p95 falls 3.69%, and the Outpost
  peak rises 0.30%. The [shared Custom Game defense pass](audits/2026-11-21-custom-defense-cells.md)
  saves another 1.05% mean in Blood Bath and 3.22–4.37% in the three
  Outpost Run windows. Pursue route stalls and the large shared draw
  passes against the current combined build.
  Qualify physical frame pacing on all three race cartridges.
  [Word stores for padded route borders](audits/2026-11-24-route-grid-borders.md)
  save about 1,700–3,400 guest cycles per authored point fill across
  64-, 96- and 128-cell widths. The FIFO still dominates dense fills.
  Measure how often the shared border setup runs in a current busy
  match before attributing any frame-rate gain.
  [Compacting the pending point queue](audits/2026-11-22-dense-release-attribution.md#compact-the-pending-point-queue)
  saves 3.61% in the archived late Z7 16-update window and moves
  whole-run means by less than 0.1% in the measured replays. Record
  a fresh late Z7 paired run and keep profiling the steady draw and
  order costs. The compaction trial at that revision averaged 10.92
  emulated updates per second in the P10 victory tail.
  The [unit projection and facing trial](audits/2026-11-22-dense-release-attribution.md#unit-projection-and-facing-on-the-compacted-release)
  lowers the later P10 window by 1.79% and the 9,489-update mean by
  0.39%.
  Friendly Marines and Firebats account for 61.6 of the 75.7 outer-band
  crops per update in the later window. [Generated animation bounds](audits/2026-11-22-dense-release-attribution.md#generated-bounds-for-offscreen-animation-checks)
  now reduce that window by another 4.02%. The exact crop stays active
  for visible or uncertified art. With the bounds active, the later
  orders pass alone still costs about 537,000 cycles. The [strict target
  retention comparison](audits/2026-11-12-target-index-bulk.md#strict-reuse-on-the-generated-bounds-release)
  preserves target order and the sampled dense matches. It fails the
  Z10 IWRAM guard and raises the dense Blood Bath mean by 1.22% when
  compared with matched ROM placement. Keep the current index.
  The [steady target and cloak-cache audit](audits/2026-11-22-steady-order-cost.md)
  finds empty cells at 93% of sampled indexed visits. Row bitmaps
  make queries cheaper and still raise mean Blood Bath cost after
  sparse clearing. Skipping redundant untracked Arbiter cache writes
  cuts the P10 victory replay's mean 0.39% with matching cache
  words, displays and saves. The replay averages 11.06 emulated
  updates per second. Across all three race cartridges and both maps,
  the [three-AI Custom Game opening measurements](audits/2026-11-22-steady-order-cost.md#three-ai-custom-game-opening-pace)
  average 29.76–29.86 emulated updates per second with 28–29 live
  units. All six miss the two-VBlank deadline at p95. The
  [later three-AI battle windows](audits/2026-11-22-steady-order-cost.md#three-ai-custom-game-battle-pace)
  drop to 13.24–18.47 emulated updates per second. In the sampled
  no-new-route-build Protoss Outpost cut, targeting alone costs 139,078
  cycles within orders and tracer passes cost 96,034 within rendering.
  The [occupied-row query loop](audits/2026-11-22-steady-order-cost.md#iterate-the-published-target-row-mask)
  reads each hostile owner's row mask once and stops after its last
  occupied row. Authored ARM batches save 0.7–7.8% across the
  speed- and size-optimized query builds. Measure the change during
  a full busy battle before attributing a frame-rate gain.
  The [row-local column trial](audits/2026-11-22-steady-order-cost.md#column-masks-per-occupied-row)
  slows compact crowds and every 32-query rebuild batch. Keep the
  shared masks and measure the real query count and actor spread
  before revisiting this design.
  [Clipped targeting bounds and a squared radius](audits/2026-11-24-target-query-radius.md)
  save 0.55–4.69% of query cycles across authored ARM layouts
  with the same chosen targets. Rebuild timings match. Measure
  the same query cost in a current dense three-CPU replay.
  [One visibility check per queued resource](audits/2026-11-24-resource-depth-admission.md)
  saves 552 guest cycles for five minerals and two geysers in an
  authored ARM batch with the production heap. Measure the same
  change with imported sprites and the full actor roster.
  [Authored minimap sampling and target ranking](audits/2026-11-24-authored-minimap-target.md)
  save 8.17–9.48% mean frame cycles in two 600-update ARM replays.
  The race engine uses its own minimap raster and indexed target
  queries. The exact KODAK Protoss 10 sample takes 152 indexed
  queries and zero fallback scans in four late three-CPU updates.
  Prioritize draw and movement costs measured on the race path.
  The [six race-cartridge phase cuts](audits/2026-11-24-kodak-package-checks.md#late-simulation-and-drawing-budget-across-the-race-roms)
  put the late simulation interval alone over the 30 fps budget on
  both maps. Orders consume 45–56% of total cycles. Profile movement
  and depot/route work across the three cartridges first. The eight
  instruction-attributed updates per case put 51–129k cycles in
  uncached contact masks. [Reusing the query origin and dimensions](https://github.com/ggu/starcraft-gba/pull/828)
  matches 450 observed masks and saves 5.96–6.06% in the paired
  IWRAM kernel fixture. The [full-frame pairs](audits/2026-11-24-kodak-package-checks.md#reusing-contact-scan-coordinates-in-complete-frames)
  are flat on canonical P10 and within 0.02% on the unpacked
  Terran and Showcase variants. Prioritize larger measured
  reductions. Terran Blood
  Bath also spends about 81k direct cycles in the monotone route
  probe. Pair those workloads separately and keep both maps in
  the actor and effect draw tests.
  The [Terran witness-table pairs](audits/2026-11-24-kodak-package-checks.md#monotone-route-witnesses-on-two-current-terran-saves)
  are flat in two-CPU Outpost and cut 1.56% of mean frame
  cycles in three-CPU Blood Bath. Both save continuations
  match. The [current 96-update Blood Bath pair](audits/2026-11-24-kodak-package-checks.md#witness-use-on-the-pylon-release)
  regresses by 0.44% with just 13 hits in 419 lookups. Leave the cache
  off in production. Any follow-up needs a live workload with sufficient
  retained paths and a cheaper miss path. Prioritize repeated draw and
  movement costs on both maps.
  The [exact crop and tracer census](audits/2026-11-24-kodak-package-checks.md#sprite-crop-and-projectile-draw-calls)
  finds only seven redundant crops across 48 updates. On Outpost the
  base projectile pass takes 39.6–64.7k cycles per update across the
  three race cartridges. No sampled tracer draw has a ground depth layer.
  Profile motion and raster work with the merged FOLLOW viewport
  cull in the fixed-camera cut and a visible holdout. Retain the
  current ground-trail picker until a measured workload exercises it.
  The [current sprite-entry census](audits/2026-11-24-kodak-package-checks.md#direct-sprite-entries-on-the-pylon-release)
  finds only one misaligned generic sprite draw in eight dense Z9 updates
  and none in eight panned P10 updates. Dense Z9 spends about 53.8k cycles
  per update in the packed path, mostly unit walk frames. Prioritize
  packed walk draws in Z9 and tracer preparation in P10. In the same
  P10 cut the camera sees zero visible tiles and all 264 sampled
  rasters exit through fog. [Skipping source tracer drawing in that viewport](audits/2026-11-25-fogged-tracer-viewport.md)
  brings panned P10 and Z10 mean costs down by 15.48% and 18.12%.
  The camera sweep into and out of fog matches both pages and tracer
  state on every update. Visible battles remain above budget. Profile
  tracer preparation and packed walk frames while units are in view.
  The [visible Showcase fight and P7 army](audits/2026-11-25-visible-drawing-and-campaigns.md)
  put generic sprite drawing at 18.1% of the Showcase sample and
  about 191k cycles per P7 update. The shared ARM quad and pair trial
  improves P7 by 1.78% and three Showcase scenes by 1.91–3.95%.
  Carry it through source and product checks. Profile the remaining
  body raster and selected-unit ellipse work. P7's Arbiter tick also
  takes about 182k self cycles per update. The first owner-mask cache
  increases total loop cost. Pursue cheaper optical list preparation
  and aura traversal with the existing refresh schedule.
  The [Outpost camera holdout](audits/2026-11-24-kodak-package-checks.md#pan-from-the-base-to-the-outpost-fight)
  moves the same battle into view. Zerg and Protoss now use 325–391k
  tracer cycles per update, roughly half of those inside the watched
  raster entry. The [tile-row trial](audits/2026-11-24-kodak-package-checks.md#reusing-effect-visibility-for-each-tile-row)
  improves the panned P10 loop mean by 8.16% with matching pixels
  and world, while the fixed mean is flat and its p95 rises 0.45%.
  [The shared renderer](https://github.com/ggu/starcraft-gba/pull/823)
  passes authored ARM and Mode 0 host differentials. The retained
  P10 [Blood Bath and save holdouts](audits/2026-11-24-kodak-package-checks.md#blood-bath-holdout-and-late-skirmish-journals)
  are flat on timing, match the selected scanout, and reproduce both
  maps' late saves and 180-update continuations. The retained pair also
  [wins Protoss 7 from a campaign save](audits/2026-11-24-kodak-package-checks.md#protoss-7-victory-from-a-saved-campaign)
  with matching sampled state and scanout and byte-identical victory
  journals. The full replay timing is flat. The [four canonical profiles](audits/2026-11-24-kodak-package-checks.md#canonical-renderer-qualification-on-all-four-profiles)
  qualify the merged renderer. Panned three-CPU Outpost
  gains are 8.45% on P10 and 7.23% on Z10. The fixed
  views are flat and the panned fights still exceed the
  two-VBlank budget.
  [The route-call census](audits/2026-11-24-kodak-package-checks.md#monotone-route-search-outcomes)
  finds that all 280 late Terran and Protoss calls succeed within
  the existing budget. Terran Blood Bath averages 88.8k probe cycles
  per update. The [paired ARM query replay](audits/2026-11-24-kodak-package-checks.md#paired-arm-replay-of-observed-route-queries)
  matches all 277 steps and five search counters. The [incremental
  search](audits/2026-10-29-monotone-route-step/README.md#incremental-coordinates-during-the-search)
  in [merged PR #802](https://github.com/ggu/starcraft-gba/pull/802) also
  matches 460,800 host comparisons and uses 24.62% fewer fixture
  cycles at `-O2` for the observed calls. Qualify the full race
  runtime and its memory on both maps.
  The [clear-prefix dispatcher experiment](audits/2026-11-24-monotone-clear-prefix.md)
  resumes the same search after the clear cells. It saves 3.38%
  across the captured late Terran route calls and reduces frame
  budget misses from 18 to 13 in the authored 280-update test.
  The [four late Terran and Protoss probe replays](audits/2026-11-24-monotone-clear-prefix.md#exact-package-route-call-coverage)
  match every route result and save 204–2,413 probe cycles per
  sampled update. Both Z10 far-read call sites are unused in the
  four matched Z10 cuts. Keep the option disabled until a linked
  release passes frame, save and memory checks. Prioritize recurring
  movement/contact and actor drawing for the larger frame-budget gap.
  The eight-entry exact
  endpoint cache simulation covers at most 0.20% of loop cycles in
  the expanded Terran Blood Bath window, before cache overhead.
  The opt-in [16-path continuation table](https://github.com/ggu/starcraft-gba/pull/805)
  saves 72.11% of route-wrapper cycles in the bounded ARM replay
  of those calls. Check full dispatch and frame timing before
  selecting a production cache size.
  The 99 new endpoints that continue an earlier path account for
  7.07% of loop cycles. Verify reuse with the existing route-slot
  precedence, aging, and grid invalidation before activation.
  The [direct grid corridor walk](audits/2026-11-22-steady-order-cost.md#walk-direct-corridors-by-grid-cell)
  uses the existing walk grid and visits the same cells in order.
  All 95 direct-path calls in the retained Outpost battle have
  in-bounds endpoints. The separate authored ARM cases show lower
  in-bounds query costs. The blocked movement scan is the larger
  measured cost and still needs a broader reduction.
  The [collision lookup census](audits/2026-11-22-steady-order-cost.md#collision-table-layout-and-lookup-count)
  finds one to six geometry lookups per scan, with no repeated tuple
  in 129 sampled scans. Speed-major packing saves ROM and improves
  only the larger authored groups. Prioritize the work leading up
  to each geometry lookup.
  [Prepared walk-grid steering](audits/2026-11-22-open-steering-directions.md#reuse-the-prepared-walk-grid)
  saves 6.0–10.7% in authored ARM selection batches with the
  observed Outpost mask mix. All 503 observed calls already have
  a valid grid, and 393,216 exact-source comparisons match.
  Measure the combined release frame cost during a busy battle.
  The [merged selection and detector query changes](audits/2026-11-22-steady-order-cost.md#selection-and-detector-query-candidate)
  saves 0.72% on the P10 victory replay and 0.17–0.25% across the six
  Custom Game replays with matching state and video. Profile repeated
  eligibility and tracer drawing here, then validate in a player-driven
  battle and on hardware. A strict target cell bound can exclude 44.8%
  of live node visits in the short battle cut, then costs 0.60% on
  the full P10 replay and exceeds the Z10 stack reserve. The animation
  index cache saves 0.29% in 16 skirmish updates and 0.02% in the full
  campaign run. Keep both trials local. In [visible Crossfire](audits/2026-11-22-steady-order-cost.md#visible-fights-in-protoss-crossfire),
  uncompressed unit sprite blits take 130,161–148,318 guest cycles per
  update. All 507 sampled calls draw attack poses. A private 85-frame
  packed bank saves 1.02–1.24% across two short Crossfire windows.
  The [generated sidecar](audits/2026-11-22-steady-order-cost.md#generated-attack-pose-sidecar)
  raises the pooled saving to 1.53% with stable frame metadata and
  about 292,000 added ROM bytes. Over 900 visible Crossfire updates,
  the saving is 0.89%. The full Custom Game and P10 samples are nearly
  flat, and Z1 saves 0.07%. All 48 ordinary Custom Game save and reload
  cases pass. Keep the option explicit and use sustained battle
  measurements to choose future actor banks. A
  [shared byte-sprite trial](audits/2026-11-22-steady-order-cost.md#aligned-reads-in-the-shared-byte-sprite-compositor)
  saves 1.53% over the sustained visible Crossfire run with matching
  pixels and state. The three Outpost matches use 0.33–0.42% more
  cycles and the P10 campaign tail is flat. Keep the trial out of the
  regular builds while reducing the unaligned and mixed-opacity
  cost. [Packed sprite row setup](audits/2026-11-24-packed-sprite-row-stride.md)
  saves 6.06% of packed draw-batch cycles at `-O2` when replaying
  the 164 sprite rectangles from eight late Protoss 10 updates
  with authored colors and masks. Both framebuffer pages and the
  palette match. Verify full-frame timing on the race cartridges.
  The [combined targeting and pose index](audits/2026-11-22-steady-order-cost.md#target-column-and-crowded-pose-indices)
  saves 1.19% over the same visible fight and 0.16% on the P10
  victory tail. Five long Custom Game pairs improve slightly and the
  sixth stays within 0.003%. All 48 ordinary save/reload cases pass.
  The [Mode 4 page census](audits/2026-11-22-steady-order-cost.md#mode-4-page-update-coverage)
  finds changes in 8–36 of 105 battlefield tiles in fixed-camera
  windows, with 55–81 tiles still carrying overlays over terrain.
  Prioritize exact reuse of finished regions where the camera and
  contributors are stable. Retain moving-camera cases, with 105
  changed tiles on each pan update in one P10 window. Continue
  profiling movement and route work toward the two-VBlank budget.
  The [shared small-diamond lookup](audits/2026-11-24-shared-diamond-spans.md)
  saves 21,635–42,273 draw cycles per update in an authored
  original-art crowd with matching pixels and world state. Two exact
  ratios cost more in the IWRAM microbench. Measure fallback shape
  frequency and combined pacing on the race cartridges.
  [Shared outlines, full-width fills and glyph drawing](audits/2026-11-24-shared-frame-borders.md)
  cut mean emulated frame cycles by 5.74% in a fixed-camera 76-unit
  authored skirmish and 0.92% with the battle in view and camera
  panning, after pairing against the packed-sprite release. The
  600-update visible pair matches state and both pages and reduces
  two-VBlank overruns from 115 to 101. The earlier visible battle
  changes about 14 of 105 tiles on average between uses of the same
  page at a fixed camera. The first exact band-command trial
  reuses 3,341 of 4,200 bands and raises whole-frame mean by 10.6%.
  Its forced fallback also disagrees on 13 frames. Prioritize
  cheaper contributor tracking and validate fallback before
  offering finished-pixel reuse. Replay the race cartridges and
  keep reducing the larger movement and painter costs.
  [Reusing detached sprite records](audits/2026-11-22-steady-order-cost.md#detached-attack-sprite-draw-records)
  saves 0.63% in Crossfire and slightly increases both measured
  Custom Game means. Keep it local. Count transparent row edges in
  an ordinary fight before trialing generated row bounds. In the
  [transparent word census](audits/2026-11-22-steady-order-cost.md#transparent-edge-and-word-run-trials),
  the edge probe finds 9.93% removable visible pixels across four
  retained P10 Melee updates. Draw-time scanning saves cycles on
  sparse fixtures and increases dense or clipped costs.
  [Precomputed row bounds](audits/2026-11-22-steady-order-cost.md#precomputed-row-bounds)
  save 18–33% on sparse authored ARM cases in IWRAM but cost
  20% on dense art and require another 608 bytes of IWRAM.
  Keep the trial local. [Whole-frame cropping](audits/2026-11-22-steady-order-cost.md#whole-frame-transparent-crop)
  saves 15–50% on authored padded ARM cases when offsets
  are read at runtime. Across 16 retained P10 Melee cuts,
  all 319 packed draws already use tight rectangles, and
  only one of 321 byte draws can shed a five-pixel margin.
  Keep whole-frame cropping local and prioritize larger
  costs in the dense workload.
  The [shared projectile heading lookup](audits/2026-11-22-steady-order-cost.md#shared-heading-for-projectiles-and-unit-facing)
  preserves launch and impact direction through exact ties. It
  cuts guest cycles slightly in six ordinary battles, the P10
  victory replay and Crossfire. The busy-frame target still needs
  larger movement and rendering gains.
  The [shared weapon profile index](audits/2026-11-22-steady-order-cost.md#source-weapon-profile-index)
  removes two Thumb division calls. It saves 0.87% across the full
  Crossfire sample and slightly improves all six three-CPU means.
  The [shared visibility lookup](audits/2026-11-22-steady-order-cost.md#visibility-coordinate-lookup)
  cuts mean guest cycles 0.56–0.87% in the six three-CPU battles,
  0.47% in Crossfire and 0.73% on the P10 victory tail. The measured
  P10 pace rises to 11.25 emulated updates per second. Larger movement
  and rendering costs still dominate the crowded scenes.
  [Effect raster tile bands](audits/2026-11-22-steady-order-cost.md#partially-fogged-effect-raster)
  cut partial-fog fixture costs. All 105 sampled Outpost draws reject
  on viewport bounds and all 498 Crossfire draws are fully visible.
  Keep the band trial local. Earlier
  [viewport bounds for captured FOLLOW effects](audits/2026-11-22-steady-order-cost.md#viewport-bounds-for-captured-follow-effects)
  catch all 105 sampled offscreen draws in a read-only replay.
  Authored mixed ARM batches preserve pixels and use 13.32–14.61%
  fewer cycles. The crossing-only batch costs slightly more.
  Measure sustained frame pacing on the race cartridges in visible
  and offscreen battles.
  The [ordinary Outpost census](audits/2026-11-22-steady-order-cost.md#outpost-resource-sprite-costs)
  attributes about 59,514 guest cycles per update to four mineral
  blits and 9,750 to one geyser blit. Reusing the loaded word for
  mixed pixels saves 0.47% in that cut and 0.21–0.53% across all
  six long three-CPU matches. The P10 victory replay averages 11.26
  emulated updates per second. Enabling aligned word loads in
  Showcase slows each of the three sampled Crossfire windows.
  The [shared opaque minimap copy](audits/2026-11-22-steady-order-cost.md#opaque-minimap-raster-copy)
  then saves 0.37–0.56% across the six three-CPU matches and 0.47%
  across 900 Showcase updates. The P10 victory replay rises to
  11.30 emulated updates per second with matching frames and SRAM.
  Use the [page census](audits/2026-11-22-steady-order-cost.md#mode-4-page-update-coverage)
  to bound a stationary-camera band reuse trial. Measure the cost
  of tracking changes before relying on it in crowded fights.
  A point-route
  predecessor skip has flat timing in the 3,600-update Blood Bath holdout
  and exceeds Z10's IWRAM reserve without ROM placement. A uniform-radius
  whole-bucket bound excludes
  3.5% of visited contact nodes in a five-update P10 sample. Broader
  movement and collision work remains the priority. Compare sustained
  cost, normal-input outcomes and fresh title continuation.
  The tighter [contact footprint trial](audits/2026-11-22-steady-order-cost.md#collision-bucket-footprints)
  finds more removable visits. Maintaining the footprint exceeds the
  P10 IWRAM guard, and moving its update helper to ROM raises late
  Outpost mean cycles by 4.30%. Focus on reducing candidate loads
  without adding work to each coordinate update.
  The [current Blood Bath spike](audits/2026-11-12-target-index-bulk.md#the-next-dense-skirmish-bottleneck)
  spends 1.09 million cycles on worker facility paths and 870,183
  on movement point fills. One destroyed building invalidates 32
  worker depot choices. The [depot expansion continuation](audits/2026-11-22-dense-depot-suffix.md)
  switches long 64×64 floods to byte-grid scratch while preserving
  the FIFO and its packed result. Busy Blood Bath's worst update is
  3.01% faster with matching gameplay, video and saves. Its average
  cost improves 0.12%. The longest depot query still consumes 529,000
  cycles. The [fresh distant transfer](audits/2026-11-22-dense-depot-suffix.md#fresh-distant-requests-on-the-pacing-release)
  reduces that call to 405,319 cycles and cuts the current match peak
  3.50% and p95 0.74% with matching directions and continuation.
  Every distinct depot-query origin in the stall had a positive
  witness before the six-cell opening. A destruction-only proof cuts
  the worst update a further 8.06%, while the mean and p95 rise 0.35%
  and 2.13%. Keep that trial out of the cartridges. Prioritize reuse of
  exact retained directions when an opening cannot affect the path.
  Measure the next update and full replay to capture cache-warming cost.
  Include closer newly reachable depots and save/load continuation.
  The [64×64 point FIFO](audits/2026-11-22-dense-depot-suffix.md#dense-point-queue-on-the-current-release)
  saves 3.29% on the current Blood Bath peak and 5.05% on the
  next update. All seven fields have different goals. Their
  requested origins are first discovered after 685–1,662 of
  2,795 pops. The earlier partial-field trials often shifted the
  cost to later reads. Measure how long fields remain usable
  before materialization and look for savings in the full fill.
  In [the five failed donor reads](audits/2026-11-22-dense-depot-suffix.md#opening-distance-and-retained-point-directions),
  four openings tie the retained route length. Even exact live-grid
  distances would certify only one read. Prioritize the full-fill cost
  and require canonical FIFO tie ordering for any donor reuse.
  The [shared two-word walkability table](audits/2026-11-22-dense-depot-suffix.md#dense-point-setup-and-output-trials)
  cuts the same Z10 peak by 0.61% and frees 16 IWRAM bytes in each
  regular build. The six long skirmishes show little change in mean cost.
  The [64×64 facility flood trial](audits/2026-11-21-facility-thumb-current.md)
  cuts the sampled Outpost peaks by 7–15% across the race ROMs on the
  preceding release. On top of the Carrier roster the Blood Bath peak
  falls another 5.09%, while p95 rises 0.32%. On the current combined
  source the isolated placement of the facility kernel exceeds the
  Z10 IWRAM reserve. The broader placement preview still needs new
  combined replay and runtime stack measurements. The nearby-root
  reserve itself already ships in the shared composed route defaults.
  The [shared changed-row sight path](audits/2026-11-22-shared-sparse-sight.md)
  cuts the Protoss 10 campaign tail by 7.87% and the tested Terran
  Outpost battle window by 0.48%. An isolated generated-header path
  preserves the product-specific building art. The same audit pins
  the nearby route call that shifts roughly 900,000 cycles to a
  later update in the gap-19 trial.
  The [shared persistent collision buckets](audits/2026-11-22-shared-persistent-collision.md)
  lower the busy Blood Bath mean by 0.90% and the Protoss 10 late
  replay cost by 0.78%. Terran Outpost improves 0.48% in mean and
  increases 0.86% at p95 in its busy window. Replay checks match
  collision-sensitive movement and placement, world state and both
  framebuffer pages. The [query-local Command validation cache](audits/2026-11-22-command-raw-validation-cache.md#after-the-moving-flyer-controls-fix)
  cuts the exact Protoss tail by 1.85% and its advancing gameplay
  p95 by 4.93%. The peak rises 0.12%. A batched selection-outline
  trial matches the same replay and saves 0.12% on the full tail.
  Count actual full-field completions on the corrected tapes,
  including updates with no new route build. At P10 call 39,774,
  FIFO expansion uses 663,684 of 863,785 full-fill cycles. The
  fixed-stride trial keeps the same state and route history but
  increases the hot P10 call at 38,952 by 23,930 cycles. It also
  adds 88–96 IWRAM bytes across P10 and Z10. Keep it optional.
  In 16 current P10 zero-new-build samples, campaign defense
  indexing and targeting use 84,015 cycles per update (5.32%).
  Four follow-up cuts show 86–88% of visited list nodes in cells
  with no hostile owners. The [campaign defense mask](audits/2026-11-22-campaign-defense-owner-masks.md)
  saves 0.80% across the exact current full P10 tail and 0.81%
  on the refreshed Z10 continuation. The sampled custom means
  are flat. The two late Z7 means rise 0.41% and 0.44%.
  Each tape retains its sampled state.
  The [persistent Command validation](audits/2026-11-22-persistent-command-validation.md)
  saves 0.71% on the exact P10 tail and 0.32% in the 1,200-update Z8
  continuation with matching state. Z7 and Z10 campaign wins and
  fresh HOME readbacks match. Measure physical timing separately.
  The [current target-index attribution](audits/2026-11-22-campaign-defense-owner-masks.md#remaining-order-cost)
  identifies target searches and cell insertion as recurring costs.
  The [armed type lookup](audits/2026-11-22-armed-type-targeting.md)
  cuts the ordinary Z10 continuation mean by 1.21% and the busy Blood
  Bath mean by 0.61%. The full Z7 replay keeps the same state and
  victory, with 1.22–1.60% lower means across the three sampled combat
  windows. The same audit records a static sprite layer trial with
  flat full-update timing. Target the larger route and order costs
  for the next pacing gain.
  The [wide facility census](audits/2026-11-22-wide-facility-consumer-census.md)
  finds 166 partial depot-field rebuilds when an existing field cannot
  answer a requested origin. The single-wide-frontier trial preserves
  the sampled P10 replay and saves 0.036% in mean game-update cycles.
  The [Z5 detector call split](audits/2026-11-22-cloak-query-cost.md)
  saves 0.16% in the matched Z10 Blood Bath sample with the same
  RAM endpoints. It passes active-cloak recovery, all 54 Custom Game
  save cases and the refreshed Z10 fresh-HOME victory at frame 63,523
  on the combined missile-trail release.
  The [local ARM route packer trial](audits/2026-11-22-z7-rendered-replay.md#route-output-packing-trial)
  cuts two P10 full-fill updates by about 63,000–73,000 cycles. The
  full-tail mean improves under 0.01% and Z10 has only 16 IWRAM bytes
  before its guard. Keep this trial local.
  The [current dense P10 attribution](audits/2026-11-22-dense-release-attribution.md)
  averages 1.573 million cycles. Orders use 514,000, and the depth
  painter uses 342,000, including 247,000 drawing units and their
  attached images. Reusing the painter's enemy visibility query saves
  0.018% on a 2,071-call holdout. Keep that trial local and prioritize
  larger movement, targeting or pixel composition costs.
  [Packed mixer saturation](audits/2026-11-22-native-custom-game-lobby.md#packed-saturation-in-the-shared-mixer)
  saves 0.23% in the late three-CPU Melee window and raises the
  earlier one by 0.10%. The [steering address census](audits/2026-11-22-native-custom-game-lobby.md#occupancy-checks-for-steering-rows)
  finds only nine empty spans in 104 visits. [Coalescing visible effect
  runs](audits/2026-11-22-native-custom-game-lobby.md#coalesced-visible-runs-in-the-effect-raster)
  saves about 62 cycles in the 1.35-million-cycle late sample.
  Keep these trials local. The [current sprite coverage census](audits/2026-11-22-native-custom-game-lobby.md#sprite-coverage-on-the-current-physical-pages)
  finds 60 of 61 whole draws eligible for reuse in the quiet
  window and 50 of 402 in the late one. Even at 8×16 granularity
  only 24.42% of observed late sprite rectangle area falls
  outside the changed regions. Trial a whole-page reuse path for
  quiet play and keep reducing orders and paint costs for busy
  fights. Include mapped effects, depth ties, camera movement and
  menu overlays in the reuse checks. [Forcing reuse on nine
  certified quiet draws](audits/2026-11-22-native-custom-game-lobby.md#measured-cost-of-skipping-a-certified-quiet-battlefield-draw)
  brings the mean from 742,158 to 594,804 guest cycles with
  matching pages and state. Even this idealized test needs
  33,012 more cycles saved per update for the two-VBlank
  budget. [Packed sprite pair transfers](audits/2026-11-22-native-custom-game-lobby.md#sequential-transfers-for-packed-sprite-pairs)
  cut 0.29% from the late loop and exceed the Z10 IWRAM guard.
  The [shorter cloak-cache scans](audits/2026-11-22-native-custom-game-lobby.md#shorter-cloak-cache-scans)
  save 0.57% on the late Protoss Melee mean and 0.51% on
  Showcase, with matching sampled unit and display state and
  unchanged static RAM endpoints. Protoss late p95 rises 0.41%
  and the full victory replay uses 0.27% more cycles. Keep this
  trial local.
  The [blocked primary-step census](audits/2026-11-22-native-custom-game-lobby.md#cycle-ceiling-for-fusing-blocked-direct-steps-with-steering)
  puts a 27,096-cycle inclusive ceiling on eliminating all failed
  primary checks in the sampled late Protoss Melee updates.
  Prioritize larger costs. The earlier [paired traversal trial](audits/2026-11-20-p7-recurring-orders.md)
  slowed busy Blood Bath.
  The [visible sprite source census](audits/2026-11-22-native-custom-game-lobby.md#visible-sprite-costs-by-source-in-late-melee)
  finds 119,084 inclusive cycles per update in byte draws,
  concentrated in attack poses and weapon effects. Use it to
  select compositor trials and measure busy rendering and orders
  together against the two-VBlank budget.
  The [timed per-page census](audits/2026-11-22-native-custom-game-lobby.md#whole-call-savings-available-to-retained-rendering)
  bounds whole-call reuse at 23,865 cycles per busy update
  before new tracking and restoration. Measure partial
  recomposition before implementing a persistent contributor
  cache. [Reusing indexed target coordinates](audits/2026-11-22-native-custom-game-lobby.md#reusing-indexed-target-coordinates)
  saves about 320 cycles on average and raises p95. Keep it local.
  The [audio observer budget trial](audits/2026-11-22-native-custom-game-lobby.md#skipping-over-budget-combat-sound-lookups)
  raises both sustained Melee means by about 0.2%. Keep the
  existing observer and target larger composition and order costs.
  The [early Z7 sample](audits/2026-11-22-dense-release-attribution.md#early-z7-movement-and-target-indexing)
  measures 39,280 cycles per target index rebuild and 23,701 within
  its cell-link operation. Preserve canonical chain order in any local
  builder trial. Count actual queued completions in the later Z7 battle.
  The [P10 point-route trace](audits/2026-11-22-dense-release-attribution.md#point-route-reuse-in-a-current-protoss-campaign-window)
  observes one fill and two requests at the same goal and origin.
  The [topology follow-up](audits/2026-11-22-dense-release-attribution.md#point-routing-after-p10-building-destruction)
  ties the 651,606-cycle refill to building destruction. Four old
  directions become stale, while the queried origin keeps its
  ordered predecessor. A longer probe saves 449,708 cycles on
  that update and raises the full P10 mean by 0.94%. Keep it local.
  Target first-fill cost without losing cheap repeat queries or
  preserving stale directions. The [small-component ARM sequence](audits/2026-11-21-z8-current-point-certificate.md#mixed-arm-small-component-fills)
  saves 35.49% on a synthetic warm-up, ten reused spans and one
  decline with exact field bytes. [Resuming a certified queue](audits/2026-11-21-z8-current-point-certificate.md#resuming-an-oversized-small-component-fill)
  saves another 5.56% against the small probe in a separate
  18-fill synthetic sequence with IWRAM and game wait states.
  Measure accepted requests, resumes and full restarts in the
  late Z7 workload before enabling either route option.
  The [paired mineral opening](audits/2026-11-22-steady-order-cost.md#paired-mineral-assignments-through-elapsed-second-18)
  shows four distinct GBA patch assignments from the first update
  while native workers still share a patch at second 12. A [fresh
  native replay](audits/2026-11-21-blood-bath-worker-opening.md#observed-retarget-during-approach)
  catches an approaching worker changing its target while another
  worker holds the old patch. The [shared contention prototype](audits/2026-11-21-blood-bath-worker-opening.md#shared-mineral-contention-prototype)
  preserves held timers and checks for reachable free alternatives.
  Both variants win the authored Transit Yard three-CPU tape,
  with different economy states and victory frames.
  Qualify duplicate opening targets, the native alternate ranking,
  ordinary saves and reloads, and busy game cost before enabling it.
  The [shared steering change](audits/2026-11-22-open-steering-directions.md)
  skips distance and terrain checks for actor-blocked directions. The
  P10 victory tail improves 0.93% in mean cycles and the matched Blood
  Bath window improves 0.28%. Measure longer movement-heavy Z7 windows
  on the corrected tape and keep exact route and world checkpoints.
  The [later Z7 instruction sample](audits/2026-11-22-dense-release-attribution.md#later-z7-orders-and-point-routing-on-the-steering-release)
  averages 1.606 million cycles, with 887,000 in orders. Point-prefix
  routing uses 150,000 including 95,000 in FIFO expansion. The uncached
  contact mask uses 132,000. Prioritize these measured costs over
  sprite-span caching, which increases P10 actor-draw cost and memory use.
  The [follow-up routing measurements](audits/2026-11-22-dense-release-attribution.md#full-fifo-completions)
  show that all six sampled point origins still need a full fill
  after 2,048 pops. Their first visits take 5,400–7,855 pops.
  The 8,192-pop cutoff alone reaches the shared scratch limit after
  4,412 pops. Compare in-place queue compaction on the exact tape and
  measure later reads and queue evictions.
  A [render-side live-entity roster](audits/2026-11-26-live-entity-census.md)
  costs 10–16k more guest cycles per late update than it saves; not merged.
  [Skipping the repeated can-attack test and gating campaign predicates](audits/2026-11-26-can-attack-chain.md)
  saves 21.8k (Terran), 8.5k (Zerg) and 16.9k (Protoss) guest cycles per
  Outpost update on the #882 relink, with bit-identical paired runs.

- **World depth and sprite lifetime.** The [shared collapse ordering fix](audits/2026-11-21-attached-death-continuity.md)
  keeps attached images in their actor's draw position, including after
  same-slot reuse and in the optional VRAM pool. The real N64 Blood Bath
  capture retains a dying SCV's sprite through the collapse and observes
  when it retires. Run the
  [depth replay assertions](audits/2026-11-21-world-depth-replay-contract.md#replay-cases)
  on both framebuffer pages. Capture an opaque equal-Y native death
  overlap and verify independent sprite IDs through reuse.
  The [fresh native tie sample](audits/2026-11-21-world-depth-replay-contract.md#native-pool-id-and-pixel-evidence)
  reproduces the ordering keys. Capture both sprite masks inside the
  active viewport before using this case to qualify a shared ID policy.
  [PR #696](https://github.com/ggu/starcraft-gba/pull/696) fixes an
  observed GBA foreground selection mismatch at two opaque same-layer
  flyers. The [cross-layer fixture](audits/2026-11-22-two-flyer-picker.md#opaque-flyers-on-different-layers)
  extends the painter order to a visible Wraith above an Overlord.
  Exercise more candidate bodies and an ordinary native viewport
  overlap.
  The [moving flyer double-tap case](audits/2026-11-22-two-flyer-picker.md#moving-same-type-flyers-in-the-double-tap-window)
  verifies the foreground single tap and same-type grouping when
  the overlap changes between A edges. The native same-Y Zergling
  row visits sprite 438 directly before 1418. Match its image
  frames and camera to the screenshot before asserting pixel order.
  Add the [shared depth parity cases](audits/2026-11-22-shared-depth-parity.md#replay-cases)
  to the harness. Record ground-pair and grouped-flyer picks, fallback Tank
  muzzle and impact, and the native Lurker's same-sprite burrow transition.
  Add the [mixed overlap cases](audits/2026-11-22-mixed-world-depth-overlap.md#harness-cases)
  to ordinary-input replays. The prepared fixtures cover the opaque
  three-body selection and two low trails ordered around a Bunker by
  birth Y. Capture ordinary detected-burrow firing and the matching
  native overlap. The [missile trail lifetime cases](audits/2026-11-22-detached-missile-trail-lifetime.md#replay-harness)
  cover existing trails remaining at their birth depth through impact.
  The native Goliath replay confirms independent image lifetime. Add
  active-camera native overlap and ordinary-input coverage in each mode.
  The [native retirement follow-up](audits/2026-11-22-shared-depth-parity.md#native-sprite-retirement-follow-up)
  identifies both free-list globals and verifies why the observed
  retired sprite stays free throughout that capture. Capture an
  actual reallocation and opaque overlap before changing tie semantics.
  The [mineral work regression](audits/2026-11-21-world-depth-replay-contract.md#mineral-work-layer)
  checks the separate layer-5 cue. Add native composed overlap cuts and
  ordinary pause/reload coverage during mining to the replay harness.
  The [fallback projectile fix](audits/2026-11-22-fallback-projectile-world-depth.md#harness-cases)
  puts authored moving shots in the shared queue using the native launch
  layer rule. Add the diagonal flight and impact cases to the harness.
  Prioritize a synchronized native opaque overlap with both pool IDs
  before replacing the GBA's exact-tie policy.

- **Zerg 7 current-controller replay.** The [rendered authoring audit](audits/2026-11-22-z7-rendered-replay.md)
  isolates the selection change at two stacked Drones. Render between
  HOST inputs when authoring a physical tape. The [route observer fix](audits/2026-11-22-z7-route-observer-purity.md)
  restores missing prefix state during controller probes. The corrected
  tape wins on the physical-button #711 ARM run at frame 267,585 with
  all 206 named observations matching. Two earlier pause and fresh HOME
  tests reach frame 9,438 after one Hatchery reselection. The corrected
  tape now also pauses at frame 30,739 and resumes from fresh HOME
  through frame 38,149 on the steering build. The complete uninterrupted
  replay on that version wins at frame 267,585, matching all 206 checks.
  Mean cycles fall 1.85% across the detector and steering releases.
  Fresh HOME from frame 30,739 also wins at frame 267,585 with all 124
  continuation checks matching. A third fresh HOME process reads
  the [victory journal](audits/2026-11-22-z7-route-observer-purity.md#full-normal-victory-on-the-steering-release)
  and re-exports identical SRAM.
  [November 24 intercepts](audits/2026-11-24-kodak-package-checks.md#intercept-the-northern-and-eastern-approaches)
  preserve the Hatchery at 730 HP through fresh HOME frame
  12,492. [East, west and north Sunkens](audits/2026-11-24-kodak-package-checks.md#sunken-coverage-on-three-approaches)
  fresh-load with the Hatchery at full health. Three
  static defenses still lose the base. Split mobile defenders
  between the approaches and sustain gas production.
  The refreshed [Z10 late-chapter tape](audits/2026-11-22-cloak-query-cost.md#combined-with-the-missile-trail-release)
  restores the frame-2,526 pause and wins at frame 63,523 on both
  combined builds. All 25 checked world fingerprints, indexed pages
  and the victory SRAM match.

- **Fresh N64 gameplay verification.** The [headless Challenger replay](audits/2026-11-21-native-headless-replay.md)
  reaches the Protoss mineral line against Zerg from reset. Four of five
  frame captures match the retained ordinary-input reference. Allow for
  asynchronous frame callbacks. The
  [fresh Wasteland comparison](audits/2026-11-21-wasteland-opening-cross-platform.md)
  matches 13 actors by type, owner, HP and scaled coordinates, and
  verifies 10/18 supply and the five-Marine R selection. The
  [ordinary Move case](audits/2026-11-21-wasteland-opening-cross-platform.md#ordinary-group-movement)
  confirms that both platforms move the selected group. The native
  scenario confirms ten Marines, a completed Barracks and Raynor.
  The GBA checks now require a completed Barracks and count living
  owned buildings for the zero-actor defeat rule. The normal Terran
  mission wins after an ordinary save and fresh HOME continuation.
  The [native worker-loss case](audits/2026-11-21-wasteland-opening-cross-platform.md#ordinary-worker-loss-boundary)
  keeps the mission active after all five SCVs die. Verify Raynor loss
  and the zero-actor boundary through ordinary inputs. The
  [native base approach scout](audits/2026-11-21-wasteland-opening-cross-platform.md#native-base-approach)
  reaches the base with five living Marines after Raynor's introduction.
  The resumed SCV reaches the mineral line alive and earns minerals
  with B Harvest. The same native Barracks is observed at foundation
  and completion. The ordinary native run won with ten Marines, the completed
  Barracks and Raynor alive, matching the GBA objective. The independent clocks
  leave the native mineral grant cadence open. Compare the score overview and
  isolate the Barracks payment. A separate
  [Blood Bath observer](audits/2026-11-21-native-headless-replay.md#blood-bath-state-and-render-row-observation)
  verifies the Command Center, worker and mineral sprite keys in
  live render rows. A [later Blood Bath observation](audits/2026-11-21-native-headless-replay.md#later-blood-bath-worker-orders)
  records a neutral SCV alternating between a mineral and its Command
  Center over 1,645 native updates. The [closer native observations](audits/2026-11-21-native-headless-replay.md#mineral-pickup-and-claim-release)
  bracket an eight-mineral pickup, matching patch debit and claim
  release in five updates. Resolve the bank credit and per-update
  service transitions to compare later mineral income.
  Extend campaign opening
  and event timing coverage. The [native Custom Game lobby captures](audits/2026-11-22-native-custom-game-lobby.md)
  show the map and seat controls, race cycling, and no difficulty field
  in the sampled Challenger and Blood Bath settings.
  The [matched Blood Bath Melee opening](audits/2026-11-22-steady-order-cost.md#matched-blood-bath-credit-and-elapsed-time)
  observes the native opening credit beside elapsed times of 9 and
  11 seconds. The GBA gives the same eight minerals at 2.470 guest
  seconds, with both hall anchors matching at half scale. At the
  sampled 19-to-28-second interval the native bank gains 24 minerals
  and the GBA gains 72–80 within corresponding clock brackets.
  The [shared Terran row trial](audits/2026-11-21-blood-bath-worker-opening.md#shared-terran-opening-trial)
  moves the southwest first credit to 3.303 guest seconds using the same
  four mineral targets and 45-update wait. All 16 SCVs deliver on HOST and
  ARM restores both old and new openings. The rendered controller wins
  on the regular ARM build at frame 42,658 with all 221 observations
  matched. All 1,134 named and closed-seat launches, 30 campaign openings
  and 24 regular fresh HOME continuations match the earlier trial. The matched
  N64 workers take turns on one southwest patch. Qualify patch contention
  and depot contact separately from the starting row.
  Use the active Blood Bath combat witness to qualify opaque
  overlaps and pool-ID reuse in new ordinary-input cuts.
  The [native Blood Bath production witness](audits/2026-11-21-native-blood-bath-lurker.md)
  follows three Zerg CPU Hydralisks through Lurker Egg and Lurker in
  their original slots. Qualify the missing Custom Game research
  and AI morph path with per-owner payment, supply and save checks.
  The [follow-on Blood Bath cut](audits/2026-11-21-native-blood-bath-lurker.md#follow-on-egg-observation)
  narrows a likely Zergling pair hatch to 33 native updates. The ordinary
  GBA counterpart verifies a queued pair and fresh HOME continuation.
  Prioritize a matched-clock, paid same-Hatchery multi-Larva trial: a
  [native witness](audits/2026-11-21-native-blood-bath-lurker.md#concurrent-hydralisk-eggs-at-two-structures)
  sees two overlapping Hydralisk Eggs near separate structures, each
  raw-Egg-to-Hydralisk in 438 guest-counter increments. It does not settle
  payment, or conversion to GBA updates. A [later witness](audits/2026-11-21-native-blood-bath-lurker.md#concurrent-eggs-referencing-one-hatchery)
  sees two overlapping Eggs whose actor records both point to one Hatchery;
  verify that field’s meaning and capture the issuing commands and payment.
  [Measured emulator clocks](audits/2026-11-21-native-blood-bath-lurker.md#observed-clock-intervals-on-n64-and-gba)
  show 26.78–27.47 s to raw Hydralisk plus 0.88–0.93 s to native completion,
  versus 17.59 s on GBA in a different Outpost scene. Reproduce with matched
  workloads and clocks before attributing or changing production duration.
  The [normal CPU Drone comparison](audits/2026-11-21-native-blood-bath-lurker.md#cpu-drone-production-and-fresh-home-continuation)
  follows a native Larva, Egg and Drone in the same slot, and a
  current GBA Hatchery debit, queue and new-slot Drone through
  fresh HOME. The experimental Outpost replay restores a live
  Carrier and later an in-flight Interceptor alongside a live
  CPU Egg, with byte-identical fresh exports.
  [Draft #689](https://github.com/ggu/starcraft-gba/pull/689) compiles the
  experimental Carrier/Egg authentication path for Terran and Z10.
  An isolated Z10 HOST fixture reaches the real load preparation and
  restores a paid Egg, a paid Carrier queue and an active Interceptor
  from one 14,100-byte record. It also rejects malformed newer
  candidates and restores legacy Carrier and transport records.
  The isolated ARM fixture also saves an ordinary paid Egg through
  pause, reloads through HOME, and matches 401 updates through its
  hatch. The [Outpost live-flight case](audits/2026-11-21-native-blood-bath-lurker.md#cpu-drone-production-and-fresh-home-continuation)
  repeats on the opt-in normal-build Z10 product candidate. Its bounded
  payload, real predecessor saves and 18-case Custom save matrix pass.
  All 378 candidate launch combinations pass. The initial ARM canary
  sample leaves 6,528 bytes untouched. Qualify sustained battles,
  pacing and uninterrupted versus fresh trajectories on that product.
  The refreshed Z10 trial passes ordinary matched flight-and-Egg
  continuation and migration of predecessor Carrier and three-CPU
  saves. PR #411 reserves 0x4f. The private 0x50 candidate needs a
  coordinated registry allocation and broader save-capacity checks.
  P10 research composition remains open. Use
  the [Lurker implementation dependencies](audits/2026-11-21-native-blood-bath-lurker.md#custom-game-implementation-dependencies)
  for the separate Den research and same-slot morph work.

- **Campaign verification.** All 30 missions now have ordinary HOME
  wins, fresh pause continuations and result readbacks on the same
  [attached-depth cartridge set](audits/2026-11-21-late-campaign-home-sweep.md).
  At Z7 call 154,198 the attached-depth and immediately preceding
  parked-depot releases match both pages, camera and palette. The earlier
  raster difference against `d664d71c` remains unexplained. Native mission
  behavior and physical timing remain separate qualification work.
  The shared double-tap fix restores the original Z7 route at the
  overlapping Mutalisk selection around call 16,376. The combined
  Wasteland and control build wins at frame 228,128 and reads back
  the same victory journal from HOME. All 254 world, ten display
  and six route cache cuts match the earlier corrected build.
  The [refreshed Z10 route](audits/2026-11-22-campaign-defense-owner-masks.md#refreshed-normal-zerg-10-route)
  wins at frame 51,588 from ordinary HOME and from a fresh pause
  save, with 21 paired continuation checkpoints and identical
  third-process victory readbacks. Preserve the older unaligned
  tape as historical evidence.
  The [facility P7 proof](audits/2026-11-13-p7-carried-normal-home.md#november-22-facility-release)
  recovers the ordinary opening through authored dialogue and wins
  after a fresh HOME load. The exact controller target reproduces
  all 4,929 physical input transitions. Extend that proof to other
  campaign saves and the next combined runtime.
  [Carried P8](audits/2026-11-22-steady-order-cost.md#carried-p8-replay-refresh)
  now wins at frame 104,113 on the current Protoss cartridge and
  matches all 40 named checks, including 27 after fresh HOME.
  The host renders before the next input so the overlapping-unit
  selection uses the painted camera. Add a separate fresh victory
  journal readback and extend the check to the next combined runtime.
  The [rendered Protoss controller matrix](audits/2026-11-13-p3-p4-home-preparation.md#rendered-protoss-controllers-on-the-visibility-release)
  reproduces all ten victories in independent HOST replay processes
  on the visibility release. The P8 and P9 fresh-process paths match
  all 327 continuation checkpoints. The [ordinary P3 ARM replay](audits/2026-11-13-p3-p4-home-preparation.md#current-ordinary-p3-victory)
  also wins at frame 47,661 on the regular Protoss cartridge after
  restoring a frame-20,000 pause save in a fresh process. The victory
  SRAM reloads into the result screen and re-exports byte-for-byte.
  Reuse the unchanged P7 and P8 authored HOME tapes.

- **Current three-cartridge playtest.** The
  [cross-layer and Command composition](audits/2026-11-22-two-flyer-picker.md#opaque-flyers-on-different-layers)
  passes all 1,134 ordinary Custom Game launches on the exact
  Terran, Z10 and P10 cartridges. It covers both maps, every human
  race and all arrangements of one to three named CPU rivals.
  The separate 54-case matrix passes fresh HOME save continuation.
  [The armed type release](audits/2026-11-22-armed-type-targeting.md#local-replays)
  repeats the 1,134 launches and all 54 save cases on the current
  race cartridge set.
  The [current campaign and skirmish replays](audits/2026-11-22-current-campaign-and-skirmish-replays.md)
  verify T4, T7 and T10 fresh-pause victories and result readbacks,
  P5 Shuttle continuation and result readback, and the normal Terran Blood Bath win.
  Current Melee previews reach the same three-CPU defeat before and
  after fresh HOME on all four profiles. The [active Terran Melee replay](audits/2026-11-22-blood-bath-melee-victory.md)
  verifies seeded deployment, an ordinary-input victory and a fresh
  HOME result. Native CPU alliances persist through the sampled
  three-CPU opening. The dual-flag composition passes ordinary human
  defeat with two neutral survivors, a staged enemy transfer and
  an ordinary Terran victory one update after retiring the last CPU.
  The [later native replay](audits/2026-11-18-blood-bath-elimination-arm.md#native-melee-defeat-and-neutral-workers)
  retains the CPU alliances and transfers four surviving SCVs at
  human defeat. The current preview also carries the compact Zerg
  transport save fix and the unstarted Interceptor refund.
  Keep launched and training Interceptor groups separate until
  their retirement and journal paths are qualified.
  Verify ordinary enemy survivor transfer and broader active wins
  with other races and two or three CPUs.
  SD readback, physical frame pacing and broader native campaign
  verification remain open.

- **Native Gantrithor capacity and rescue.** The [owned-ROM audit](audits/2026-11-21-native-gantrithor-capacity.md)
  verifies an eight-Interceptor maximum for raw 82. Separate that limit
  from the current four-child rescue stock, which needs an ordinary N64
  before/after capture. [PR #667](https://github.com/ggu/starcraft-gba/pull/667)
  adds the generic capacity policy to the shared engine with the existing
  save row and four-slot production default. The synthetic eight-slot
  policy passes HOST checks for the queue boundary, rank 7, full actor pool
  and HUD. Qualify a larger policy on the normal ARM save and load path.
  Capture an ordinary native raw-72 launch through return to qualify the
  source-backed layer transitions on live sprites.

- **Dense Z7 orders.** The [late stage profile](audits/2026-11-21-z7-late-order-budget.md)
  puts 65.7% of 2.661 million mean cycles in orders. Even zero-build
  updates spend 1.296 million cycles in orders. The instruction replay
  separates the repeated route failure of 49 Hydralisks from recurring
  target permissions. In the current paired Z7 wins the prefix trial
  saves 1.53% across the full resume and raises early peak cost by 10.61%.
  Keep it opt-in and address the nearby-root completions.
  The [live eligibility fast path](audits/2026-11-21-z7-late-order-budget.md#live-target-eligibility-fast-path)
  cuts mean guest cycles by 5.38% and 6.17% in the two late windows on
  the shared-depth release. It preserves the sampled route and gameplay
  results. [Sparse packing for short isolated fields](audits/2026-11-21-z7-sparse-route-packing.md)
  cuts the two late window means by 2.55% and 2.70% on the current
  renderer and Carrier build, with all four normal builds qualified.
  Profile the remaining zero-build orders and measure wider custom maps
  and busy P10 play on hardware.
  [Immutable actor lookups](audits/2026-11-21-z10-actor-lookups.md)
  cut the full resumed Z7 mean by 1.17% and the two late windows by
  1.52% and 1.57%, with matching images, SRAM and campaign result.
  Combat facts add about 2% in those late windows and 0.085% across the
  full tape in that initial trial. Prioritize the repeated
  route failures and expensive permissions in the busiest updates.
  The current sparse-route capture averages 2.09M and 2.05M cycles for
  zero-build updates in the two late windows. Orders account for 1.15M
  and 1.10M of those means. Re-profile the five retained zero-build cuts
  and compare an exact indexed query that carries invariant ground/air
  permissions and the candidate's existing coordinates into live policy
  checks. Keep the scalar permission path as an independent oracle and
  preserve cloak, fog, containment, invincibility and target tie order.
  [Spatial index retention](https://github.com/ggu/starcraft-gba/pull/590)
  raises the dense p95. The [query-local trial](audits/2026-11-21-indexed-target-query.md#dense-z7-ordinary-replay)
  cuts the two late means by 2.69% and 3.22% with exact timed route
  state and video. Its early Z7 window rises 1.14% and its Z8 mean
  rises 0.71%. Position reuse alone stays within 0.05% of stock across
  the Z7 and Z8 windows. Lazy weapon checks cut the Z8 increase to 0.18%.
  The [current lazy cache](audits/2026-11-21-indexed-target-query.md#lazy-query-on-the-fallback-depth-release)
  costs 0.15% in Z8 and 0.21% in early Z7, then saves about 0.8%
  in late Z7. Keep it off and measure route stalls and large draw passes.
  [Immutable combat facts](audits/2026-11-21-indexed-target-query.md#immutable-combat-facts)
  cut the Z10 late means by 2.76% and 3.13%, with matching state.
  The [normal Z10 qualification](audits/2026-11-21-z10-combat-facts-release.md)
  also verifies the rollback link and both maps with one to three AI
  seats, three human races and matching fresh continuation. The normal
  Z7 pair matches all 254 fresh-load replay checks and the victory
  journal at frame 228,128. A P10 V2
  candidate matches combat and victory journals but saves only 0.074%
  in the campaign sample. Prioritize larger shared order and route costs.
  The separate [point-certificate trial](audits/2026-11-21-z8-current-point-certificate.md)
  matches 668 live ARM route answers and improves the sampled Z8 mean
  and peak. The retry removes an 856,694-cycle stall at Z8 call 33,900.
  [Sparse cold-prefix packing](audits/2026-11-21-z8-current-point-certificate.md#sparse-packing-after-a-cold-prefix-exhausts-its-component)
  cuts call 33,852 by 120,735 cycles relative to that trial. The sampled
  mean and p95 are now below packed stock. Test bounded demand from the
  nearby-root reader at call 26,357 saves 731,494 cycles in an opt-in
  trial. Its retained extra frontier saturates the four ordinary rows
  and adds about one million cycles at call 33,890. The [reserved-row trial](audits/2026-11-21-z8-current-point-certificate.md#reserved-nearby-root-frontier)
  restores both cuts and takes 8.21% fewer cycles on average than packed
  stock across 203 sampled Z8 updates. It needs 264 extra EWRAM bytes.
  The [current release trial](audits/2026-11-21-z8-current-point-certificate.md#reserved-frontier-on-the-combat-facts-release)
  confirms the sampled Z8 gain and passes the four ROM links, a P10
  victory replay and late three-CPU Blood Bath save continuation.
  The 64×64 Custom Game checks exercise fallback. The whole resumed
  Z8 interval saves about 0.39%. In the Z7 cut at calls 47,758–48,237, mean
  and p95 initially fall 12.06% and 18.05%. The worst call rises
  11.23% when two nearby reads complete after their ordinary cutoff.
  [Transfer into the free reserve](audits/2026-11-21-z8-current-point-certificate.md#transfer-after-the-ordinary-cutoff)
  removes that peak increase. The paired mean, p95 and peak now fall
  12.42%, 18.47% and 13.70%. Profile the certificate-only read at
  call 48,149, then check both late Z7 windows before changing normal
  route flags. Measure physical performance separately.
  On the pinned combat-facts Z10, the
  [cold component guard](https://github.com/ggu/starcraft-gba/blob/codex/sc64-route-cold-component-bypass-20261121/docs/audits/2026-11-21-z8-current-point-certificate.md#skip-the-cold-prefix-on-proven-disconnection)
  saves about 120,000 cycles at the late peak and matches the full Z7 victory
  journal. The
  [blocked-goal warm retry](https://github.com/ggu/starcraft-gba/blob/codex/sc64-point-warm-blocked-gap-20261121/docs/audits/2026-11-21-z8-current-point-certificate.md#one-wider-retry-for-blocked-goals)
  saves 709,645 more cycles at call 154,508. Its Z7 win and re-exported
  journal match stock. The traced goal stays partial on each resident cut
  and evicts without an eager refill. Both late means remain over four
  times the 30 fps cycle budget. The
  [composed routes](audits/2026-11-21-route-composition-current.md#normal-cartridge-activation)
  are enabled on the standard packed builds. The current Z7 pair wins
  through ordinary inputs with matching world, display and re-exported
  journals. Mean guest cycles fall 12.84% in the early dense window and
  about 4% in both late windows. All four links, sampled stack canaries,
  T10 and P10 victories and 54 Custom Game continuations pass. Earlier
  VBlank observations explain sampled display phase shifts. Dense
  campaign loads and busy skirmishes still exceed the 30 fps budget.
  Prioritize sustained order work and physical frame pacing checks.

- **Busy Blood Bath.** The [minimap and field hint changes](audits/2026-11-21-minimap-and-field-hud.md)
  reduce Z10 mean guest cycles from 876,399 to 842,472, against a
  561,792-cycle 30 fps budget. Match the native minimap markers and
  fog against synchronized samples. In an
  [earlier three-CPU profile](audits/2026-11-21-blood-bath-current-profile.md),
  the worst sampled update spends 1.40 million cycles in
  depot reachability and about 870,000 in four full point floods.
  An aligned point-transfer trial raises p95 by 0.48% with exact
  state and route results. The [parked depot flood comparison](audits/2026-11-21-parked-depot-flood-current.md)
  counts 105 partial positive builds and no complete negatives in
  56 sampled updates. Fusing discovery cuts the sampled build cycles
  by 22.24% and the Blood Bath update peak by 7.58%, with matching
  fields and continuations. Its Outpost window peak falls by 15.81%.
  The [terrain restore measurement](audits/2026-11-21-terrain-restore-interior-traffic.md)
  estimates about 11,145 cycles of mean headroom for contiguous row
  transfers in its fixed-camera sample, before bookkeeping. The
  [Mode 4 padding change](audits/2026-11-22-terrain-cache-padding.md)
  frees 2,400 bytes of EWRAM per profile and lowers the current busy
  Z10 replay mean from 842,472 to 840,021 cycles. Measure per-page
  damage tracking before adding writes to the painter's hot path.
  The [packed sprite compositor](audits/2026-11-22-packed-sprite-transfer.md)
  lowers that mean to 832,982 cycles and frees 64 bytes of static
  IWRAM code on each normal profile. The separate byte sprite trial
  saves another 2,143 cycles on Blood Bath. Prioritize projectile
  drawing and the exact collision lifecycle for larger sustained
  savings. The
  [point-route origin census](audits/2026-11-21-blood-bath-point-origin.md)
  links all four fills at update 2,055 to stale-donor gate
  failures. Both local certificates decline all four. A full-width
  FIFO can reach the requesting origins after 685–1,662 of the
  2,795 pops per fill. The [resumable and placement trials](audits/2026-11-21-dense64-prefix-and-placement.md)
  find that many parked fields soon materialize. Peak improves
  with Thumb facility-loop placement, about 4.85% on Blood Bath and
  15.28% in the sampled Outpost window. Mean changes are small and
  both p95 measurements rise. Keep production placement and focus
  the next sustained-load investigation on order work, path-grid
  rebuilds and the steady painter cost. With composed routes active,
  the current Thumb facility trial cuts peak Blood Bath cost 5.11%
  and raises p95 0.60%. It leaves 24 bytes of static IWRAM code margin
  in Showcase. Keep the placement option experimental.

- **Native worker openings.** The [matched Blood Bath replay](audits/2026-11-21-blood-bath-worker-opening.md)
  confirms that N64 SCVs start at the hall, briefly share a mineral
  target, and reassign before mining. The first deposit arrives between
  native updates 172 and 188. The opt-in formation moves the stock
  first deposit from GBA update 75 to 141. Both normal Terran SRAM
  variants resume through the same deposits. The native nominal bracket
  is 215–235 GBA updates. Nearest footprint perimeter projection
  reproduces two of four native arrival points. Resolve tangential
  approach selection and temporary contention before activating it.
  Combining formation and wait moves the first GBA credit to 198
  updates. The [longer native comparison](audits/2026-11-21-blood-bath-income-windows.md)
  measures 168 minerals in the later window, compared with 312 stock
  and 200 combined. The combined trial reloads during mining and
  matches the next 240 updates. Prioritize patch occupancy and depot
  approach before wider balance changes.

- **Normal race cartridge win and save qualification.** All ten Protoss
  missions win after fresh pause loads on the
  [normal P10 build at `f69b2bbd`](audits/2026-11-13-normal-campaign-matrix.md#normal-protoss-cartridge-at-f69b2bbd).
  Each victory journal restores and re-exports identically. The
  [release 25e9e8b9 matrix](audits/2026-11-13-normal-campaign-matrix.md#release-25e9e8b9-race-replays)
  ties Z2–Z6 to one Z10 ROM and P5 to one P10 ROM. Z2 and P5 match paused
  saves and fresh continuations. Z3–Z6 match all 627 uninterrupted
  checkpoints. All ten Zerg missions now win after fresh loads on one
  [Storm-depth Z10 cartridge](audits/2026-11-13-normal-campaign-matrix.md#z1-and-z3z6-on-the-storm-depth-release).
  Z6 restores two queued impacts, and Z7 resumes after reselecting
  its Hatchery. Z8's three known battlefield pixel differences remain
  in the replay gate. The
  [current normal skirmish matrix](audits/2026-11-21-current-skirmish-matrix.md)
  covers all 2,232 ordinary starts across both maps. Each of the three
  rows supports a named race, Random or Closed with one to three CPUs.
  All 108 sampled fresh SRAM continuations match. The [shared Carrier
  qualification](audits/2026-11-21-carrier-empty-training-pass.md#shared-cartridge-qualification)
  ties current Terran and Protoss finales to stock pause and victory
  journals and current Zerg finale victory to the same pause journal.
  Complete long custom wins and check hardware timing.
- **Keep campaign tapes current.** Current P1–P3 win after
  [fresh saves and updated ordinary endings](audits/2026-11-13-normal-campaign-matrix.md#early-protoss-current-endings).
  Preserve the new ending inputs and update the stale checkpoint tails.
  The [new Z7 tape](audits/2026-11-13-normal-campaign-matrix.md#current-z7-home-win)
  wins from HOME at frame 228,128 with all 271 checkpoints matched.
  The [current Z10 fresh-load proof](audits/2026-11-13-normal-campaign-matrix.md#z7-fresh-save-through-victory-on-the-storm-depth-release)
  matches all 254 remaining checks after one ordinary A press restores
  the Hatchery selection. It averages 12.66 main-loop updates per
  emulated second. Keep this route in the speed and save regression gate.
- **Measure the remaining slow updates.** The refined eager route queue
  lowers the current Z8 peak from 12.10 to 11.67 million guest cycles with
  matching route fields and framebuffer samples. Two P10 route spikes
  improve 0.86% and 2.45%. Keep the queue opt-in while checking the
  remaining RAM guards and sustained P10 simulation and draw costs.
  Inlined Terran Building reads now ship on normal Terran and cut the
  tested T6 save and load stalls by 9.76–11.19%. All ten normal mission
  pairs match their fresh loads, victories and 32 KiB journals, and
  the composed link reproduces the qualified bytes. Measure hardware
  against the 30 fps goal.
  [Route queue](https://github.com/ggu/starcraft-gba/pull/636)
  [Building decoder](https://github.com/ggu/starcraft-gba/pull/638)
- **P10 battle draw cost.** The
  [packed Overmind frame](audits/2026-11-21-p10-packed-overmind.md)
  ships in ordinary P10 and cuts the sampled late window by 1.53%,
  with both indexed pages, palette and world matching through victory.
  The [packed Nexus frame](audits/2026-11-21-p10-packed-nexus.md#shared-world-depth-release)
  cuts the late mean by 7.43% in a normal three-AI Blood Bath match on
  the shared world-depth release with matching world and indexed pages.
  Continue sharing the slow simulation and route windows in the
  release gates.
  The [empty weapon overlay probe](audits/2026-11-21-empty-weapon-overlays.md)
  changes the current three-AI replay mean by −0.026%. Keep the default
  renderer and use the exact frame and route receipts for larger trials.
- **Interceptor pulse depth.** Normal P10
  [restores a live shot](audits/2026-11-19-world-depth.md#interceptor-pulse-restore)
  and matches 36 updates through damage, retirement and slot reuse.
  A staged ten-pulse ARM workload raises mean cycles by 0.28%.
  Normal Z10 also [restores live and docked saves](CARRIER-PROJECTILES.md#normal-z10-cartridge).
  Add the native weapon-79 framebuffer cut.
- **Match worker economy against the N64.** The
  [paired northwest and southwest Blood Bath run](audits/2026-11-19-n64-skirmish-ai.md#matched-northwest-and-southwest-worker-comparison)
  uses the same human and CPU hall coordinates on both machines. The GBA
  wallet gains 472–544 minerals over the bracketed opening where the
  native HUD gains 268. The opt-in 102-update mining wait reduces
  that GBA gain to 320–352 and passes a fresh active-wait save.
  [Native patch queues](audits/2026-11-21-native-mineral-service-queue.md)
  show one worker in service and FIFO handoffs in an ordinary four-seat
  Blood Bath. [A later contested patch](audits/2026-11-21-native-mineral-service-queue.md#contested-service-later-in-blood-bath)
  also removes its waiter and transfers service within five native
  updates of an eight-ore debit. In the
  [ordinary GBA predecessor save](audits/2026-11-21-native-mineral-service-queue.md#ordinary-predecessor-save-with-simultaneous-service),
  two SCVs service one patch concurrently and resume exactly
  after HOME. Keep that continuation valid while qualifying a
  saved service policy, depot travel and unloading, and campaign
  and CPU economy for new matches.
  The [packed row feasibility check](audits/2026-11-21-native-mineral-service-queue.md#queue-order-in-existing-width-save-rows)
  round-trips all 240 proposed waiter ranks through four codecs with
  independent timers. The [standalone FIFO stage](audits/2026-11-21-native-mineral-service-queue.md#standalone-fifo-and-journal)
  passes arrival order and journal fallback checks on both standalone
  profiles. A fresh ARM process restores the queue and matches 50
  updates. Eight independent workers add about 3,929 guest cycles per
  orders pass in the authored ARM fixture.
  [Linear rank reconstruction](audits/2026-11-21-native-mineral-service-queue.md#linear-reconstruction-from-saved-ranks)
  cuts the authored 240-worker load preparation from 6.97 million
  cycles to about 208,000 with matching state. All four packed row
  codecs feed the shared builder and preserve a worker's cargo.
  [Authored packed journal candidates](audits/2026-11-21-native-mineral-service-queue.md#authored-packed-journal-candidates)
  now exercise all four codecs through the SRAM selector, including
  rejected newer candidates and half-range fallback. The
  [shared product opt-in](audits/2026-11-21-native-mineral-service-queue.md#shared-product-opt-in)
  now connects those rows to the base, Carrier and research-jobs
  envelopes. Qualify a fresh-process resume from active and empty queues,
  protect older parallel saves, check the memory guards, and compare race
  economy and busy-match pacing before shared release activation.
  The [bounded patch scan](audits/2026-11-21-native-mineral-service-queue.md#bounded-patch-scan-and-fresh-process-continuation)
  reduces the authored ARM pass from about 16,000 to 7,400 cycles at
  eight clustered miners and from 55,000 to 47,400 at 72. Very dense
  fixtures add up to about 3,000 cycles. Compact and typed authored saves
  resume byte-identically for 100 updates in a fresh mGBA process.
  Attribute queue costs and verify full product resume in busy matches.
  A [fresh native opening sample](audits/2026-11-22-steady-order-cost.md#matched-blood-bath-credit-and-elapsed-time)
  brackets the first service claim at ares steps 5,941–5,955.
  The November 23 normal Terran release exposes Blood Bath as Free for All.
  The [matched Blood Bath defeat](audits/2026-11-26-blood-bath-defeat-parity.md)
  pairs two retained N64 captures with a passive physical GBA match from
  the same four start anchors. Both lose their final structure. The GBA
  shows four living SCVs on its result panel and retries with A or goes
  HOME with B after a byte-identical defeat reload. The earlier native
  survivor capture records neutral ownership at defeat. Verify the
  native score and controls after a Blood Bath defeat.
  The [current four-profile Melee preview](audits/2026-11-22-native-custom-game-lobby.md#november-23-preview-on-current-cartridge-inputs)
  passes 20 ordinary save continuations and 20 cross-version
  Free for All loads or Melee rejections. Random seats and four
  legacy campaign saves also pass. The last full three-CPU
  Melee samples average 16–20 emulated updates per second.
  Fresh-pause defeats match on all four preview products. The
  [normal-product candidate](audits/2026-11-22-native-custom-game-lobby.md#normal-cartridge-integration-candidate)
  keeps the RAM guards and repeats the campaign and skirmish
  startup and save checks, including the single-CPU active Terran
  win. Confirm local release integration, hardware pacing and
  active multi-CPU results. The [installed P10 pause probe](audits/2026-11-24-kodak-package-checks.md#pending-impacts-can-reject-an-ordinary-protoss-pause-save)
  rejects nine impacts and keeps the earlier journal intact. The
  [expanded P10 journal](IMPACT-EVENTS.md#typed-p10-layout) accepts
  that exact save and fresh-loads its ordered queue. It also passes
  a constructed campaign replay and the maximum 32-event ARM save.
  Measure the longer pause-save operation on a GBA SP.
  The [Z10 campaign journal](IMPACT-EVENTS.md#z10-campaign-with-the-carrier-cartridge-layout)
  saves the exact Z3 12-impact burst and restores all 32
  rows in the maximum fixture.
  The [depot-raid fix](SKIRMISH-AI.md#depot-raid-recovery)
  restores Terran recovery in three cases across the race
  profiles. Qualify busy ARM matches and address the six
  remaining host failures, including one new mineral-float
  failure in the mixed Zerg-profile match.
  The [late Melee samples](audits/2026-11-22-native-custom-game-lobby.md#instructions-in-late-three-cpu-melee)
  place the largest costs in orders and depth-ordered drawing.
  The [current one- and two-CPU budget](audits/2026-11-24-kodak-package-checks.md#late-frame-budget-with-one-or-two-cpus)
  measures the extra order cost at 66–73% of the Outpost
  increase. Its instruction cuts place 61–81% of the extra
  order cost in movement. Target contact work first for Outpost.
  The [rectangular memo trial](audits/2026-11-15-z7-contact-cost.md#column-bounds-in-the-current-replay-windows)
  retains more cache entries in the late three-CPU Outpost fight
  but records zero hits and raises p95. Z7 and Z9 checkpoint windows
  also record zero hits. Keep the current row memo and prioritize
  the uncached scan and visible actor drawing.
  The [screen-edge raster trial](audits/2026-11-24-kodak-package-checks.md#screen-edge-visibility-in-the-whole-raster-path)
  preserves both framebuffer pages across seven ARM windows, with
  no reduction in budget overruns and slower drawing in all three
  panned Outpost windows. Keep the current raster.
  The [selection-ring renderer](audits/2026-11-22-dense-release-attribution.md#selection-ring-work-in-zerg-9)
  cuts Z9 overlay cost by 25.4% and the separate busy window's
  mean loop time by 2.20%, with unchanged video-page bytes and RAM
  guard endpoints. The window remains above the two-VBlank budget.
  Continue profiling actor and effect drawing in dense battles.
  [Vital meter rows](audits/2026-11-22-dense-release-attribution.md#drawing-filled-vital-cells-in-one-pass)
  cut the selected-actor bar cost by 46.9% and mean Z9 loop time by
  another 2.25%. The two changes together save 4.40% in that window.
  [Rasterizing complete rings by row](audits/2026-11-22-dense-release-attribution.md#rasterizing-unobstructed-selection-rings-by-row)
  saves a further 0.62% in Z9 and 0.26–0.72% in five Showcase scenes.
  Prioritize movement and targeting setup in the dense Custom Game
  samples. Those core simulation costs are larger and apply throughout
  a battle.
  The [adaptive Pylon lookup](VALIDATION.md#qualification-with-the-queue-and-contact-changes)
  is enabled across all four canonical ROMs. On the combined engine it
  reduces mean loop cycles by 1.96% and 1.11% in two 69-Pylon Z9 windows.
  RAM endpoints, sampled stack high-water writes, both framebuffer pages,
  state and fresh save continuation match. Isolated display-register
  samples differ. The 54 Custom Game openings and fresh loads, full
  Terran 2 and Protoss 5 replays, and Zerg 10 late checkpoint through
  victory all passed. Keep whole-program peak stack, physical frame
  delivery and the 30-mission sweep on one release trio open.
  The [actual contact inputs](audits/2026-11-24-kodak-package-checks.md#contact-queries-in-those-outpost-windows)
  favor the guarded early axis check in the placed ROM kernel
  fixture. The [opt-in](https://github.com/ggu/starcraft-gba/pull/839)
  covers both ROM kernels. The [canonical pairs](audits/2026-11-24-contact-axis-bias.md#canonical-terran-and-z10-whole-frame-pairs)
  measure 1.55–4.81% reductions on Z10 and byte-identical
  P10 and Showcase links. Terran one-CPU Outpost averages
  within the budget while 52 individual updates still miss it.
  The [combined renderer and collision pair](audits/2026-11-24-kodak-package-checks.md#compose-the-renderer-and-rom-collision-option)
  saves another 1.03% of mean time in the panned Z10 fight
  with a higher p95 in that earlier sample.
  The [current contact and movement census](audits/2026-11-24-contact-axis-bias.md#current-paired-frames-and-contact-attribution)
  measures 1.05–2.70% gains in four busy changed-ROM
  windows. Three-CPU frames remain above budget and
  individual slower frames occur. The [default-on pairs with the row renderer](audits/2026-11-24-contact-axis-bias.md#default-on-with-the-selection-ring-renderer)
  match gameplay, both framebuffer pages, all seven pause journals,
  and fresh continuation. The Wasteland win and the late Z10 victory
  also replay on the new ROMs. Measure physical GBA SP frame delivery.
  P10 spends 131,466 cycles per
  sampled update in direct nearest-target queries and
  36,473 in rebuilding the target index. [Per-row occupancy trials](audits/2026-11-24-contact-axis-bias.md#per-row-target-mask-trials)
  raise both costs. Reuse facts from the existing index
  when testing further pruning, and retain stable entity ties.
  Keep a one-CPU Blood Bath tail check, where a near-budget
  mean masks 25–77 overruns per 120 updates.
  Prioritize repeated contact work and safe reuse across the two
  video pages. Trials in raster fog lookup, audio decoding and a
  single-direction contact bound did not improve these samples.
  The [same-page census](audits/2026-11-22-native-custom-game-lobby.md#same-page-battlefield-changes-during-melee)
  finds 27 identical battlefield draws in 32 earlier updates and about
  30 of 105 changed tiles per late update. In the same windows,
  restoring the old work page to bare terrain would touch all
  108 rows and require 341–531 aligned runs on average. Prioritize
  reusing unchanged contributors before trying sparse terrain
  transfers. Keep camera and scene transitions on the full-render
  path. The [contact census](audits/2026-11-22-native-custom-game-lobby.md#terrain-filtering-in-the-contact-scan)
  finds at most two new early exits in 80 Terran queries. The
  [ARM decoder trial](audits/2026-11-22-native-custom-game-lobby.md#first-order-audio-on-arm)
  increases late Protoss Melee cycles by 2.33%. A compact
  [IWRAM decoder and voice sum](audits/2026-11-25-audio-interrupt-cost.md)
  later cut the audio interrupt by 40–48%. DMA captures on all three
  race cartridges also match the previous mixer with four voices active.
  On the relinked cartridges, orders alone averaged 332k–430k guest
  cycles in three late skirmish windows. Prioritize orders in these saves
  and actor drawing on Zerg and Terran. Keep the 561,792-cycle whole-loop
  budget as the performance gate. Physical GBA timing remains open. Reusing
  [unit admission checks](audits/2026-11-22-native-custom-game-lobby.md#reusing-actor-admission-during-the-draw)
  reduces the late 600-update mean by 0.20%, while p95 rises
  0.46%. Late orders and drawing still need work to reach the
  two-VBlank budget.
  The [packaged Terran matched opening](audits/2026-11-22-steady-order-cost.md#packaged-terran-melee-mineral-opening)
  still gathers faster at the same Blood Bath corners. In the sampled
  seconds, the GBA has at most one worker actively mining each targeted
  patch, whereas the four native SCVs initially approach the same patch.
  The qualified native rerun confirms the claim precedes its positive
  work timer. Check first assignment and worker travel before tuning
  service time.
  [Mining-timing probe](https://github.com/ggu/starcraft-gba/pull/643)
- **Current cartridge campaign saves.** [T1 on the packaged Terran ROM](audits/2026-11-17-save-ui-product-matrix.md#november-23-packaged-terran-t1)
  wins at frame 10,120, with the frame-6,214 pause and victory reloading
  in fresh HOME processes. [Shadow Hunters on normal P10](audits/2026-11-13-p9-normal-home.md#scripted-destruction-and-result-autosave)
  writes a new journal at the actual victory. A fresh HOME load
  restores RESULT and re-exports the same 32 KiB SRAM. The other
  campaign result journals still need a common current-build gate.
  [P3 on the batched target-index cartridge](audits/2026-11-13-normal-campaign-matrix.md#november-23-batched-target-index-cartridge)
  restores frame 20,000 and reaches victory at frame 47,661.
  The 49 observations and a third-process result load pass.
  The same release wins Z1 from HOME with all 51 observations
  matching. The [Melee-enabled Zerg cartridge](audits/2026-11-13-normal-campaign-matrix.md#november-23-melee-enabled-cartridge)
  restores the Z1 pause at frame 984 and matches all 51 observations
  through victory and the fresh result load. The Melee-enabled
  Protoss cartridge also repeats P3 with identical pause and result
  journals. On the [Melee-enabled Protoss cartridge](audits/2026-11-13-normal-campaign-matrix.md#november-23-melee-enabled-cartridge),
  P8 matches all 40 checks, restores its pause and wins at frame
  104,113. A third HOME process restores the victory and exports
  identical SRAM. The same ROM also passes [P7 pause, victory and
  result reload](audits/2026-11-13-p7-carried-normal-home.md#november-23-melee-enabled-cartridge).
  All 12 snapshots match the archived field and display checks.
  [P10 on the same cartridge](audits/2026-11-13-normal-campaign-matrix.md#p10-on-the-packaged-protoss-cartridge)
  wins from the retained frame-36,761 pause and re-exports its result.
  The [regenerated full HOME route](audits/2026-11-13-p10-normal-home.md#november-23-packaged-full-home-win)
  wins from a fresh start at frame 93,377 with all 37 checkpoints
  matching. It restores both the frame-30,847 pause and the victory
  journal. Apply these result checks to the remaining missions on
  the current builds.
  On [the November 23 normal cartridge](audits/2026-11-13-p9-normal-home.md#november-23-current-cartridge-victory-and-result-reload),
  a regenerated P9 controller matches 93 ordinary-input checkpoints
  through victory. Separate fresh HOME loads restore the in-progress
  construction save and the victory journal. The longer route averages
  11.14 emulated calls per second. Focus on steady crowded gameplay cost.
  In [two post-save P9 windows](audits/2026-11-13-p9-normal-home.md#cost-of-the-post-save-location-checks),
  location checks take 242,186–260,553 cycles per update, or about
  18–20% of the main loop. A [direct Bring trial](audits/2026-11-13-p9-normal-home.md#direct-location-count-trial)
  lowers total cycles by 15.59–16.12% with matching sampled
  state and pixels. The [full P9 victory and result save](audits/2026-11-13-p9-normal-home.md#full-victory-and-journal-with-direct-location-counts)
  match all 93 checkpoints and all three SRAM exports. The replay
  goes from 11.14 to 12.16 emulated calls per second. The specialized
  location scan already ships in the shared bridge.
  [Inline cached validation](audits/2026-11-13-p9-normal-home.md#inline-validation-for-cached-actor-types)
  saves another 1.64–1.65% in two current P9 windows, 0.95% over
  the full P9 victory and 0.62% through the full P10 victory.
  Both full runs match their pause, result and readback journals.
  Busy skirmish windows match their existing timing. Continue
  checking other campaign providers and reducing steady shared costs.
  The [current Z8 comparison](audits/2026-11-12-z8-normal-physical.md#november-23-current-controller-comparison)
  matches the input tape and pause SRAM on the baseline and Bring
  builds. Both controllers lose after the first Nexus falls and mineral
  income stalls. The worker census finds stocked patches outside
  visibility and the 360-pixel worker search radius. Scout and
  transfer workers before depletion, then verify a new ordinary ARM
  victory. A 1,500-pixel search delays the host loss from frame
  62,535 to 79,353. The separate attempt-order change reproduces
  the stock failure. Measure scouting and actual cargo returns
  before treating the wider worker search as a recovery. Two
  [instrumented recovery trials](audits/2026-11-12-z8-normal-physical.md#observed-mineral-returns-in-the-recovery-trials)
  fail earlier than stock. A scout move stalls near a Hatchery,
  and changing worker priority increases early returns while
  Kerrigan still dies. In the [late survival trace](audits/2026-11-12-z8-normal-physical.md#late-hero-survival-on-the-melee-release),
  she takes sustained damage while idle. An ordinary move toward
  a nearby friendly unit reaches its destination and delays defeat
  to frame 78,849. A separate [worker transfer](audits/2026-11-12-z8-normal-physical.md#observed-deposits-after-the-worker-transfer)
  yields 22 deposits from a stocked field. Kerrigan's later retreat
  condition finds no eligible rally, and the host run loses at
  frame 69,355. The [recovered historical tape](audits/2026-11-12-z8-normal-physical.md#november-23-replay-of-the-historical-winning-tape)
  first differs at a sampled bank check and then restarts from a
  pause-menu shortcut. Resuming 25 unexpected pauses reaches
  defeat at frame 21,782 when an enemy Dark Templar enters the
  trigger area. Kerrigan is alive. Three [ordinary interception
  trials](audits/2026-11-12-z8-normal-physical.md#interception-trials-on-the-normal-z10-cartridge)
  preserve the defending group. Holding farther up the approach
  improves net damage from five to 19 HP, and an explicit attack
  matches the same health timeline. [Forward detection](audits/2026-11-12-z8-normal-physical.md#forward-detection-and-fresh-home-continuation)
  stops the attacker before the defeat trigger. A fresh HOME
  load and 120 neutral updates match the observed state after
  interception. The [continuation trials](audits/2026-11-12-z8-normal-physical.md#production-and-the-southern-approach-after-interception)
  explain the Hydralisk production stoppage and qualify its
  recovery by replacing the destroyed Den. Six Hatcheries accept
  18 new orders and the live count reaches 29. The direct assault
  stalls. The intermediate waypoint reaches the objective area,
  where the larger attack loses Kerrigan. Stage reinforcements in
  shorter moves and keep Kerrigan behind the main group while
  continuing toward a current-ROM victory.
  An [earlier Nydus diversion](audits/2026-11-12-z8-normal-physical.md#earlier-nydus-transfers-and-fresh-home-recovery)
  brings a Drone and three new Hydralisks south and verifies them
  after a fresh HOME load. The combined group destroys the first
  outer Cannon and retains seven Hydralisks, two Zerglings and
  full-health Kerrigan on another HOME load. The next attack
  destroys a Pylon and runs out of attackers before damaging
  the Nexus. The southern recovery builds a new Pool and Den
  and verifies three new Hydralisks in a fresh HOME load with
  the Cannon gone. Continue from that save, hold reinforcements
  outside attack range until they can advance together, and keep
  Kerrigan in reserve.
  The [next rally](audits/2026-11-12-z8-normal-physical.md#second-dark-templar-defeat-after-the-southern-attack)
  qualifies a fresh load with 22 Hydralisks at the forward
  position. The attack clears another Cannon and Pylon and
  trips the separate Dark Templar defeat condition. A neutral
  run from the same save stays active past that frame. Put
  detector-backed defense at the northwest objective before
  advancing the main group.
  [Three successive interceptions](audits/2026-11-12-z8-normal-physical.md#three-dark-templar-interceptions-and-a-fresh-home-save)
  now survive a fresh HOME load at frame 46,916. The third
  attacker beats passive detection and falls to an earlier
  explicit attack. Preserve the guard, train replacements
  for the eastern assault and keep detecting later attackers.
  The [late gas recovery](audits/2026-11-12-z8-normal-physical.md#late-gas-supply-and-the-northwest-defeat-trigger)
  qualifies an Extractor and a closer Hatchery, cutting observed
  eight-gas credit intervals from about 870 to 182 game updates.
  Two economy-only branches and the renewed assault all lose
  at frame 56,579. The fourth Dark Templar takes the northwest
  route. The [two-Overlord guard trials](audits/2026-11-12-z8-normal-physical.md#two-detector-northwest-formations)
  use the earlier post-Cannon save. Three formations all leave the
  fourth Templar at 21 HP when the objective triggers defeat at
  frame 47,628. [Two reinforcement routes](audits/2026-11-12-z8-normal-physical.md#northern-hydra-reinforcement-attempts)
  through the north lose their Hydralisks or stall before reaching
  the guard. The [lower corridor](audits/2026-11-12-z8-normal-physical.md#lower-corridor-and-overlord-transport)
  loses all five Hydralisks. The Overlord gets through, and its
  transport gates are unavailable in this stock Z8 save. The
  [early western Nydus](audits/2026-11-12-z8-normal-physical.md#early-western-nydus-and-hatchery)
  moves two Hydralisks west and the existing Hatchery trains a
  fifth. [Mobile defense](audits/2026-11-12-z8-normal-physical.md#mobile-defense-past-the-early-ground-attack)
  preserves five Hydralisks, a full-health Overlord and the damaged
  Hatchery through a fresh HOME load at frame 9,685.
  [Western reinforcements and interception](audits/2026-11-12-z8-normal-physical.md#western-reinforcements-and-the-first-dark-templar)
  train three more Hydralisks, regroup the army, and intercept
  the first Dark Templar with five surviving Hydralisks and an
  Overlord positioned forward. Fresh HOME at frame 21,883
  retains all six units. Fresh HOME at frame 27,641 also retains
  three Hydralisks transported south and three trained there.
  A Drone assigned to the idle Extractor restores gas after
  a fresh HOME load at frame 28,908. The [northern defense](audits/2026-11-12-z8-normal-physical.md#keep-the-northern-extractor-working)
  defeats the Reaver and four Dragoons and keeps gas collection
  active through a fresh HOME at frame 30,626. A later southern
  reserve adds three Hydralisks. A [southern interception](audits/2026-11-12-z8-normal-physical.md#southern-interception-and-northwest-detector)
  uses the existing southern detector, and a replacement
  Overlord reaches the northwest guard at frame 40,031.
  [Reinforcing the north first](audits/2026-11-12-z8-normal-physical.md#reinforce-the-north-through-the-nydus)
  defeats the next wave and preserves eight Drones, 357 gas
  and Kerrigan at fresh HOME frame 39,680. An [earlier northern hold](audits/2026-11-12-z8-normal-physical.md#hold-the-northern-approach-and-evacuate-workers)
  stops the next approach wave. Another southern Templar interception
  preserves active gas and all eight buildings at fresh HOME frame
  46,924. The alternate frame-48,594 save preserves all eight
  evacuated Drones. The [southern production recovery](audits/2026-11-12-z8-normal-physical.md#southern-objective-probe-and-production-recovery)
  establishes a Pool and Den, intercepts the northwest Templar
  before the frame-56,579 loss boundary, and trains six more
  Hydralisks. Fresh HOME at frame 58,760 retains the reserve
  and Kerrigan. The [eastern assault](audits/2026-11-12-z8-normal-physical.md#eastern-outer-defenses-and-nexus-shield-readings)
  removes the outer Cannon and Pylon. The direct attack takes
  the first Nexus down to 80/750 shields before losing seven
  Hydralisks. The [rebuilt formation](audits/2026-11-12-z8-normal-physical.md#rebuild-the-eastern-attack-to-eleven-hydralisks)
  increases the force to eleven Hydralisks at fresh HOME frame
  64,983. Attacking the Pylon first leaves seven for the Nexus.
  A direct bypass splits the force and leaves the Nexus at full
  health and shields. The [single-approach staging replay](audits/2026-11-12-z8-normal-physical.md#stage-the-eastern-formation-together)
  fresh-loads all eleven at frame 65,165 with Kerrigan separate
  at 400 HP. An attack from the same approach removes 48 Nexus
  shields and costs ten Hydralisks. A [reinforced assault](audits/2026-11-12-z8-normal-physical.md#reinforce-and-destroy-the-eastern-nexus)
  fields sixteen and destroys the eastern Nexus at frame 70,971.
  Fresh HOME retains the result and Kerrigan at full health.
  A separate defensive branch removes three attackers and
  fresh-loads the southern Hatchery, Pool and both Dens. Its
  second defense keeps the new Den and Kerrigan, with the Hatchery
  at 359 HP. A paid worker recovery restores a miner, a queued
  Drone and a replacement Pool under construction at frame
  75,299. A [separate same-chain recovery](audits/2026-11-12-z8-normal-physical.md#rebuild-after-the-eastern-nexus-falls)
  continues directly after destroying the eastern Nexus and
  fresh-loads a full-health Hatchery and Pool at frame 77,041.
  An earlier direct Move to the Reaver's approach leaves
  Kerrigan at her starting position. The [connected approach](audits/2026-11-12-z8-normal-physical.md#intercept-the-reaver-from-the-connected-approach)
  reaches the Reaver through six ordinary waypoints and
  defeats it. Fresh HOME at frame 77,066 preserves Kerrigan
  at 204 HP and the Hatchery at full health. A [later Pool](audits/2026-11-12-z8-normal-physical.md#pool-completion-under-the-next-attack)
  completes with Kerrigan still at 204 HP, but both buildings
  take damage before the first reinforcements hatch. Five
  Hydralisks reach the first connected waypoint with three
  injured. Both tested follow-up paths lose two. Test an
  earlier ground intercept. The [worker evacuation](audits/2026-11-12-z8-normal-physical.md#evacuate-workers-before-the-eastern-reaver-reaches-the-base)
  preserves all five Drones, 19 Hydralisks and Kerrigan at
  fresh HOME frame 72,755. The [replacement base](audits/2026-11-12-z8-normal-physical.md#found-a-replacement-base-without-moving-the-whole-refuge)
  completes with four surviving Drones, then produces
  a fifth that brings in 72 minerals. Fresh HOME at frame
  76,885 retains the miner and both enemy Nexuses. A follow-up
  Pool spends a Drone and the miner disappears. [Two guards and
  a second Pool](audits/2026-11-12-z8-normal-physical.md#two-defenses-restore-the-pool-and-expose-the-next-defeat)
  restore mineral deliveries and fresh-load at frame 80,708.
  Trigger 17 makes the southern Dark Templar invincible
  on entry and declares defeat at 80,798 after dialogue.
  An earlier guard defeats the Templar by 80,636. Fresh
  HOME at 80,900 retains 16 Hydralisks, five Overlords
  and full-health Kerrigan. With no workers or buildings,
  the force damages the eastern Nexus and fresh-loads at
  frame 82,525 before losing on the neutral continuation.
  A [second guarded interception](audits/2026-11-12-z8-normal-physical.md#intercept-the-second-southern-raid)
  fresh-loads at frame 82,409 with Kerrigan and 12 Hydralisks.
  The eastern group falls on the continued attack. An
  [alternate route with the economy intact](audits/2026-11-12-z8-normal-physical.md#keep-the-replacement-economy-through-both-southern-raids)
  kills both Templars and fresh-loads at the same frame with
  full-health Hatchery and Kerrigan, the Pool and one Drone.
  Use the preserved production to assemble the next assault.
  The native
  scenario permits Ventral Sacs for the alternate transport
  route. Qualify human Z8 research, boarding and fresh SRAM
  as one path. A builder can wait indefinitely at a disconnected
  footprint. The pending order charges resources only on arrival.
  A shared reachability check must distinguish an exhausted
  search from budget-limited work before cancelling that order.
  [Zerg 9 on the installed cartridge](audits/2026-11-13-z9-normal-physical.md#november-24-installed-cartridge-opening-and-air-production)
  fresh-loads an Extractor, Lair, Den, Spire and three full-health
  paid Mutalisks at frame 8,925. At frame 12,531, a fresh HOME
  load restores all five full-health Mutalisks at the first
  rally and three gas workers. The next fresh save has eight
  full-health Mutalisks. A successful interception keeps the
  full-health Extractor and all three workers. Fresh HOME at
  frame 26,691 restores fourteen full-health Mutalisks. The
  combined air group destroys the first road Cannon. A fresh
  frame-27,561 load keeps all fourteen and the gas line.
  The [road assault and crystal return](audits/2026-11-13-z9-normal-physical.md#road-assault-crystal-return-and-victory)
  clear four more Cannons, start the 600-second countdown and
  return the crystal at 27 HP. Z9 declares victory at frame
  57,089 and restores RESULT and byte-identical SRAM from HOME.
  A advances into Z10, whose opening save also restores from HOME.
  The [first Z10 raid](audits/2026-11-21-z10-current-full-circle.md#november-24-cartridge-opening-and-first-raid)
  leaves the eastern Sunken at 28 HP with the mining line still
  active. The [earlier defense](audits/2026-11-21-z10-current-full-circle.md#earlier-defense-and-reaver-pursuit)
  brings paid Hydralisks out before the raid. The mobile flank
  loses the mining line. The [four-Sunken branch](audits/2026-11-21-z10-current-full-circle.md#four-sunkens-before-the-raid)
  preserves both miners. After a focused attack the Reaver
  disappears, four Hydralisks survive and the Den falls.
  The [recovery](audits/2026-11-21-z10-current-full-circle.md#rebuild-after-the-first-raid)
  rebuilds the Den, puts four Drones on minerals and starts a
  second Hatchery. Fresh HOME at frame 15,812 has six
  Hydralisks and the new hall at 714 HP. A two-sided raid
  destroys both halls. [Relocated workers](audits/2026-11-21-z10-current-full-circle.md#relocate-workers-and-restore-mineral-income)
  establish a full-health Hatchery and resume mineral
  deliveries at fresh HOME frame 24,934. The next Reavers
  destroy the hall during passive play. A [second relocation](audits/2026-11-21-z10-current-full-circle.md#second-relocation-and-worker-recovery)
  restores the Hatchery and Pool, then adds an Overlord and
  three paid Drones. Fresh HOME at frame 34,571 preserves
  four Drones and 801 minerals. The attack arrives around
  35,300. A [separate production branch](audits/2026-11-21-z10-current-full-circle.md#hydralisks-before-the-raid)
  trains an Overlord and three Hydralisks before the raid,
  with fresh HOME at 34,723. The first mobile intercept
  loses all three. Test a closer defense with reinforcement.
  [Z9 phase measurements](audits/2026-11-13-z9-normal-physical.md#opening-frame-cost)
  average 19.15–20.46 emulated updates per second. A sparse-aware
  Pylon row grouping trial roughly halves the authored dense
  power-check cost. [Canonical ROM pairs](audits/2026-11-24-kodak-package-checks.md#pylon-lookup-in-dense-missions-and-panned-skirmishes)
  save 1.02–1.95% with 69 completed Pylons and under 0.1%
  with nine in panned Outpost. Every measured frame exceeds
  budget. P10 tail latency increases, and three samples stop
  with different display pages. Keep this trial off pending the
  stack and presentation checks. The P10 and Z10 panned
  samples spend about 860,000 and 668,000 cycles on the
  update respectively. Prioritize current measurements
  of orders and movement within that shared cost.
  The [shared distance-first cursor queries](audits/2026-11-13-p10-field-cursor-cost.md#current-four-cartridge-candidate)
  save another 1.24% over the full P9 victory and reload and 1.33%
  over the P10 victory tail. The P9 mean reaches 12.31 emulated
  updates per second. Foreground picks, the ordinary double-tap,
  six three-CPU matches and 24 fresh Custom Game loads agree.
  Two post-reload P9 instruction windows still average about
  1.10 million cycles each. The defense pass and target-index
  rebuild each account for about 80,000 cycles per update. Use
  the paired samples to measure these passes across changing
  live populations and confirm any gain in the late Zerg workload.
  A [fresh target-index counter trial](audits/2026-11-12-target-index-bulk.md#batch-the-fresh-dirty-cell-count)
  saves about 5,600 cycles per rebuild in the two P9 samples and
  0.33% over the P10 victory tail. The six three-CPU games and 24
  fresh loads agree. Full P9 victory uses 0.37% fewer cycles,
  with all 93 checkpoints and all three SRAM exports matching.
  Finish normal-product build qualification before activation.
  A local P10 armed-type lookup on top of the shared cursor queries saves
  2.39% and 2.37% in the P9 windows and 1.28% in the P10 tail.
  The sampled state, display pages and final P10 SRAM match.
  It frees 440 IWRAM bytes. Generate the facts for each exact
  feature set and check the other applicable profiles before
  activating the lookup.
  [P6 on normal P10](audits/2026-11-20-current-p6-save.md)
  reloads from frame 64,800 and wins with Zeratul and Tassadar alive. All
  113 checkpoints match, including 40 after the reload. The same cartridge
  loads the earlier frame-121 journal and matches its 120-update continuation.
  [P6 trigger and burrow changes](audits/2026-11-20-p6-runtime-cost.md)
  reduce full-route cycles by 16.66% with the same SRAM and victory
  checkpoints. The route still averages 15.75 calls per emulated second.
  Continue applying the normal-ROM gate to the remaining
  [campaign entries](audits/2026-11-13-normal-campaign-matrix.md).
  [Z8 on normal Z10](audits/2026-11-12-z8-normal-physical.md#november-20-normal-z10-with-fresh-sram)
  now wins after a fresh load at frame 16,659. All 30 post-load world
  samples match, with three battlefield framebuffer differences still
  open. The route averages 16.36 advancing updates per emulated second.
  The [Z8 attack-pose index trial](audits/2026-11-12-z8-normal-physical.md#attack-pose-lookup-timing)
  matches gameplay and video but raises mean cycles by 0.41%. Use the
  recorded busy draw windows for renderer work. The
  [instruction profile](audits/2026-11-12-z8-normal-physical.md#instruction-attribution-of-the-z8-stall)
  attributes 90.89% of frame 21,667 to 16 mineral continuation route
  fills. The [nearest-first selector](audits/2026-11-20-mineral-continuation-routes.md)
  cuts that stall by 87.9% while preserving the full Z8 replay checks.
  All four builds retain their RAM margins and match the control on a
  three-AI Blood Bath save and load. The next [Z8 samples](audits/2026-11-20-mineral-continuation-routes.md#remaining-slow-updates)
  attribute the two remaining orders peaks to movement route floods and
  the result stall to save validation. The [current Z8 point-prefix trial](audits/2026-11-16-resumable-frontier-budget.md#z8-check-on-the-current-mineral-routing-release)
  matches the release world and pixels while lowering the movement peak
  by 9.9%. All four current ARM builds and three-AI save smokes pass.
  The forced P10 cached read still defers a million-cycle full flood.
  Address that path on real terrain before normal activation. Qualify
  shared candidate row decoding separately.
  [P4 on normal P10](audits/2026-11-13-p3-p4-home-preparation.md#november-20-p4-fresh-sram-victory)
  wins from a fresh load at frame 65,700 with both heroes alive.
  All 146 checkpoints match, including 71 after the load. The run
  averages 15.19 calls per emulated second on the current P10.
  [P9 on this P10](audits/2026-11-13-p9-normal-home.md#november-20-fresh-sram-victory)
  restores three active constructions at frame 30,109 and wins with
  both heroes alive. All 42 checkpoints match, including 30 after load.
  The run averages 9.35 calls per emulated second.
  [P10 loads and wins on the nearest-mineral build](audits/2026-11-13-p10-normal-home.md#fresh-load-and-result-autosave-on-the-merged-build)
  at 10.53 calls per emulated second. Its second bank reloads the frozen
  victory with the same world and all three heroes alive.
  [T9 on normal Terran](audits/2026-11-13-t7-t9-normal-physical.md#november-20-current-terran-fresh-sram-and-result)
  loads from frame 35,363 and wins with Kerrigan alive. All 51 checkpoints
  match, including 12 after load. The second bank reloads the frozen
  victory and re-exports identical SRAM.
  [T10 on normal Terran](audits/2026-11-13-t10-normal-physical.md#november-20-fresh-sram-and-result-autosave)
  loads from frame 2,276 and wins with Raynor alive and the Ion Cannon
  destroyed. All six checkpoints match. A third emulator restores
  the frozen result and exports identical SRAM.
  [T7 on normal Terran](audits/2026-11-13-t7-t9-normal-physical.md#november-20-t7-fresh-sram-and-result-autosave)
  restores the loaded Dropship at frame 2,289 and wins with Kerrigan
  alive and the Emitter delivered. All 15 observation labels match,
  and the result bank restores the selected unit and re-exports
  identical SRAM.
  [T8 on normal Terran](audits/2026-11-13-t8-normal-physical.md#november-20-fresh-sram-and-result-autosave)
  resumes after relocation and worker landing at frame 2,568. All 272
  observation labels match, including 265 after the load. The Norad II
  survives, all hostile buildings are destroyed, and the frozen result
  re-exports identical SRAM.
  [T6 on normal Terran](audits/2026-11-13-t6-normal-physical.md#november-20-fresh-sram-and-result-autosave)
  saves with Raynor aboard the first Dropship at frame 19,466. The
  fresh route delivers Raynor and two Dropships, with Norad alive.
  All 42 observations match, and the frozen result re-exports identical
  SRAM. The 54.16-million-cycle result autosave is the slowest update.
  [T4 on normal Terran](audits/2026-11-13-t3-t4-normal-physical.md#current-packed-reader-release)
  restores the selected unit at frame 4,968 and wins with Raynor alive.
  All ten observations match, and a third emulator reloads the frozen
  victory with byte-identical SRAM.
  [T1 on normal Terran](audits/2026-11-13-t1-t2-normal-physical.md#current-packed-reader-release)
  restores Raynor after rescue at frame 6,214 and wins with ten Marines.
  All 12 observations match, and the frozen result re-exports identical
  SRAM. The route averages 29.28 emulated updates per second.
  [T2 on normal Terran](audits/2026-11-13-t1-t2-normal-physical.md#composed-attacker-depth-and-journal-build)
  restores the selected worker at frame 18,204 and destroys the Infested
  Command Center with Raynor alive. All 31 observations match. A third
  emulator reloads the frozen result and exports identical SRAM.
  [T3 on normal Terran](audits/2026-11-13-t3-t4-normal-physical.md#november-20-t3-fresh-sram-and-result-autosave)
  restores the countdown at frame 26,993 and survives all eleven waves.
  All 56 observations match, and the frozen result re-exports identical
  SRAM. The route averages 28.47 emulated updates per second.
  [T5 on normal Terran](audits/2026-11-13-t5-normal-physical.md#november-20-fresh-sram-and-result-autosave)
  restores the first paid Wraith attack at frame 72,076 and wins with
  Raynor and Kerrigan alive. All 162 observations match, and a third
  emulator reloads the frozen result and exports identical SRAM. The
  [single-cartridge Terran sweep](audits/2026-11-13-normal-campaign-matrix.md#single-cartridge-terran-sweep)
  now passes all ten normal HOME routes, fresh pause loads, victories,
  and frozen-result re-exports on ROM `4b66e7e8`. T1 and T3 average
  roughly 29 emulated updates per second. The slower routes still
  need performance work and a physical GBA pass.
  [Z1, Z2 and P5 on the current word-packed cartridges](audits/2026-11-13-normal-campaign-matrix.md#current-z10-and-p10-cartridges)
  match their full observations. Z2 and P5 also match their pause SRAM
  through a fresh process. Continue the other missions on those same ROM
  hashes and add a Z1 pause cut.

- **Late Zerg sight cost.** Sparse sight rows cut dense Z7 full-update
  cycles by 5.44% and late three-AI Blood Bath cycles by 0.84%. The Z7
  fresh load still wins at frame 194,998, and both versions emit the same
  Z9 save after 600 updates on a 128×128 map. Dense Z7 still spends
  about 1.37 million cycles per advancing update on orders. Prioritize
  route and recurring order costs before tuning sight again.
  [Sparse sight audit](audits/2026-11-20-late-zerg-sparse-sight.md)

- **Terran opening Bunker.** The shared planner places one paid Bunker
  after the Academy and before the Factory in the two observed Blood
  Bath Free for All lobbies. All four normal ROMs restore both the
  active SCV assignment and the four loaded passengers, then match
  600 updates from uninterrupted play. P10’s measured mixed windows
  average 1.01 and 1.18 million guest cycles per update. Compare first
  boarding and staffing after losses. Add `terran-factory-solo` and
  `terran-factory-mixed` to the matching Blood Bath replays. Extend the
  mixed fixture beyond the first-wave cutoff through update 16,008.
  Target the unmodified Factory windows after the paid Bunker and
  Academy complete, and compare the worker count, first-wave orders,
  and spending. Repeat from a fresh pause save after the Bunker fills,
  and check that the other map and early air or economy-loss fallbacks
  can still develop normally. Include the earlier Factory from the newer
  restored-lobby sample before using timing across mixed start rotations.
  [Terran opening Bunker audit](audits/2026-11-20-terran-bunker-opening.md#current-release-with-death-depth)
  [Factory timing evidence](audits/2026-11-20-terran-bunker-opening.md#factory-timing)

- **Native skirmish opening defenses.** The shared Zerg planner now
  builds four paid Sunkens by update 8,305 in both qualified Blood Bath
  lobbies. The normal Terran ROM restores the active morph from SRAM.
  The native fourth completion is about 650 to 800 updates earlier,
  and the mixed ARM workload takes 5.4% more cycles in the sampled
  late opening. Protoss now places a paid Shield Battery after 15 Probes
  and two completed Gateways in the same lobbies. Add
  `protoss-battery-tech` to the unmodified solo and mixed comparisons.
  Record the gas and Core foundations, and verify the paid Battery
  and pause-save continuation. For `protoss-cannon-opening-native`,
  repeat unmodified Blood Bath starts and record the first Nexus
  expansion, Pylon, Forge, Cannon, Gateway, and Assimilator before any
  player loses their final building. One unmodified restored lobby
  now confirms the expansion and Cannon sequence, while two other
  clean runs open with Gateways. Compare seat and opening selection
  before changing production behavior.
  The shared Sunken rule now uses the native 400 HP cap.
  Native morph completion and the GBA campaign and custom fixtures
  preserve damage. Fresh ARM loads on all four profiles keep the stored
  current HP of older colonies and complete an older pending morph at
  400 HP. Zealot save compatibility and remaining campaign replay work
  are tracked under current integration priorities.
  [Zerg opening defense audit](audits/2026-11-20-zerg-opening-defense.md)
  [Protoss opening Battery audit](audits/2026-11-20-protoss-battery-opening.md)
  [Native Cannon census](audits/2026-11-19-n64-skirmish-ai.md#protoss-cannon-opening)

- **Result journal under combat load.** The [completed-journal check](IMPACT-EVENTS.md#save-and-load)
  reproduced stale SRAM after a normal P10 victory with nine pending
  impacts. The shared writer now saves the completed result with the
  final world. All four HOST vectors check victory and defeat with a full
  queue, including the rejection of oversized saves during active play
  on Z10 and P10. The [Blood Bath retirement preview](https://github.com/ggu/starcraft-gba/pull/566)
  also passes the pause and victory boundary on Terran and Protoss ARM
  from prepared fights in both skirmish modes. Keep the preview opt-in
  pending full ordinary match and native retirement checks.

- **Packed journal decode.** [Byte-slice reads](audits/2026-11-20-packed-save-byte-reads.md)
  lower the Z8 pause, fresh load and victory costs by 26.2%, 28.2% and
  33.7%. The full replay, both SRAM exports and fallback to an older bank
  match the control. P10 and Showcase now share the byte reader for
  typed building rows. The [current P10 campaign pair](audits/2026-11-20-packed-save-byte-reads.md#replay-with-the-stationary-effect-depth-fix)
  cuts the pause load by 8.2% and the victory save by 7.8%, with identical
  journals. Both profiles also pass a three-AI save and fresh load.
  The [Terran compact-unit read trial](audits/2026-11-13-t6-normal-physical.md#compact-unit-read-timing)
  cuts the Norad II result autosave by 24.51% with a prior pause bank
  present. The pause and frozen-result loads drop 22.22% and 20.95%.
  The original pause bank and complete victory SRAM match byte for
  byte. The composed links retain all four RAM limits and the normal
  Terran active and staffed Bunker saves. The original victory
  profile attributes 5.90 million cycles to four immutable program
  validations. Factoring the duplicate source-level check within
  the adapter leaves timing flat because the compiler already
  combines it. [Validating each T6 journal candidate once](audits/2026-11-13-t6-normal-physical.md#composition-with-the-attacker-image-renderer)
  saves another 8.88% on the victory update and 5.86% on the pause load
  on the composed normal Terran. All observations and SRAM bytes
  match and both banks keep independent validation. The
  [remaining result profile](audits/2026-11-13-t6-normal-physical.md#remaining-cost-on-the-composed-terran)
  assigns 15.10 million inclusive cycles to compact-unit decoding and
  6.07 million direct cycles to the generic bit reader. The writer
  uses 1.57 million inclusive cycles. The
  [fixed-width decoder](audits/2026-11-20-packed-save-byte-reads.md#normal-terran-activation-with-aligned-route-packing)
  saves 18.96% on the result update with aligned route packing enabled.
  The pause and frozen-result loads save 16.14% and 14.92%. Both
  bank boundaries retain independent validation. In the
  [combined result profile](audits/2026-11-20-packed-save-byte-reads.md#remaining-save-cost-after-activation)
  the packed-building bit reader now uses 2.97 million direct cycles.
  Compare a fixed-width building reader against the current wire
  and both independent journal bank checks.

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
  The [short-path certificate trial](https://github.com/ggu/starcraft-gba/pull/616)
  cuts the current P10 late-battle peak by 42.42% and the mean by
  2.11% across 58 ordinary updates after a fresh load. The current Z10
  trial restores Z8 and samples 203 busy updates. Mean cycles fall
  7.68%, peak falls 10.25%, and p95 rises 0.14%. Sampled world, route
  counters and video match in each pair. The measured P10 and Z10
  range readers have zero visits to the separate far helper. Its
  1,087,475-to-70,103-cycle result comes from a forced read. Keep the
  flags opt-in. All four combinations now pass their normal link
  guards across the qualified revisions. Terran and Showcase also
  match both mixed three-AI Blood Bath saves and both fresh
  600-update continuations. The 128×128 Z9 observed range calls are
  short cache misses. The bounded deferred-read FIFO trial still
  raised the longest forced declined read from 1,245,315 to
  1,329,267 cycles and left a later whole-field stall. Keep it off.
  Target eager fills without moving work to a later update. A separate exact-cache scan trial on Z8
  matched 21 sampled states and saved less than 0.01% mean cycles
  across 486 timed calls. Focus on the larger route fills.
  A two-pop FIFO trial raised the sampled Z8 peak by 2.2%. Keep the
  single-pop loop. The [shared render cull](audits/2026-11-19-world-depth.md#visibility-and-building-overlay-cost)
  cuts mean loop cost by 0.17% to 0.84% across four Z8 and P10 windows,
  with matching sampled pixels.
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

- **Carrier skirmish parity.** All three normal race ROMs support paid
  production, flight, combat, and hangar saves. The [normal Z10 release](CARRIER-PROJECTILES.md#normal-z10-cartridge)
  passes human-paid production, active and docked SRAM, stock migration,
  all 378 custom starts, and the paired Z6 and late Z7 victory checks.
  [Normal Terran qualification](CARRIER-PROJECTILES.md#normal-terran-cartridge)
  covers human and CPU purchases, active and docked reloads, and stock
  migration. The [shared empty-pass check](audits/2026-11-21-carrier-empty-training-pass.md#shared-cartridge-qualification)
  covers both normal Carrier-enabled sibling ROMs. Qualify Showcase
  activation next. The [Showcase probe](CARRIER-PROJECTILES.md#showcase-carrier-probe)
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

- **KODAK delivery (2026-11-24):** the four verified `1f0ad66e`
  race and Showcase ROMs are on the card under
  `Games/Development/StarCraft-2026-11-24`. The
  [exact-package checks](audits/2026-11-24-kodak-package-checks.md)
  verify 1,134 normal-button Custom Game launches spanning both
  maps, every player race and every explicit one-to-three CPU
  lineup. All 30 campaign entries and eight Showcase scenes start.
  Thirty-six early Custom Game fresh saves load and match
  continuation. Six late three-CPU saves on both maps and all
  race cartridges also match uninterrupted play for 180 updates.
  Twenty-seven cases with Random seats preserve resolved races,
  rows and starts through fresh HOME load and pause restart.
  T1–T4 and T6–T10 win through ordinary inputs on the exact
  Terran image, including fresh mid-mission and victory loads.
  Z2 and all ten Protoss chapters also win on the exact images.
  Z1 adds a verified continuation from an earlier-release pause.
  Z3 wins after a paid economy and independently replayed assaults,
  bringing the exact-package victory count to 22 of 30. P4 also
  passes fresh field and victory save reloads.
  An ordinary T5 retreat keeps Kerrigan at full health through
  frame 129,658. Local box
  selection moves Marines with both heroes excluded. An explicit
  Move near input call 22,000 stalls. Redirecting Raynor around
  call 12,000 takes him to (856,398). A separate empty-ground order
  moves Kerrigan away from her previous target. Both retain 294 and 250 HP
  respectively through frame 19,288. The branch has only the two
  heroes and still needs a full win. A direct approach also enters
  the recruitment zone with Raynor, who dies there. Recruitment
  requires Kerrigan to enter.
  An earlier checkpoint and three short escort waypoints preserve both
  heroes and seven Marines at full health through frame 13,603. The
  fresh [escort journal](audits/2026-11-24-kodak-package-checks.md#terran-chapter-5-input-divergence)
  advances eight of the nine units. Advance the escort through
  additional short waypoints and verify recruitment at the rebel base.
  The same audit records fresh N64 reset replays of the two-slot
  Challenger and four-slot Blood Bath Melee lobbies, plus the
  Wasteland opening bank and five-Marine R selection on both systems.
  [The last open CPU edit](audits/2026-11-24-kodak-package-checks.md#last-open-cpu-row)
  matches in both directions and both Blood Bath modes on N64 and
  all three race ROMs. The [Z1 victory replay](audits/2026-11-24-kodak-package-checks.md#zerg-1-victory-replay)
  runs 17 matching ordinary-input segments from the imported
  frame-984 pause to victory. The result reloads byte for byte
  and the Z2 briefing opens through HOME. Recover or replace
  the input tape before frame 984 for an exact blank-SRAM run.
  [Z3's fresh start](audits/2026-11-24-kodak-package-checks.md#zerg-3-early-economy)
  produces a paid economy and a Hydralisk–Mutalisk army. Its
  first hostile turret falls by frame 12,156. The fresh save
  keeps the 250-health Chrysalis intact and restores with
  25 hostile buildings left. A separate staging branch reaches
  a completed Lair and Spire, two Drones on gas, 20 Hydralisks
  and five Mutalisks by frame 31,682. Its fresh load restores
  the full staging force and intact Chrysalis. Three gas Drones
  bring the force to 12 Mutalisks. A concentrated push removes
  two turrets with all 19 Hydralisks and 12 Mutalisks still alive
  at the fresh frame-48,118 checkpoint. A branch that keeps the
  ground force near the base clears three buildings and an
  exposed tank with 18 Mutalisks. The fresh frame-55,138 save
  retains all 18 Hydralisks, 18 Mutalisks and the intact Chrysalis.
  The next seven replayed segments reduce the hostile buildings
  from 23 to nine. A Hydralisk intercepts a Marine at the base
  with the Chrysalis at 170 HP. The recovered frame-64,819
  save reloads with three hostile buildings still standing.
  Combined ground and air orders win at frame 68,639 with the
  Chrysalis alive at 170 HP. Fresh HOME restores the result and
  the Z4 briefing deploys through ordinary input. A read-only trace
  identifies the intervening pause-save rejection at frame
  64,486 as a 12-impact volley against the eight-impact journal
  limit. Qualify combat saves beyond that limit without losing
  pending projectile outcomes or older journals.
  [Z4's installed-ROM replay](audits/2026-11-24-kodak-package-checks.md#zerg-4-victory-and-zerg-5-deployment)
  hatches the Chrysalis, completes paid air production and wins at
  frame 47,951 with Kerrigan alive. The victory loads byte for byte
  and Z5 deploys from HOME.
  [Z5's fourteen-stage replay](audits/2026-11-24-kodak-package-checks.md#zerg-5-victory-and-zerg-6-deployment)
  advances the terminal pause and wins with Kerrigan alive. The
  victory loads byte for byte and Z6 deploys. Z6 restores a paid Lair
  and Extractor
  after the first gas delivery at frame 3,519. Paid Ventral Sacs
  completes, Kerrigan boards an Overlord, and fresh HOME restores
  the cargo. The [Z6 ferry and assault replay](audits/2026-11-24-kodak-package-checks.md#zerg-6-ferry-and-mainland-assault)
  unloads her on the island at full health. A later fresh save
  retains 25 Hydralisks near region 4. Reinforced attacks clear
  the region by frame 63,540 with Kerrigan at full health. The
  [region 5 replay](audits/2026-11-24-kodak-package-checks.md#region-5-storm-attacks-and-evacuation)
  cuts the defending army from 67 to 30 with paid Storm casts
  and evacuates Kerrigan. A fresh continuation rebuilds a
  30-Hydralisk force. After another ferry and two more Storms,
  the fresh frame-130,610 checkpoint keeps Kerrigan on the island
  at 400 HP and all 30 Hydralisks, with 26 defenders and 14
  buildings left in region 5. A further ferry and coordinated advance
  clear both Force 2 regions. The [Z6 victory replay](audits/2026-11-24-kodak-package-checks.md#zerg-6-victory-and-zerg-7-deployment)
  moves Kerrigan across the island trigger and wins at frame 177,044
  with 400 HP. The victory reload is byte-identical and Z7 deploys
  through HOME. With Z9, the installed package has 27 of 30
  verified wins.
  The [Z7 guarded opening](audits/2026-11-24-kodak-package-checks.md#zerg-7-guarded-opening-and-production)
  completes a full-health Hatchery, then the Pool, Lair, Den
  and Sunken Colony. A paid defense save at frame 10,885
  restores eight Hydralisks, three Mutalisks and four Drones.
  After a northern interception, two watched Guardian slots
  hold Zerglings and three Guardians remain at fresh HOME.
  The ground push destroys the Pool. A combined defense with
  a distant attack order also loses the Hydralisks and Drones.
  Try a closer ground hold before sending the intercept north.
  Two fresh loads agree on the three-Sunken checkpoint. Ordering
  all 12 fighters east loses the whole base. The campaign stays
  active with 16 surviving actors under its zero-total-actors loss
  rule. Verify the equivalent state on N64 when a matching campaign
  checkpoint is available.
  The [regenerated Z7 winning input tape](audits/2026-11-12-z7-normal-physical.md#november-25-rendered-controller-recovery)
  wins on the padded-journal ARM build at frame 267,585 after
  matching all 206 named checks from empty HOME. The Z8 and Z10 results
  below bring recent normal build coverage to all 30 missions.
  The [Z8 Den and defense replays](audits/2026-11-12-z8-normal-physical.md#rebuilt-den-and-a-defensive-checkpoint)
  preserve the economy at frame 87,554 and hold the Pool with four
  Hydralisks through frame 89,195. Protect the forward detector before
  pushing the Nexus. The [staged eastern attack](audits/2026-11-12-z8-normal-physical.md#stage-the-eastern-formation-together)
  destroys a Pylon and lowers the Nexus to 170 shields before losing
  the assault force. On HOST, [moving the southern guard forward at frame 32,000](audits/2026-11-12-z8-normal-physical.md#rendered-controller-interception-before-the-southern-escape)
  stops the earlier escape. Keeping Kerrigan at her retreat destination
  lets the controller destroy both Nexuses and [win on ARM at frame
  86,969](audits/2026-11-12-z8-normal-physical.md#ordinary-arm-victory-after-the-delayed-retreat).
  Kerrigan has 392 HP. Fresh HOME restores RESULT and identical SRAM.
  The [Z10 relocated-base replay](audits/2026-11-21-z10-current-full-circle.md#hold-at-the-relocated-hatchery)
  verifies three Hydralisks and a new mining Drone at frame 35,958.
  Establish the defense earlier from frame 34,723 to survive the raid.
  The [Z10 dialogue-aware replay](audits/2026-11-21-z10-current-full-circle.md#victory-after-acknowledging-the-late-dialogue)
  advances the dialogue that froze the mission at frame 60,149, delivers
  the crystal and wins on the padded ARM build at frame 63,523.
  The crystal checkpoint and victory journal both fresh-load and
  re-export byte for byte. The latest vitals build also reproduces
  the same late-suffix inputs and result journal exactly.
  The T5 escort
  regroups through the south corridor and fresh-loads at frame
  21,884 with both heroes and seven full-health Marines. A second
  verified save at frame 22,304 has cleared a Vulture. At frame
  27,001 the six Marines and both full-health heroes reload at
  the south approach. Two focused attacks clear the first two
  bunkers. Both heroes are at full health and five Marines
  survive in the fresh frame-27,771 save. Kerrigan enters through
  the east corridor and recruits the rebel base. Fresh HOME
  restores 27 units and 19 buildings with both heroes alive.
  The captured Factory and Starport complete paid Vulture and
  Wraith orders. Two miners establish minerals and gas. The air
  force destroys the eastern Command Center and Refinery.
  Frame 57,882 fresh-loads seven Wraiths and both heroes. The
  next western attack removes a Missile Turret. Fresh HOME at
  frame 64,503 retains nine Wraiths, both heroes and 20 enemy
  buildings. Seven hostile Wraiths remain. A [separate reinforcement branch](audits/2026-11-24-kodak-package-checks.md#expand-supply-and-reinforce-before-the-western-assault)
  pays for a Depot and a tenth Wraith. Fresh HOME at frame
  68,291 has room for more at 44/50 supply, with both heroes
  and the gas worker alive. The reinforced wing destroys two
  western turrets, the Starport and two northern Missile
  Turrets. At frame 96,735, fresh HOME keeps eight Wraiths,
  both heroes and 1,092 gas. The hostile Wraith count is zero
  and 16 buildings remain. A new harvest order restores
  mineral income and is retained at fresh HOME frame 97,618.
  The [Terran 5 victory run](audits/2026-11-24-kodak-package-checks.md#terran-5-victory-and-fresh-home)
  reinforces, removes the remaining production and clears the
  last depots at frame 170,041. Fresh HOME restores RESULT
  and identical victory SRAM. A advances into Terran 6.
  Qualify the remaining routes on the latest combined package and
  measure GBA SP timing.
  [Native Wasteland](audits/2026-11-24-kodak-package-checks.md#wasteland-opening-on-n64-and-kodak)
  reaches the base with Raynor at 270 HP through ordinary controls.
  The first FlashRAM slot reloads at the base, and the second
  retains the earlier crossing. The next native save fresh-loads
  a 60-health SCV moved to the rocky pass. A subsequent run
  reselects it and moves it farther south. The cursor over a
  mineral patch shows B Harvest. The [harvest comparison](audits/2026-11-24-kodak-package-checks.md#wasteland-mineral-collection)
  confirms native deposits from 40 to 72. A fresh first-slot
  load from a separate save continues mining with no new order
  and identical FlashRAM. The installed GBA cartridge also completes
  two full-health SCV deliveries and resumes mining from a fresh HOME
  load with matching SRAM. The [Barracks comparison](audits/2026-11-24-kodak-package-checks.md#wasteland-barracks-construction)
  observes the 150-mineral charge on both systems. The native
  construction resumes after a fresh load. The infantry advisor
  activates the five-of-ten Marine counter. A new B Harvest
  order raises the native bank from 10 to 34. A later native
  run trains a Marine for the displayed price of 50. After fresh
  load, a second paid Marine completes. A fresh seven-Marine
  load then adds a third paid Marine and the objective reaches
  eight of ten. Three further native saves and continuations
  carry mining and paid infantry through ten of ten and victory.
  Accepting the result opens Backwater Station. A fresh process
  retains the unlock, opens its briefing and deploys to the playable
  map at Raynor's first dialogue. The [Backwater comparison](audits/2026-11-24-kodak-package-checks.md#backwater-station-opening)
  matches the starting bank and roster. N64 shows 5/26 and
  starts the SCV gathering, while GBA shows 6/26 and the worker
  starts idle. The native checkpoint fresh-loads and resumes
  mining. [Both Backwater Barracks](audits/2026-11-24-kodak-package-checks.md#backwater-marine-production)
  train a Marine for 50 minerals through ordinary controls.
  The N64 unit table gives both Raynor variants zero
  supply. [PR #837](https://github.com/ggu/starcraft-gba/pull/837)
  applies it in the shared rule. All four product links
  pass. Existing Wasteland and Backwater journals display
  one fewer supply with their prior actors intact, and
  Backwater trains a Marine for 50 minerals. Trace the
  native SCV's first target and order separately. The [Cancel Last
  comparison](audits/2026-11-24-kodak-package-checks.md#canceling-the-last-queued-unit)
  confirms a 50-mineral refund and uninterrupted first-Marine training
  on N64 and the updated GBA build. The HUD stays at 6/26 while
  a second Marine waits. Both versions return to 5/26 when the sole
  active Marine is canceled. A funded N64 Backwater run confirms one
  active and four waiting Marines, matching the GBA queue size.
  The [paired affordability check](audits/2026-11-25-visible-drawing-and-campaigns.md#backwater-marine-affordability)
  confirms both games reject an order under 50 minerals with a
  free slot and remaining supply.
  The [paired local-cap replay](audits/2026-11-25-waiting-training-supply.md)
  verifies idle refusal, paid waiting and Depot recovery on N64. The GBA build
  accepts a waiting order, stalls its head at 18/18, reloads that
  state and resumes from a completed Depot. Expand the native
  checks to Protoss, multi-facility start priority and the global 200
  limit. Continue fresh-load and exact campaign qualification on
  the shared implementation.
  [Measured ROM headroom](audits/2026-11-24-kodak-package-checks.md#rom-space-available-for-cutscenes)
  leaves 1.96–2.81 MiB per race cartridge for all future content.
  The USA N64 ROM uses 1.30 MiB for the whole slideshow bank and
  2.09 MiB for the opening animation, palettes and music. Short
  full-motion playback needs a benchmark on GBA SP.

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
  The [sparse-route Z10 cartridge](audits/2026-11-12-z10-arm-qualification.md#normal-sparse-route-build-and-fresh-victory)
  wins the final Zerg mission from HOME and from the new pause journal,
  with the same 20 post-load checkpoints and the same victory at frame 50,193.
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
  The [November 24 delivery rechecks the last-row edit](audits/2026-11-24-kodak-package-checks.md#last-open-cpu-row)
  through ordinary inputs in both Blood Bath modes on both systems.
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
  A [live final-building comparison](audits/2026-11-21-blood-bath-live-results.md)
  now witnesses defeat in two unmodified N64 Free for All launches and in the
  current Terran GBA ROM, including ordinary pause and fresh load through
  result. The current opt-in Terran replay also saves after CPU retirement
  and preserves four surviving human SCVs as neutral through defeat. A
  [one-update native capture](audits/2026-11-21-blood-bath-live-results.md#ordinary-human-survivor-transfer)
  shows the surviving SCV changing to neutral on the same update as
  defeat publication and the census falling to zero. The native census
  still counts the Command Center with zero HP before the scan. Capture
  competitive CPU retirement and concurrent final-building losses.
  The [normal Terran Blood Bath win](audits/2026-11-21-blood-bath-terran-win.md)
  covers blank HOME, actual paid infantry and buildings, and an ordinary
  victory against one named Zerg AI. The same 186 checkpoints match on
  the sparse-route release and the Terran Carrier candidate. Broaden
  the ordinary human route to more races and opponent counts.
  [Draft #445](https://github.com/ggu/starcraft-gba/pull/445)
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
  now pass the P10 HOST transfer and save tests. The [current preview](https://github.com/ggu/starcraft-gba/blob/codex/sc64-hangar-retirement-current-20261121/docs/audits/2026-11-21-carrier-retirement-refund.md)
  passes the Z10 and P10 HOST suites for paid queue refunds and neutral
  cargo. Z10 also resumes an ordinary active compact journal on ARM.
  Verify a positive retired-seat compact journal on ARM, launched
  fighters, the other coupled survivors, and simultaneous outcomes.
  The [November 20 preview](audits/2026-11-18-blood-bath-elimination-arm.md#november-20-composition-and-final-result-boundary)
  reproduces CPU retirement on current Terran and matches 900 updates
  after reload. Prepared Terran and P10 ARM fights in both modes also
  pass final retirement, neutral worker transfer, the between-pass save
  and fresh victory reload. The [native two-CPU Free for All opening](audits/2026-11-18-blood-bath-elimination-arm.md#native-two-cpu-free-for-all-opening)
  establishes the lobby and passive mining. The next native capture needs
  the last grounded building to fall while another CPU can still play.
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
  The [adaptive Cannon power lookup](VALIDATION.md#qualification-with-the-queue-and-contact-changes)
  cuts the dense lookup fixture from 80,203 to 41,838 cycles and
  retains sampled stack and fresh save behavior on the combined build.
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
  the loop with matching fields and no extra RAM. The newer
  [Z8 and P10 timing](audits/2026-11-13-route-linear-word-pack.md#current-z8-and-p10-pause-replays)
  finds lower route peaks with matching packed fields.
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
  The [aligned route word pack](audits/2026-11-13-route-linear-word-pack.md#current-z8-and-p10-pause-replays)
  keeps eager route fields and improves the sampled Z8 and P10 peaks
  by 1.47% and 0.96%. It leaves working RAM unchanged. Prioritize fewer
  exact route rebuilds or faster FIFO expansion for larger gains.
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
SD copy occurred. Research/ammunition/Flash experimental stages remain excluded.
That historical package predates the current 30-mission sweep. The near-30-fps
gate remains open. Keep source-only public artifacts and local Actions policy.

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
  [native fog audit](audits/2026-11-07-native-fog-memory/README.md).
  A [fresh scout-and-return run](audits/2026-11-07-native-fog-memory/README.md#scout-and-return)
  shows the same shoreline transition from dark to lit to dim after four
  Probes move away. This confirms retained terrain in the sampled area.
  The [remote mineral replay](audits/2026-11-07-native-fog-memory/README.md#remote-mineral-patches)
  then shows dim crystals and a persistent cyan minimap mark after
  the group leaves. GBA currently hides the world art and has no resource
  minimap overlay. Track the last observed presence of discovered patches
  in the shared engine and saves so unseen depletion cannot erase a mark.
  Geyser memory and a matched GBA display comparison remain open.
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
  A [Zerg Blood Bath Melee win against three allied Zerg CPUs](audits/2026-11-25-custom-game-full-matches.md#zerg-against-three-allied-zerg-opponents)
  completes one ordinary-input two- and three-opponent win per map and human
  race on mGBA; a mixed Terran/Zerg/Protoss alliance remains unqualified.
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
- The [matched Wasteland selection run](audits/2026-11-22-steady-order-cost.md#wasteland-selection-and-terran-7-continuation)
  confirms that five Marines remain selected after releasing R in both
  the native N64 emulator and the current Terran GBA ROM. Both controls
  keep selection empty. Terran 7 on the same GBA ROM wins after a fresh
  HOME save load and passes victory journal readback.
- [Wasteland camera comparison](audits/2026-11-22-steady-order-cost.md#wasteland-camera-and-fog)
  confirms that panning alone leaves unexplored space dark. Native analog
  scrolling places its indicator at the edge before the camera clamps.
  The GBA D-pad centers the cursor over the interior. Account for this
  difference in subsequent normal-input camera and fog comparisons.
- The [facility-transfer campaign checks](audits/2026-11-22-dense-depot-suffix.md#fresh-distant-requests-on-the-pacing-release)
  confirm all 30 starts and fresh SRAM victories in T10, Z1 and P10.
  The Z1 replay reloads at frame 984 and uses an ordinary A press and
  release to reselect the Hatchery during two neutral tape updates.
  All 49 remaining checkpoints match, and a third process verifies its
  victory journal. Continue qualifying other normal campaign save points.
- The [ordinary Custom Game openings](audits/2026-11-22-steady-order-cost.md#automatic-gathering-in-two-custom-game-openings)
  show automatic Terran gathering on native Challenger and GBA
  Outpost. A follow-up launches Blood Bath Melee with one Zerg CPU
  on both engines and confirms automatic gathering. Ordinary
  pre-launch delays now reproduce the native southwest human start
  with each possible Zerg corner on GBA. The first mineral credit is
  at neutral update 74 in all three. Verify the native Zerg start and
  gameplay clock before tuning the collection rate.
- [Fresh P5 and P6 controls](audits/2026-11-22-steady-order-cost.md#protoss-campaign-continuation-and-replay-controls)
  verify P5 and P6 victories after fresh SRAM load on the facility
  release. The [merged P6 controller](https://github.com/ggu/starcraft-gba/pull/728)
  verifies the intended unit after each contested click and matches
  105 ARM checkpoints. Qualify later campaign save points.
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
Scarab stages or resolve native canal timing. The content count at that historical revision was 29/30.

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
- **Zerg production:** Custom Games now produce through Larva and
  simultaneous Eggs for humans and computer opponents in every product
  ([Custom Game Larva and Eggs](DEVELOPMENT.md#custom-game-larva-and-eggs)).
  Open: Larva as actors that can be selected and killed, a hall link so
  hatched units follow a rally point, Egg animation, Lurker and Guardian
  morphs, and campaign adoption mission by mission with new replays.
  Z7's mission Eggs spawn the second unit of a pair inside the order loop
  without linking it into the target index; the Custom Game hatch now links
  it ([audit](audits/2026-11-25-custom-larva-eggs.md)), and the Z7
  path should get the same fix with a new Z7 replay.
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

The [shared Carrier shortcut](audits/2026-11-21-carrier-empty-training-pass.md)
recovers about half the added cost on the paired Terran-versus-Zerg win
with zero Carriers. The remaining cost is 2.45% versus the sparse-route
control. Preserve active human and CPU Carrier save checks while
profiling the residual cost and preparing a normal Showcase build.
The [completed building blit release](audits/2026-11-21-terran-building-word-draw.md#shared-release-checks)
cuts mean guest cycles by 8.46% in the ordinary Blood Bath win and
5.49–6.68% across the other three production replays. All four normal
builds match active-building saves and fresh display cuts. Profile the
remaining order and route costs and measure the release on hardware.

| ID | Priority / state | TODO and reason | Completion evidence required |
| --- | --- | --- | --- |
| C1 | P0 / in progress | A [paired 31k-call HOST trial](audits/2026-11-05-z10-damage-attribution/README.md) now attributes 12 lethal Drone hits and 63 lethal Hydralisk hits to the generic Reaver path. Trace/control and original-input sequence matched; all 99 observed AI unit purchases paid their expected costs. | Finish source attribution for generic placeholders, construction removals, target priorities and income; compare relevant native behavior. This HOST-stage result is not a guest win or proof that native ammunition alone changes Z10's outcome. |
| C2 | P0 / partial | Shared Reaver/Scarab behavior. **Done on the Protoss P10 ROM** by [PR #761](https://github.com/ggu/starcraft-gba/pull/761) ([details](PAID-SCARABS.md)): Reavers hold Scarabs (capacity 5), a player or computer opponent buys one at a time for the rule-table 15 minerals and 105 source ticks, an empty Reaver cannot fire, an attack launches one Scarab that flies, splashes 100/50/25 within 60 source px and is lost when its target or route is lost; custom games open the Support Bay and Reaver, Protoss opponents build and stock them; campaign Reavers placed at mission start hold 0 and computer-owned campaign Reavers restock on the 105-tick timer without payment; state lives in existing unit-row fields, so `RaceSave` stays 16,332 bytes and the existing SRAM save carries a Scarab in flight. **Open:** Zerg Z10 (and Z8/Z9) enemy Reavers still use the generic 100-damage attack (next stage: town-bank payment and Z10 requalification); the Terran, Zerg and Showcase ROMs compile none of it; placed Reavers start with their source hangar count (P7 owner 0 holds 5, the rest 0); Reaver Capacity and Scarab Damage research do not exist; launch delay, steering and the 90-tick life are not established against the N64; no cancel command; the ARM replay of a Scarab in flight is done on the mission 2 pad tape (the custom-game tape needs a bank fixture, so it stays HOST-only); the P7 controller was changed (it paints every frame, and releases a wave when supply is spent) and wins at frame 79,213 against the restocking enemy Reavers; the P10 controller wins since #750 (frame 49,001, identical on base); no hardware timing. | HOST product, ordinary-controls and save tests; ten Protoss controllers; AI and scripted-human matrices; ARM idle-cost window (all in the PR text). Remaining: extend it to the other release profiles, including Zerg campaign enemy Reavers, with source/native ammunition and attack-event checks, paid training, flight and splash, AI purchases, memory guards and fresh-process saves across those profiles and the full profile matrix; check the earliest ordinary legacy campaign save with a placed Reaver. |
| S1 | P0 / in progress | Finish shared Egg/Guardian production. Custom Games use Larva and Eggs in every product ([Custom Game Larva and Eggs](DEVELOPMENT.md#custom-game-larva-and-eggs)); that release needs no save tag, so it replaces the custom preview. The [opt-in late service](audits/2026-11-06-current-native-qualification/late-eggs.md) has four actual ARM campaign Egg/SRAM continuations; campaign activation and Guardian gameplay remain unfinished. | All real profile codecs and journals, interleaved tag contexts, old raw-36 Overlords, bad-newer fallback, atomic rejection, paired births, trigger reservations and Carrier/Egg capacity boundaries. Then ARM/RAM/stack, ordinary controls and genuine SRAM continuation before common activation. |
| S3 | P0 / open | Qualify saves throughout each combined campaign, including early Protoss missions under the P10 product. [One early physical P1–P7 save point each](audits/2026-11-06-early-protoss-save/README.md) now reloads with matching selected-state continuation on the pre-#323 `ff94a2ea` P10 ROM; P1 also has a matching mid-mission controller-tail continuation, but its historical tape no longer wins on the packaged product; [Regular P10 on November 17](audits/2026-11-17-protoss-early-replays.md) now proves a P1 fresh load through victory. P2/P3 win uninterrupted. P2 reload resets the remembered menu row and misses a Gateway; P3 reload has 100 fewer minerals at the next checkpoint. The [November 18 repeat-command release](audits/2026-11-18-saved-repeat-command.md#early-protoss-campaign-continuation) wins both saved routes on normal P10 from the same cuts. [Four current cartridges](audits/2026-11-17-save-ui-product-matrix.md) pass 32 custom-game fresh loads, and Races/Unified pass the opt-in host fixture. The candidate also wins Z1 and falls back to a valid older P3 bank after a malformed newer UI record. T1 saves and loads before the old input tape diverges. The [regenerated T1 route](audits/2026-11-17-save-ui-product-matrix.md) reaches Barracks with one fewer menu move and wins in both HOST builds. Both rebuilt ARM ROMs now win after the pause reload on their own tapes with the same named victory state and all 11 checkpoints. [Current P10 Homeland](audits/2026-11-13-p7-carried-normal-home.md#november-20-normal-cartridge-and-fresh-sram) now wins from a fresh load at frame 8,845, with all 49 remaining checkpoints matching uninterrupted play. The normal loader clears the selected Gateway and retains its four queued units. Continue qualifying other campaign save points with the normal loader and the current controls. | Fresh-process ordinary save/load and continued wins across all campaign entries on current combined flags, with source-owned effects, actors, clocks and objectives preserved. Reproduce and fix each rejection; do not infer arbitrary-save support from one point. |
| C3 | P1 / partial | All 30 mission controllers have won with ordinary input on their qualified HOST revisions. Z10 has its first win (`tests/sc64_z10_campaign_run.c`: a Sunken Colony fortress with Hydralisks through the first waves, then a Lair, Spire and 34 Mutalisks to the Temple, escorting the crystal Drone; frame 58,795, with a fresh-process save continuation to the same win). The [Z10 ARM replay](audits/2026-11-12-z10-arm-qualification.md) now reproduces that win and 59 matching checkpoints after a fresh SRAM load. Z10 also passes all ten HOST campaign starts/saves and the 234 custom startup/save lineups. [Carried Z7](audits/2026-11-12-z7-normal-physical.md) wins from HOME on the normal Z10 cartridge at frame 194,998 with 253 matching observations after the route-pure controller starts its paid air plan earlier. Its full replay averages 9.35 emulated updates per second. The November 13 Z10 KODAK cartridge also wins after a fresh SRAM load at frame 185,926 and one ordinary A press to reselect the Hatchery; all five remaining observations match. [Carried Z8](audits/2026-11-12-z8-normal-physical.md) wins from HOME on the same normal Z10 cartridge at frame 45,468, with 19 matching observations, both Nexuses destroyed and Kerrigan at 400 HP. [Carried Z9](audits/2026-11-13-z9-normal-physical.md) wins from HOME on that cartridge at frame 72,628, with 32 matching observations and a live Drone carrying the crystal back to the hive after the countdown. Several wins are timing-sensitive (Z6, Z8, Z10, P8). [Carried Z3](audits/2026-11-12-normal-campaign-replay-gaps.md) wins twice on HOST with identical route-pure tapes and wins from HOME on the normal Z10 ROM at frame 136,588 with 465 matching physical replay observations. [Carried Z4](audits/2026-11-12-z4-normal-physical.md) also has a route-pure HOST tape and a normal Z10 win at frame 34,984 with 42 matching observations on the nearest-first runtime. [Carried Z5](audits/2026-11-12-z5-normal-physical.md) wins at frame 48,347 on the same normal Z10 runtime, with 55 matching observations and separate physical acknowledgments for authored text and the source pause. [Carried Z6](audits/2026-11-12-z6-normal-physical.md) wins on the normal Z10 ROM at frame 56,913 with 65 matching observations, including Kerrigan boarding and leaving the Overlord. [T1 and T2](audits/2026-11-13-t1-t2-normal-physical.md) win from HOME on the exact KODAK Terran cartridge at frames 10,120 and 29,366 with all retained scalar checkpoints matching and authored pages acknowledged. [T3 and T4](audits/2026-11-13-t3-t4-normal-physical.md) also win from HOME on the exact KODAK Terran cartridge at frames 54,000 and 8,911, with all recorded checkpoints and authored acknowledgments verified. [T5](audits/2026-11-13-t5-normal-physical.md) now wins from HOME on the normal Terran T10 cartridge at frame 122,281 with 150 matching observations, both heroes alive and no hostile buildings. [T6](audits/2026-11-13-t6-normal-physical.md) now wins on exact KODAK Terran at frame 23,536 after replacing a host-only menu write with physical keys and isolating route queries. [T7](audits/2026-11-13-t7-t9-normal-physical.md) wins from HOME at frame 3,931 with Kerrigan alive and the Psi Emitter delivered. [T8](audits/2026-11-13-t8-normal-physical.md) wins from HOME at frame 139,216 with all 272 observations matched. The T7/T9 audit also verifies [T9](audits/2026-11-13-t7-t9-normal-physical.md#t9-new-gettysburg) through victory at frame 47,266 with all 51 observations matched, Kerrigan alive and all hostile Protoss buildings destroyed. [T10](audits/2026-11-13-t10-normal-physical.md) wins from HOME at frame 3,361 on the same Terran cartridge, with all six checkpoints matched, the Ion Cannon destroyed and Raynor alive. P3–P6 use isolated route observers. On the [November 17 regular P10](audits/2026-11-17-protoss-early-replays.md), P2–P6 win from HOME and P1 wins after a fresh load. | Keep the distinct Z9 save path; qualify a neutral carried Z7 save cut or persist the selection safely; Temple/crystal text and results screens; finish latest-ROM and fresh SRAM replays; keep all 30 winning as the engine changes. |
| N1 | P1 / in progress | Validate N64 actor records, game clock and event cadence before behavior-changing timing optimizations. The [matched T1 Marine STOP/no-STOP replay](https://github.com/ggu/starcraft-gba/blob/5445fb3d/docs/audits/2026-11-07-n64-stop-control/README.md#2026-11-12-matched-replay-without-stop) has identical RAM and frames at both pre-press boundaries. C-Down clears the move order and target by step 7,230; with no press the same Marine is farther along and still has the destination pending. The [matched native Start trial](https://github.com/ggu/starcraft-gba/blob/d44739a695afe28ef1ac9070c4d140029cd26bbf/docs/audits/2026-11-07-n64-stop-control/README.md#2026-11-12-matched-start-pause-control) opens Game Paused while retaining the pending move and target. The [matched menu Resume trial](https://github.com/ggu/starcraft-gba/blob/f2898c66202c4c66a6ffd75086c856a92468877f/docs/audits/2026-11-07-n64-stop-control/README.md#2026-11-12-matched-options-and-resume-controls) continues the same move; a second Start opens Options on the N64, while GBA field controls use Start as a Resume shortcut. A RAM counter freezes and advances with Resume, but its purpose is unproven. The [November 18 native actor replay](audits/2026-11-07-native-actor-differential/README.md#november-18-reproduction-from-retained-inputs) recovered the original observer sources and reproduced three frames and both RAM captures from retained inputs; the callback log differs. | Trace actual writes, compare idle and other actors, and establish an independent game clock before using it for timing. Keep snapshots bounded and read-only, with no MMIO side effects. The paired samples do not establish the precise stop tick or a selected-unit pointer ABI. |
| P1 | P1 / in progress | Reduce representative busy-frame cost toward 30 fps. The [2026-11-10 instruction-level profile](audits/2026-11-10-busy-combat-profile/README.md) found route floods at a third of custom-battle cost. Reusing evicted route fields from the spare bank, keeping fields across structure changes, an interrupt page flip, a cheaper placement cursor and cached tracer values cut three pinned 140-unit custom windows by 33–49% in mean emulator cycles, with HOST traces identical to the base in all 54 custom lobbies, and the P7 campaign window by 4%. A second round (see the audit's [worst-update section](audits/2026-11-10-busy-combat-profile/README.md#round-two-the-worst-updates-2026-11-12)) cut the slowest Zerg update by 15% and the Protoss one by 5% (cheaper depot slot scan and goal-cell test, incremental walkability update) with identical traces; the mean barely moves. Those windows are still two to three times the budget. | The [dense point-flood probe](audits/2026-11-12-dense-point-flood.md) ([#389](https://github.com/ggu/starcraft-gba/pull/389)) saves 1.76–3.32% mean and 13.36–22.27% peak loop cycles across the three paired ARM custom windows, with matching sampled workloads. All nine ROM verify targets pass, the 64×64 Z1 campaign wins in matching ARM replays, and Z8 produces identical SRAM and a matching fresh-process win. The 54 bounded HOST lobby trace pairs match. It is on for eligible maps in the canonical builds. Next: address depot floods after structure changes, Arbiter bookkeeping, target-index work, and terrain and unit drawing. Exact-source pairs on the pinned windows with trace identity; confirm on GBA hardware. |
| C4 | P1 / open | Replace campaign AI scalar approximations where verified source/native behavior differs. Late-Zerg waves use averaged groups/gaps; arrival, rebuilding and transport tactics are not equivalent to script commands. | Source script interpretation plus N64 observations of actual production, wallets, launch groups, targeting and rebuilds. Reproduce a concrete mismatch before changing AI; retain mission objective semantics. |
| U1 | P1 / partial | Scourge/Spire source restrictions and shared picker correction are merged in #290. Continue the broader campaign PUNI/UPGR/TECx audit, especially P10. | Preserve current controlling ownership, nonspending denial and full supported custom catalog. Apply remaining source permissions with per-owner tests and save compatibility; do not infer complete availability parity from this one product fix. |
| U3 | P0 / partial | Shared Forge/Core research. Done: the Protoss P10 ROM's custom games (SRAM, integer combat; [audit](audits/2026-11-11-protoss-research-sram/README.md)). Open: campaign missions (needs the generated source-policy header), computer-opponent research, Terran/Zerg/Showcase ROMs, Storm and other paths that bypass shield armor, hardware. Q8 fractions and Flash stay staged and unused. | Authenticated owner and payer identity; source initial/max levels; atomic paid producer jobs, power/cancel/death/capture/reset behavior, active-job saves and legacy migration; weapon/armor/shield/range effects on every relevant path; ordinary building menus and cross-product campaign/custom/Showcase checks. |
| U2 | P1 / partial | Carrier and Interceptor behavior. **Regular Protoss:** paid production, AI, flight and saves in custom games and P7–P10, including the shared Gantrithor service in P10. **Showcase, Terran and Z10 previews:** ordinary ARM controls paid for both units and saved and loaded both a live projectile and a docked Interceptor. The [ARM gate](CARRIER-PROJECTILES.md#pinned-arm-release-gate) qualified campaign openings, sampled custom lobbies, one generation of custom SRAM migration and short stack samples. All three combined previews now have exact computer purchase debits and active-shot ARM saves from blank three-AI Blood Bath matches. The [Z10 run](CARRIER-PROJECTILES.md#mixed-blood-bath-battle-from-blank-sram) also has a 1,000-update continuation through a fresh reload. **Regular Terran and Z10:** [Terran](CARRIER-PROJECTILES.md#normal-terran-cartridge) and [Z10](CARRIER-PROJECTILES.md#normal-z10-cartridge) pass paid human and AI combat, live and docked saves, stock migration and skirmish/campaign gates. Showcase still uses its separate preview. | Regular Showcase activation, full-pool saves, broader journal fallback and physical GBA stack and timing. Protoss: Gantrithor rescue stock is assumed; ARM Gantrithor combat replay and hardware timing remain open. |
| S2 | P0 / in progress | Maintain one shared capability and persistence contract across products. The stale Nydus guard now follows the selected codec's existing seven-bit capacity; no wire or cap change. P1 HOST research/controller saving is composed in #361. Ammunition, remaining controller contexts, repair, detection and morphs still need shared persistence. | Per-product and per-mode inventory from actual flags/call sites, lossless versioned representations, source-owned permissions, lifecycle/death/capture checks and every applicable save codec. No Showcase-only engine fix or mission-objective predicate used as a service gate. |
| K1 | P1 / partial | Qualify complete supported custom games for all race combinations and 1–3 AIs. Validate building/unit progression, economy, targeting, victory and long-game stability. The [2026-11-10 planner audit](audits/2026-11-10-custom-ai-reliability/README.md) fixed the opponent planner's stalls and economy failures on HOST: of 288 planner-driven matches (a mirrored-human fixture, both maps, every race mix, 1–3 AIs) 281 now reach the engine's victory or defeat instead of 271, all 144 on Blood Bath, and the AI matrix adds full-match, prerequisite, spending-audit and save/resume cases. The [ARM opening matrix](audits/2026-11-21-current-skirmish-matrix.md) covers every named or closed-seat lineup and 54 fresh continuations. Open: broader ordinary-input human wins, full-match ARM coverage, hardware, and K2/K3 below. Zerg opponents now order through Larva and Eggs; they still tech slowly where gas is short, because gas workers wait for nine Drones. | Both maps, each human race, homogeneous and mixed AI races, actual tech progression and save/load. Track global pool/queue pressure and ensure human production is not silently starved (measured: with three AIs at their shares the human keeps at least an equal share of unit and building slots). |
| K2 | P1 / fixed in #386 | [#386](https://github.com/ggu/starcraft-gba/pull/386) lets short-range ground attacks contact the blocked footprint edge and falls back to a perimeter field when the regular route cannot plan a step. Runtime `ecaa865` passes 640 Z7 sanitizer cases. The mirrored HOST Outpost fixture (Z7, Terran vs three Zerg AIs) goes from unresolved at 216,001 updates on base to Terran defeat at 34,091. The corrected ordinary-input Z8 controller wins on Z9 in HOST and mGBA at frame 45,468, with matching fresh-process save continuation. All nine ROM variants verify. The pinned Z9 ARM battle's mean loop cost rises 0.17%, still far above budget. See [the replay and performance audit](audits/2026-11-12-melee-perimeter.md). | Broaden exact N64 contact-distance evidence and continue K1/K3. The older Gateway placement fixture fails on control and candidate before depot logic; update its setup separately. |
| K3 | P2 / open | Custom-game end states and worker rally. A game ends only when the human or every AI has no structure, so a world in which no owner can ever act again stays open (six of 288 audit matches, all four-owner Outpost games with every field mined out). Separately, a new worker and a resumed cargo carrier go to the nearest mineral field anywhere, including another owner's base while the owner's own second base has ore (about 1% of worker spawns). | Decide the native end-of-match behavior from N64 observation before adding any rule. Owner-aware rally in the shared spawn path with campaign controllers rerun. |
| M1 | P0 / in progress | Keep expanded mechanics within GBA memory and global actor capacity. Z10 has reached the 240-actor pool, preventing its former 70-Hydralisk launch goal. | Unchanged safety reserves, ROM/EWRAM/IWRAM and stack checks, child/queue/trigger capacity accounting and sustained busy play. Document adaptation limits; do not present catalog capacity as native-scale simulation. |
| B1 | P2 / open | Finish combat-rule and attack-event fidelity: fractional HP/shields, armor/size/upgrade effects, Firebat collateral, appear-at-target wind-up, shield-hit effects and specialized weapons. | Verify current code first; compare source/N64 events, not only appearance. Exercise allies, invincibility, shields, deaths and impact saturation with persistence and clipping. See [combat audit](ENGINE-FIDELITY-AUDIT.md). |
| B2 | P1 / open | Unify shared projectile flight and source attack events. Current follow/bounce traces use fixed endpoints while damage stays bound to a live target; stationary/melee paths can apply damage before depicted wind-up. | Native moving-target/release observations; bounded shared flight/event state, consistent visual/damage impact, separate shots, verified bounce rules, death/slot reuse/overflow, genuine save continuation and crowded ARM cost across products. See [visual/combat audit](audits/2026-11-06-n64-priorities/visual-combat.md). |
| V1 | P2 / open | Audit remaining source sprites, animations and placement/order effects, including each supported unit/building, damaged buildings, corpses, resource cargo and construction poses. | Paired N64/GBA observations, correct source frames/anchors/palette/timing, no missing assets, scene/HUD clipping and crowded effect/VRAM reuse checks. Existing fixes must be checked before reopening old reports. |
| V2 | P2 / in review | Blocky fog/terrain boundaries. Branch `codex/soft-fog-finish-20261112` (supersedes draft #146) draws explored-but-not-visible terrain through a palette-derived shade map and softens fog edges with world-anchored dithered masks; unknown cells stay opaque black and sight rules are unchanged. On in the Terran, Zerg, Protoss and Showcase ROMs with all linker guards passing (IWRAM end unchanged; EWRAM end +4.1 KB Zerg, +9.3 to +9.6 KB others (mostly the shaded-tile cache); a 256-byte shade map in OBJ VRAM between the projectile arena and the terrain cache). Evidence class: authored approximation. Native captures show a gradual edge and [dim terrain after the scouts leave](audits/2026-11-07-native-fog-memory/README.md#scout-and-return). The GBA colors and widths need a matched display comparison. Cost on the pinned busiest 1,000 custom-battle updates: loop mean +3.2% Terran, +3.3% Zerg (16-slot tile cache), +3.1% Protoss (the draft was +31% to +35%; shade-only without edge masks +2.0% to +2.5%); simulation cycles unchanged. | Open: cost is above the 1% aim (strip composing in scrolling frames, per-frame signatures); native preview and real-LCD check of the dither; shared memory for scouted mineral art and minimap marks; a native check of geysers after sight leaves; campaign, SRAM and hardware qualification. See [the soft-fog audit](audits/2026-10-29-soft-fog/README.md). |
| Q1 | P2 / open | Resolve controls/advisor PR regressions and polish N64-informed UX: non-destructive deselection, queue cancellation/refunds, alert suppression, camera bookmarks, settings and results. | Ordinary-input campaign/controller matrix and updated replays, visible hints matching input, native alert/control observations where relevant, persistence and no paid-order surprises. |
| R1 | P1 / ongoing | Publish only qualified source, merge ready PRs, and package versioned ROMs with accurate descriptions. | `./dev release check/package/copy/verify` ([tools/sc64_release.py](../tools/sc64_release.py); see [Development](DEVELOPMENT.md#release-check-package-and-sd-copy)) now records one revision-stamped build/verify/HOST result per product (default, T10, Z9, P10, Showcase, optional Z10), versioned file names, `MANIFEST.json`, `SHA256SUMS`, a README with the source revision and the [hardware checklist](HARDWARE-CHECKLIST.md), and a no-overwrite, no-delete SD copy with read-back hashes. Still manual: prepared ARM/replay qualification beyond those steps, filling in the checklist on a device, safe ejection, and describing what is on a card only after a verified copy. No unnecessary Actions. |

P0 items are completion/behavior blockers, not permission to skip their memory,
save, source-boundary or regression gates. N1 and P1 should proceed alongside
campaign work; late cosmetic work should not displace them.

## Playable-production gate after the feature15 audit

Updated 2026-11-20 for the Protoss P10 ROM, limited to what was shown. The
custom menu now offers the Robotics Support Bay when `SC64_PAID_SCARAB` is
compiled, Reaver production is admitted, and a pad-only HOST test builds the
chain (Pylon, Gateway, Cybernetics Core, Robotics Facility, Support Bay), trains
a Reaver (empty), buys Scarabs from the order list, attacks, and saves through
the pause menu with a Scarab in flight (fresh-process resume matches through
impact). The earlier staged findings below remain true of the other products:
a spawned Reaver, a catalog row or an admitted product elsewhere is not a
playable tech tree, Z8 and late-Zerg AI towns have distinct banks, and Z10's
enemy Reavers still fire the generic attack. The staged items are marked in
[PAID-SCARABS](PAID-SCARABS.md#project-todo-c2-defect-list).

Staged-path findings (historical, kept for the other products):

Read-only review of staged source at `2e6bc51c` found that the passing HOST
service fixture begins with a spawned Reaver. Paid ammo controls exist only under
staged flags; capacity/damage research is unwired. Z8 and late-Zerg AI towns have
distinct banks, while the staged generic Scarab stocking falls back to a
combat-owner wallet outside the unit production bank swap. Current source hangar
bytes do not establish N64 starting ammunition. Research acquisition, native
replenishment/launch observation and campaign mechanics remain required for the
other products. The staged Flash plan is dropped.

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
| SD delivery | The November 24 KODAK folder holds verified T10, Z10, P10 and Showcase builds from runtime `1f0ad66e`. The [exact-package checks](audits/2026-11-24-kodak-package-checks.md) cover campaign and Showcase starts, custom lineups and early custom fresh saves. |

## Verification and maintenance

- Update this file after meaningful findings, tests, merges or release changes.
  Keep stable task IDs; add new confirmed gaps, retire disproved findings, and
  distinguish partial proof from closure. Reprioritize after evidence changes.
- Link detailed source-only audits instead of copying private ROM data, images,
  audio or saves. Consult [roster status](SC64_SKIRMISH_ROSTER_STATUS.md),
  [shared transport](SHARED-TRANSPORT.md), [validation](VALIDATION.md), and the
  dated audits for the actual scope of each earlier result.
- Add the [foreground picking cases](audits/2026-11-21-world-depth-picking.md#harness-cases)
  to the framebuffer replay harness. Start with the opaque Overlord and
  Hatchery hit on all four products and the Scout air missile's detached
  trail. Then cover lifted buildings, transparent pixels and contextual
  controls. Qualify same-kind hits, independent turret poses and exact
  N64 sprite-ID ties with active-camera replay evidence.
- Add the [selected building and detached attack sprite cases](audits/2026-11-21-world-depth-overlaps.md#replay-cases)
  to the framebuffer replay harness. Preserve separate masks for the ring,
  the detached sprite and the weapon projectile. Use the native sprite IDs
  for exact-tie cases.
- Add the [shared depth cases](audits/2026-11-21-shared-world-depth-regressions.md#replay-cases)
  for live Goliath turret overlap, body-only banks and active-camera sprite
  pool reuse. Capture the ore-return attachment and independent mining effect.
  Extend the nested-death overlap fixture to ordinary-input replay and
  capture last-image retirement. Add ordinary Mutalisk and Spore Colony
  impact cuts around child birth and expiration. Record the N64 pool IDs
  at opaque same-layer overlaps.
- Add the [native depth follow-up cases](audits/2026-11-21-native-depth-followup.md#replay-cases)
  by case ID. Start with same-Y Tank hull, turret and firing flash, then
  cover the separate Goliath turret and capture native pool-ID reuse.
  Promote the tested aircraft crop and detached missile-trail masks to
  ordinary-input cuts in campaign, Custom Game and Showcase.
- Add the [building crop and sprite ID cases](audits/2026-11-21-world-depth-edge-and-ties.md#replay-cases)
  to the framebuffer replay harness. Capture a completed building crossing
  each viewport edge, then capture opaque native exact ties with sprite ID
  retirement and reuse before changing GBA tie handling.
- Add the [fallback death depth cuts](audits/2026-11-21-fallback-world-depth.md#replay-cases)
  to the framebuffer replay harness. Verify ground collapse and aircraft
  overlap in ordinary play, measure a busy death scene, and check reachable
  legacy weapon fallbacks for missing depth metadata.
- Add [fixed-camera fog and HUD-boundary coverage](audits/2026-11-19-world-depth.md#cache-and-clip-regressions)
  to the world-depth replay harness. Keep the pixel checks for both capture
  modes and the legacy static-clip fixture. Include
  [mineral-edge-depth](audits/2026-11-19-world-depth.md#harness-cases)
  at both HUD edges and both sides of the camera.
- Use the [dynamic depth harness cases](audits/2026-11-19-world-depth.md#dynamic-depth-harness)
  for the next rendering qualification. Start with lift, landing and the
  mid-flight save boundary, then add
  [Interceptor flight overlap replays](audits/2026-11-19-world-depth.md#interceptor-flight-overlap)
  around the new native layers and both far-return save histories. Add
  [SCV foundation overlap](audits/2026-11-19-world-depth.md#scv-foundation-overlap)
  with an approach, attached builder, completion and fresh save load.
  Use the [projectile replay cuts](audits/2026-11-19-world-depth.md#projectile-replay-cuts)
  for moving and stationary source weapons. Pair ground and air shots
  with a lifted building and record the release-layer snapshot before and
  after target loss. The siege overlap case compares both draw orders
  to account for palette remapping. Verify separate effects and exact
  sprite-ID ties with their own opaque overlap captures.
- Add the [burrow depth cuts](audits/2026-11-19-world-depth.md#burrowed-collapse)
  for collapse, release and the fresh-load boundary. Use the actual native
  sprite pool ID for the same-anchor tie witness.
- Add the [point-Storm cuts](audits/2026-11-19-world-depth.md#point-storm-cuts).
  Capture the ground and air crossings, HUD edges and active save reload
  in P4, P6 and Z6. Record native unit-targeted casts as a separate case.
- Add the [nuclear target-dot cuts](audits/2026-11-19-world-depth.md#nuclear-target-dot-cuts)
  to the preview replay harness. Test both HUD edges and a fresh paint-phase
  reload in T8 and Custom Game. Follow with the
  [missile, burst and impact cuts](audits/2026-11-19-world-depth.md#nuclear-missile-and-impact-depth)
  for the two active native sprite layers and fresh descent-phase reload.
- Add the [Yamato charge cuts](audits/2026-11-19-world-depth.md#yamato-charge-attachment)
  for a flyer crossing the glow and a caster just outside the viewport.
  Use the [Yamato flight, trail and impact cases](audits/2026-11-19-world-depth.md#yamato-flight-trail-and-impact)
  for normal-input overlap and fresh-load replays. Keep exact native
  sprite-ID ties and the [Interceptor pulse](audits/2026-11-19-world-depth.md#projectile-replay-cuts)
  as separate harness cases.
- Add the [attacker image depth cases](audits/2026-11-19-world-depth.md#attacker-image-depth)
  to the normal-input rendering harness. Capture north/south and flyer
  muzzle overlaps, loss of visibility and shooter-slot reuse. Record the
  native turret-flag tie separately.
- Add the [death sprite depth cases](audits/2026-11-19-world-depth.md#death-sprite-depth)
  to ordinary-input replay coverage. The shared queue sorts source unit
  collapse frames, direct Death attachments and opcode-17 corpses and
  rubble. Start the [detached death replay cuts](audits/2026-11-19-world-depth.md#detached-death-sprite-replay-cuts)
  with gas bursts and the Queen's second image, then run both HUD edges
  across page flips. Check same-Y ties, burrow and special-effect pools
  with separate native and GBA overlap witnesses.
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
