# Can-attack chain in Custom Games

Two behaviour-preserving changes to the attack path, measured on the
three race cartridges. Base: `codex/source-release` at `d496f1a2`.
All evidence is from mGBA (an exact per-instruction cycle profiler built
on its core); nothing here was measured on a real Game Boy Advance.

## What changed

**The duplicate can-attack test.** The two automatic acquisition paths
in `update_orders()` (an idle unit returning fire and attack-move) call
`nearest_enemy_typed(x, y, q->owner, range, q->type, q->raw_owner)` and
pass the result straight to `tick_combat()`. Every path through that
query admits a candidate only through
`sc64_auto_target_candidate_allowed()` with the same attacker, owner and
source owner, and that ends in the same `SC64_CAN_ATTACK_FROM_SOURCE()`
test that `tick_combat()` then repeated. In the profiles where
`SC64_TARGET_POLICY_PROBE` is set (Zerg), the candidate check takes a
shorter branch for non-Scourge attackers: the Z8 hidden-unit test, then
`race_can_attack()`. That branch applies the same rules under those
flags; the host proof below covers it too. `tick_combat()` is now
`tick_combat_from(q, target, validated)`. Those two paths pass
`validated = 1` and skip the repeated test. Every other caller passes 0
through the `tick_combat()` macro and keeps it: explicit orders,
defenses, Bunkers, Carriers and Scarabs. The checks before the test stay
where they were and still run: the Scourge bounds check,
`target_alive()` and the P6 infested attack. See
`starcraft/sc64_combat_validated.h`.

**Custom Game gates.** `race_t6_unit_burrowed()`,
`race_p6_combat_unit_burrowed()` and `race_late_airborne()` have no side
effects, and each returns nonzero only when a campaign-mission test
holds (`race_t6_active()`, `race_p6_active()`, `race_late_active()`;
the late-Terran test comes last, after read-only slot and flight
checks). So all three return 0 whenever `race_mode != 0`. `starcraft/sc64_custom_game_gates.h` turns each name
into `(!race_mode && (function)(slot))`. Custom Games skip the call;
campaign missions call the function and take exactly the path they took
before. Declarations and definitions put the names in parentheses so the
macros never expand there. Three call sites keep the plain call: the P6
burrow test in the `update_orders()` unit loop and the T6 and P6 tests
in the `draw_minimap()` unit loop. In those loops the inline
`race_mode` test made campaign missions slower (see below).
`race_bc_facility_airborne()` is real per-building state in Custom
Games, so it is not gated.

## Proof that the skipped test is a duplicate

