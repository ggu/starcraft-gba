# Z4 promotion and full-mission performance

The canonical outputs now expose T1–T10, Z1–Z4 and P1–P3. This is 17 mission
entries, not a claim that all 30 missions or the 30-fps goal are complete.
Staged Z5–Z6 and P4–P6 remain separate development profiles.

## Zerg output and saves

`./dev sc64 prepare/build/test/play --profile zerg` selects the four-entry
Zerg profile. Its output is `build/StarCraft-Zerg-Z4-GBA.gba`, game code
`SCZ4`, save generation 20, with emulator saves under `.local/play/zerg-z4`.
The explicit `zerg-z3` profile retains `build/StarCraft-Zerg-GBA.gba`, `SCZ3`,
generation 5 and `.local/play/zerg`. The old primary filenames remain real
files; `*-Z3` compatibility names link to them. No save is renamed or migrated.

Preparation now generates the Z4 scenario and shared actor bank. Readiness
requires both carried Z3 and Z4 gated map payloads, the sprite marker, and the
Z4 manifest's schema and source identity. A stale Z3-only stage header is
rejected even if a separate Z4 manifest is present. ROM/ELF selection in the
verification and split tools follows the new canonical paths.

The generic split report-backed SRAM checker still supports mission 14/Z3
and 22/P3, not Z4 mission 15. A carried Z3 report retains its actual producer's
`zerg-z3` label while ROM/ELF hashes authenticate the selected image. Z4 uses
its dedicated replay and save qualification.

## Current playthrough evidence

The former Z4 controller could continue inside a cursor helper past its frame
limit. Its helpers now enforce frame/host budgets and consecutive-stall bounds.
Gas assignment also follows a live Drone position and verifies selection;
walking to a stale coordinate had left the economy without gas or air tech.
All resulting world changes use ordinary controller inputs.

Z4 wins at host tick 59260 / native frame 59101. The 812-transition trace and
an independent input-only host replay reach the result screen with WON, a valid
model, completed phase and primary objective, no source faults or unsupported
scripts, living Kerrigan, completed Lair/Spire and no Force-3 Command Center.
The full current-flag ARM replay reaches the same terminal fields.

A two-process ARM SRAM probe uses the real save/load codec, writes a 32-KiB
save at frame 4936, and restores mission 15 and its model in a fresh emulator
process. The normal profile matrix checks all four menu entries, save/load
round trips, old-generation rejection and 234 skirmish configurations.

Carried Zerg chapters were rechecked using the larger canonical pools and
flags: the dedicated Z1 controller wins at tick 43604, Z2's escort controller
wins at frame 7601, and the hybrid Z3 controller wins at frame 54677. The Z2
controller tracks its moving carrier and keeps the initial escort nearby.
These are source-objective wins, without changing objectives, statistics,
resources, deadlines or actor limits.

P3's current 2999-input host and ARM replays win at tick 60197 / frame 59586.
Its gameplay-state checkpoints match before/after the route split. P1's
generic controller loses Fenix. P2's idle route reaches its timed arrival
with all 128 unit slots occupied: Fenix cannot spawn, but the adapter marks
arrival ready and then treats the unset hero slot as a death. The source
requires Fenix's survival after arrival; that condition must not be weakened.
Capacity handling and active P2 strategy need follow-up. Neither route is a
fresh current-source completion proof. Earlier frozen playthroughs
and the staged 22/30 history do not close this requalification gap.

## Promoted optimizations

`SC64_VITAL_BAR_FAST` avoids division and an overwritten background fill for
empty/full bars. An asset-free whole-frame oracle checks 1890 cases per path,
including offscreen coordinates, scene clipping and capacities up to 100000.
Intermediate-value arithmetic is unchanged.

`SC64_ROUTE_FLOOD_SPLIT` keeps cache lookup, clearing and metadata in ROM while
the existing flood loop stays in IWRAM. It preserves enqueue order, tie rules,
visitation restoration and completeness. Eight policy combinations pass the
independent BFS differential fixture. No new route state or save fields exist.
The canonical profiles retain complete-field routing; partial/resume variants
were rejected after worse maximum frame times and remain private experiments.

Z4 also enables the previously qualified terrain scrolling, visible static-key
culling, disjoint static dirty-row drawing and contiguous window transfers.
The normal scene clip is required for dirty-row drawing; other clips retain
the full static pass. Current host replays compare gameplay state every tick
and full framebuffers every 5000 ticks. All compared outputs match, including
construction, source props and dynamic Cannon frames.