On the host (`GBA_HOST`) the test still runs for validated calls, and
the build aborts with the attacker, target and frame if it would fail.
With `SC64_COMBAT_VALIDATED_REPORT` set, each host binary reports how
many validated calls it proved. Results of the host runs are under
[Host verification](#host-verification).

## Cost

Mean guest cycles per update over paired runs from the same save, base
ROM against changed ROM (`ab.py`: 300 updates for Custom Game saves,
150 for campaign pause saves). After every update each pair compared
the frame, screen, live unit and building counts, a fingerprint of the
decoded world state, the camera, the raw `u[]` and `b[]` tables, both
framebuffer pages and the BG palette; at the end it compared the
exported pause save. Pending impacts and fog grids are not hashed on
their own, only through what they draw and what the save holds.
**Every pair below matched on all of them.**

| Save | Updates | Base | Changed | Saved | % | Simulation only |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| Terran Outpost | 300 | 772,619 | 749,146 | 23,473 | 3.04% | 545,618 → 522,514 |
| Terran Blood Bath | 300 | 869,018 | 851,591 | 17,427 | 2.01% | 520,023 → 501,610 |
| Terran opening | 300 | 562,731 | 562,731 | 0 | 0.00% | 115,386 → 111,530 |
| Zerg Outpost | 300 | 1,058,372 | 1,046,026 | 12,346 | 1.17% | 674,475 → 662,113 |
| Zerg Blood Bath | 300 | 1,022,939 | 1,020,219 | 2,720 | 0.27% | 567,547 → 564,650 |
| Zerg opening | 300 | 561,897 | 561,897 | 0 | 0.00% | 129,776 → 129,776 |
| Protoss Outpost | 300 | 973,092 | 949,163 | 23,929 | 2.46% | 705,457 → 681,753 |
| Protoss Blood Bath | 300 | 1,002,514 | 1,001,593 | 921 | 0.09% | 702,705 → 701,844 |
| Protoss opening | 300 | 562,325 | 562,329 | -4 | 0.00% | 200,346 → 200,346 |

The openings already run at the VBlank pace (about 29.8 updates per
second), so the Terran opening's 3.9k faster simulation does not show in
the frame mean. The Zerg ROM in the final build is byte-identical to the
one measured first, so its rows are from that run.

Campaign pause saves (150 updates each), 19 of the 20 in the set. All
matched.

| Save | Base | Changed | Saved |
| --- | ---: | ---: | ---: |
| Terran 1 | 563,294 | 563,293 | 1 |
| Terran 3 | 566,157 | 566,157 | 0 |
| Terran 4 | 642,096 | 642,096 | 0 |
| Terran 5 | 575,896 | 575,896 | 0 |
| Terran 6 | 822,609 | 819,547 | 3,062 |
| Terran 7 | 605,057 | 605,980 | -923 |
| Terran 8 | 664,270 | 664,650 | -380 |
| Terran 9 | 1,230,957 | 1,226,998 | 3,959 |
| Terran 10 | 793,853 | 794,438 | -585 |
| Zerg 1 | 563,183 | 563,183 | 0 |
| Zerg 2 | 562,160 | 562,160 | 0 |
| Zerg 4 | 906,748 | 906,748 | 0 |
| Zerg 5 | 1,310,600 | 1,310,153 | 447 |
| Zerg 6 | 725,226 | 725,226 | 0 |
| Zerg 7 | 663,204 | 662,776 | 428 |
| Zerg 8 | 642,923 | 641,626 | 1,297 |
| Zerg 9 | 1,386,716 | 1,384,281 | 2,435 |
| Protoss 10 | 1,291,802 | 1,289,379 | 2,423 |
| Protoss cartridge, selected-mission save | 1,183,004 | 1,155,552 | 27,452 |

Terran 7, 8 and 10 are 0.06–0.15% slower. The `p10-p7victory` save does
not resume into play (the runner's `screen == 1` check fails on the base
ROM too), so 19 of the 20 campaign saves were run.

## Where the saving comes from

Per-function profile over 32 updates from each Outpost save (calls and
self cycles per update, base → changed):

- Terran: `race_t6_unit_burrowed` 201 → 5 calls, 9.3k → 0.2k;
  `race_late_airborne` 86 → 0 calls, 4.6k → 0; `sc64_p3_can_attack_from`
  64 → 36 calls, 17.6k → 9.3k self (the duplicate); `race_tick_buildings`
  self 9.1k → 6.3k (the airborne calls were inlined there).
- Zerg: `race_can_attack` 87 → 59 calls, 20.9k → 14.4k self;
  `tick_combat_from` inclusive 55.2k → 45.8k.
- Protoss: the observer can-attack chain (`sc64_p7_observer_can_attack_from`,
  `sc64_p6_can_attack_from`, `sc64_p3_can_attack_from`,
  `sc64_p7_observer_target_is_cloaked`) 146 → 105.5 calls, 96.0k →
  69.1k inclusive, a drop of exactly the 40.9 `tick_combat` calls per
  update; `race_p6_combat_unit_burrowed` 179 → 129 calls.

The largest function that got slower is `update_orders` self time, by
0.3k (Terran) and 0.5k (Protoss) per update.

### Gates left out of two loops

The first build gated every call site. It was bit-identical everywhere,
but the Protoss 10 campaign save ran 17.9k cycles per update slower. A
per-line profile put 5.1k of that on the gated P6 burrow test in the
`update_orders()` unit loop: about 25 more cycles per unit, because the
loop runs from ROM on Protoss and the campaign still calls the
function. The rest was spread over neighbouring lines in the same loop.
Without the gate there, Protoss 10 went to 0.5k slower. The minimap
loop's two gates cost another 2.7k of drawing on that save, and
removing them brings it to 2.4k faster. In Custom Games those three
gates saved little: drawing was 0.1–0.4k faster.

## RAM and IWRAM

`__bss_end` is unchanged on all three race ROMs. `__iwram_end`:
Terran 0x03004B78 → 0x03004BA8 (+48 bytes in `update_orders`, 1,112
bytes still free below 0x03005000); Zerg 0x03004FD0 and Protoss
0x03004F08, both unchanged. On Protoss `update_orders` runs from ROM and
is 20 bytes smaller (0xF42 → 0xF2E).

Older ROMs that link the engine, built from the same private inputs:
`__bss_end` is unchanged on all 15. `__iwram_end` grows 8 bytes on the
Protoss P3 (0x03004870 → 0x03004878), Protoss P6 (0x03004E78 →
0x03004E80), races (0x03004908 → 0x03004910) and Zerg Z8 (0x03004E60 →
0x03004E68) ROMs. It is unchanged on Backwater, Protoss P7, P8 and P9,
Showcase, skirmish, unified, and Zerg Z3, Z4, Z6 and Z7. `verify-sc64`
and `verify-campaign` could not be built: their private inputs are not
on this machine.

## Host verification

All host binaries ran with `SC64_COMBAT_VALIDATED_REPORT=1`. No run
reached the abort.

| Run | Result | Validated calls proven |
| --- | --- | ---: |
| `./dev build` | passed | — |
| `./dev test` | passed | — (no race game build) |
| `make test-unified` | passed | 8,083 in 38 binaries |
| `./dev sc64 test --profile terran` (`test-sc64-terran-full-campaign`, `make -k`) | passed | 710,178 in 31 binaries |
| `./dev sc64 test --profile zerg` (`test-zerg`) | 2 targets fail; both fail identically on the base | 428,839 in 12 binaries |
| `./dev sc64 test --profile protoss` (`test-protoss`) | 4 targets fail; all fail identically on the base | 3,634,520 in 53 binaries |
| Skirmish AI matrix, all three profiles, 460 cases | 454 pass, 6 fail; the base fails the same 6 | 119,115 / 127,684 / 126,193 in one 40,000-update mirrored Blood Bath match per profile |

The failures that also happen on the base `d496f1a2`, with the same
output and frames:

- Zerg `test-sc64-z10-replay`: the replay of the Z10 controller tape
  does not reach the controller's win at frame 51,588.
- Zerg `test-sc64-z10-carried-missions`: "source Force2 zones were not
  cleared within bounded normal-input attempts" at frame 33,474.
- Protoss `build/test-sc64-protoss-canonical-p7-controller`: two
  `-Wsign-compare` errors in `tests/sc64_p7_campaign_run.c`, lines 929
  and 941.
- Protoss `test-sc64-protoss-carried-missions` and
  `build/protoss-px/p3/trace.jsonl`: the P3 controller wins at frame
  45,165, then "controller UI did not normalize".
- Protoss `build/protoss-px/p7/trace.jsonl`: the P7 controller stops at
  frame 173,828 with `won=0`, 232 units and 117 buildings.
- Matrix: `match-zerg-1-0-pt-216000`, `match-zerg-1-2-pt-216000`,
  `match-carrier-protoss-1-2-pp-216000`,
  `match-carrier-protoss-0-1-zpt-216000`, `raid-terran-1-2-t-0` and
  `raid-protoss-0-1-t-0` reach the tick limit. All 460 case record
  files are byte-identical between base and change.

## Commands

ROMs: `make Z8_DESCRIPTOR=.local/build/z9-inputs/z8-descriptor.json
build/StarCraft-Terran-Transport1-GBA.gba
build/StarCraft-Zerg-Z10-Transport1-GBA.gba
build/StarCraft-Protoss-P10-Transport1-GBA.gba`, in this worktree and in
a clean worktree at the base. Pairs: `ab.py SAVE N BASE_DIR CAND_DIR`
and profiles: `prof.py PRODUCT_DIR SAVE 0 32 TAG`, from the
iwram-stack-reclaim worktree's `.local/iwram/` tools. Race suites: the
targets of `./dev sc64 test --profile terran|zerg|protoss`
(`test-sc64-terran-full-campaign`, `test-zerg`, `test-protoss`) run as
`make -k Z8_DESCRIPTOR=... TARGET` so that one failure does not hide
the rest. Matrix: `tools/sc64_skirmish_ai_matrix.py --profiles
terran,zerg,protoss --jobs 8 --output DIR`. Each failing target was
rerun on the base worktree.

## Not run

- Hardware: none of this was measured on a Game Boy Advance.
- The `p10-p7victory` campaign save (it does not resume into play).
- `verify-sc64` and `verify-campaign` ROMs (inputs missing).
- The build-time IWRAM stack check from pull request #882, which has
  not merged. No list on that branch names `tick_combat`, which this
  change renames to `tick_combat_from`. Its Terran list names
  `race_t6_unit_burrowed`, which keeps its name; in Custom Games it is
  now called about 5 times per update instead of 201, so the IWRAM
  placement gains less from it there.