| Full mission | Reference update / draw / full-loop cycles | Final update / draw / full-loop cycles | Full-loop fps equivalent |
| --- | ---: | ---: | ---: |
| P3 | 299266 / 458946 / 831225 | 298563 / 445386 / 819341 | 20.18 → 20.48 |
| Z4 | 293723 / 623376 / 1004486 | 294009 / 570595 / 946544 | 16.70 → 17.72 |

These are weighted Timer-2/3 cycle measurements in instrumented mGBA input
drivers. Full-loop time includes audio and presentation wait; draw excludes
that wait. Observer work is outside the measured range. They are not physical
GBA measurements or unmodified-ROM debugger qualification.

The Z4 terrain candidate's background mean falls 140208 to about 94341 cycles.
Drawing and full-loop means improve in every measured 5000-tick Z4 window.
P3's route split frees 848 IWRAM bytes versus the prior layout; Z4's split
frees 864 bytes versus terrain-only. Guards remain unchanged.

| Output | IWRAM end | BSS end |
| --- | --- | --- |
| Terran | `0x03004f10` | `0x0203ddd4` |
| Zerg Z4 | `0x03004c48` | `0x0203c410` |
| Legacy Z3 | `0x03004f38` | `0x02039c58` |
| Protoss | `0x03004c78` | `0x02039e00` |
| Showcase | `0x030049e8` | `0x0203dee8` |

IWRAM must stay below `0x03005000`, BSS below `0x0203e000`, and save payloads
at most 16368 bytes. Canonical tuning can be overridden with
`ZERG_CANONICAL_FLAGS=` or `PROTOSS_CANONICAL_FLAGS=`; required storage/profile
flags are separate. Other staged profiles and the original-art build retain
their previous tuning.

## Remaining work

Full missions remain well below the requested 30 fps, with expensive unit
rasterization during large battles and rare route-building hitches. Eight
missions still lack the staged completion evidence; more stages must also be
integrated into the three canonical outputs. P1/P2 need current playthrough
requalification. Native AI/script, movement and special-attack limitations
documented elsewhere still apply. The full goal remains active.

## Final local verification

The release checks passed locally on the source submitted with this audit:

- `./dev doctor`, `./dev build`, `./dev test`, and `make unified test-unified`.
- Python discovery: 472 `test_sc64*.py` tests passed. The shared campaign
  predicate fixture and updated ordinary-input Z2 controller also passed.
- `make test-sc64-route-flood-split test-sc64-vital-bars`: all eight route
  policies and 1890 whole-frame bar cases for each reference/fast path.
- `make race-campaigns zerg-z3 test-canonical-render showcase` and
  `make test-race-campaigns`: ARM links, ROM verification, canonical pixel/HUD
  fixtures, 54 menu launches and 702 startup/save configurations.
- `make test-sc64-zerg-carried-missions test-sc64-z4-campaign`: fresh wins
  for all four Zerg chapters under canonical flags, plus the 234-case Zerg
  skirmish matrix. The explicit legacy Z3 tests and both profile status checks
  passed separately.
- Full-mission instrumented ARM input replays for Z4 and P3, host state/pixel
  parity and the separate Z4 two-process SRAM check described above.
- Fresh stock/art rechecks: independent comparison of 228 raw unit identities
  and 132 weapons; stock motion/combat and all 12 building-capacity variants;
  23 sprite/Cannon tests without skips; all 50 source construction identities;
  and the compiled full Terran roster (43 visible units, 53 regular buildings
  and eight props). The only intentionally invisible actor is raw 101.

The construction pixel fixture checks source art and child-layer composition,
but selects expected stages with the same progress helper used by the renderer.
It does not independently establish native race construction transition timing.
Worker actions, finish/order signals and post-timer effects retain the limits
in the stock/art audit.

Terran, legacy Z3 and Showcase ROM hashes are unchanged from PR #29. Their
earlier qualification is preserved; no new physical-hardware result is claimed.

Private evidence is under `.local/build/z4-canonical-readiness/`,
`.local/build/z4-carried-missions/`, `.local/build/p3-carried-missions/`,
`.local/build/p3-current-profile/`, `.local/build/p3-vital-bar-fast/`,
`.local/build/p3-partial-flood/split-vital-full/`,
`.local/build/z4-terrain-scroll-audit/` and
`.local/build/z4-terrain-route-split-audit/`. Final checks are logged under
`.local/build/z4-promotion-final/`. All work runs locally; Actions remains
disabled/manual-only. No ROMs, maps, extracted art, saves or private tapes are
published.
