# Paid Scarabs (Reaver ammunition)

Status 2026-11-12: working in the **Protoss P10 product** (campaign missions P1 to
P10 and custom games). Terran, Zerg Z9/Z10 and Showcase do not compile it; see
[Other products](#other-products).

Code: `starcraft/sc64_paid_scarab.h`, enabled by `-DSC64_PAID_SCARAB`
(`mk/paid-scarab/Makefile`, part of `PROTOSS_P10_FLAGS` only). The staged
`SC64_SCARAB_*` Flash-era probes are untouched and cannot be combined with it.

## Rules

| | |
| --- | --- |
| Price, build time | the `U_SCARAB` rule row: 15 minerals, 105 source ticks (131 updates) |
| Capacity | 5 (`sc64_psc_capacity()`; no Reaver Capacity research exists in the product, so 10 is never reached; the saved fields already hold 10) |
| Buying | one Scarab per command, paid from the owner's own bank when ordered; at most 5 on order; stock plus orders never exceed capacity; no cancel command (as for Interceptors), a dead Reaver's orders are lost |
| Attack | a Reaver in reach of a ground target with a Scarab held, none of its own in flight and its 60-tick cooldown expired launches one; with none held it never fires (it still walks into reach) |
| Flight | a `U_SCARAB` row: 3 ticks of launch delay, then 8 runtime pixels (16 source) per source tick toward the target, explodes within 10 source pixels of the target bounds; 90-tick life |
| Splash | the Scarab weapon's 100 Normal damage to every hostile ground actor within 60 source pixels of its bounds: full inside 20, half inside 40, quarter inside 60; shields then armor; burrowed actors only inside 20; air, own and allied actors never |
| Lost | target dead or its slot reused: the Scarab flies on and explodes at the last position; route blocked for the rest of its life: it expires with no damage; Reaver dead: the Scarab goes on, detached |
| Starting stock | production hands over an empty Reaver; a Reaver a mission places at its start (within 36 updates) is empty too, because the imported placed-unit rows (`Sc64ImportedUnit`) have no hangar field. A Reaver that appears later (reinforced, rescued) and one from a save written before this service starts **full** the first time the service looks at it. The full-load case is a project rule, not a source fact. |
| Campaign enemies | a computer-owned Reaver in a campaign mission starts a Scarab whenever it holds fewer than 5 and has none on order, one at a time on the 105-tick timer, and takes **no payment**: no mission was found in which the opponent has an engine bank for it (P7 owners 0 and 4 and the P8 to P10 enemy forces are scripted source banks). |
| Custom opponents | a Protoss opponent builds a Robotics Facility and a Support Bay (the custom menu offers the Support Bay now), trains up to three Reavers (one per three Zealots/Dragoons) and buys their Scarabs through the human's command from its own bank |

Approximations: straight-line homing at constant speed (no turn rate or
acceleration), centre-distance reach, the integer combat path (no fractional HP),
and a one-in-flight limit per Reaver. The starting-stock rule for placed Reavers
is an assumption (no source data was checked).

## Checked against the N64

Numbers from a private ares measurement (unit-table reads, source ticks); only
numbers are recorded here.

| Row | N64 | This build | Status |
| --- | --- | --- | --- |
| Build time | 105 steps | 105 source ticks | confirmed |
| Capacity | 5 | 5 | confirmed |
| Trained Reaver, stock | 0 | 0 | confirmed |
| Top flight speed | 16 px per step | 16 source px per tick | confirmed |
| Shot gap | 60 to 69 steps | 60-tick cooldown, one Scarab at a time | consistent |
| Target dies in flight | flies on, explodes at the stored position | same | confirmed |
| Quarter-zone hit | about 59 px, 25 damage, shields then armor | 25 at 41 to 60 px | consistent (one read) |
| Placed Reaver, stock | 5 for P7 owner 0; 0 for P7 owner 4, P8 owner 3, Z6/Z8/Z9/Z10 enemies | 0 for every placed Reaver | contradicted for P7 owner 0: known difference; the import has no hangar count |
| Campaign AI restock | enemy Reavers climb 0 to 5, one per 105 to 112 steps, the first order at once | same timer, no payment | changed to match |
| Launch delay | about 4 to 5 steps (resolution 1 to 2) | 3 ticks | not established |
| Flight steering | turns at a limited rate (8 px per step mid-turn) | straight-line homing | approximation |
| Player Reaver (trained or placed), price read, full and half splash zones, own-side damage, several Scarabs in flight, terrain following, 90-tick life | not measured | as in the table above | not established |

No parity is claimed beyond the confirmed rows.

## Storage and saves

Only existing actor-row and save fields are used (no new row field, no wider
unit number, so nothing here depends on the open 16-bit unit-number work):

| State | Field (existing P10 typed row width) | Used |
| --- | --- | --- |
| Reaver: held, on order, cooldown, established | `carry` (17-bit signed) | bits 0 to 13 |
| Reaver: progress of the front order | `work_timer` (11 bits) | 0 to 105 |
| Scarab: parent Reaver slot, or -1 when detached | `build` (existing range -2 to 239) | -1 to 239 |
| Scarab: target (unit or building number) | `target` (-1 to 399) | -1 to 399 |
| Scarab: last target position | `tx`, `ty` (11 bits each) | position |
| Scarab: remaining life | `work_timer` | 1 to 90 |
| Scarab: immune | `flags` (14 bits) | value 16 |

`build` and `target` are read and validated at those widths:
`race_paid_scarab_candidate_valid` (called from the Carrier candidate check, so
every load path runs it) rejects a Scarab whose `build` is outside -1..239 or not
a live Reaver, whose `target` is outside -1..399, a second Scarab for one Reaver,
life outside 1..90, a selected, non-immune or hurt Scarab, and Reavers over
capacity, over cooldown, with progress but nothing on order, or "not established"
with a non-zero field. `RaceSave` stays 16,332 bytes (read from the ELF's DWARF,
base and candidate). A save from the base ROM loads: its Reavers hold zero in both
fields, which means "not established". The staged Flash plan is dropped.

## Memory (P10, HOST-measured link, not hardware)

Base 03b5db64 against this branch (`verify-protoss-p10` passes on both):

| | base | branch |
| --- | --- | --- |
| `__iwram_end` (limit `0x03005000`) | `0x03004d78` | `0x03004e10` (+152 bytes, 496 left) |
| `__bss_end` (limit `0x0203e000`) | `0x0203bedc` | `0x0203bed8` (-4) |
| `sizeof(RaceSave)` | 16,332 | 16,332 |

The IWRAM growth is seven hot functions that each gained one "is this a
Scarab" test (`nearest_enemy_typed`, `sc64_update_exploration`,
`sc64_unit_bucket_move`, `sc64_contact_blocked_mask_uncached`,
`sc64_target_index_move`, `sc64_unit_buckets_begin`, `unit_step_clear`). A
flying Scarab must be invisible to targeting, vision and collision, and those
paths already run in IWRAM; no new function is in IWRAM. The Terran, Zerg Z9 and
Showcase ROMs, and the default ROM, are byte-identical to the base (same SHA-256),
and the retired Z6, Z7, Z8, P7, P8 and P9 targets still link and verify; none of
their compile lines contains the flag.

## PROJECT-TODO C2 defect list

| Defect from the staged path | Status |
| --- | --- |
| Combat-owner wallet fallback outside the unit-production bank swap | Not applicable: the new service pays from the owner's own engine bank (`race_owner_minerals`, the human's `minerals` or the opponent's bank); computer-owned campaign Reavers take no payment |
| 8-bit typed cooldown against 16-bit motion | Not applicable: no heading or velocity is stored; motion is integer homing from position and target |
| Live-Scarab HP container; dead hidden/training rows | Not applicable: stock is a count, the Scarab has fixed HP 20 and is immune, there are no hidden children |
| Typed writer, guarded decode, reverse bridges, fallback writer | Fixed by not adding any: the P10 typed row carries every field; base saves load |
| AI stocking only as a disabled probe | Fixed on P10: custom opponents buy, campaign opponents restock |
| Source hangar bytes for starting stock | Open: the imported placed-unit rows have no hangar field; placed Reavers start empty |
| Capacity and damage research | Open / not applicable: neither exists in the product; capacity is 5 |
| Terran, Zerg, Showcase | Open, see below |
| Z10 enemy Reavers | Open: next stage |
| Scarab aimed at a building whose slot dies or is reused | Fixed: every building death and spawn path, including the Psionic Storm kill, drops the target; tested with a new hostile building in the freed slot |
| P7 controller with restocking enemy Reavers | Open: it does not win; the engine checks are clean (path grid, reachability, Scarab rows) and the controller loses waves at the guarded Nexus, so the mission rule or the controller needs a decision |
| ARM replay of a Scarab in flight | Open: the HOST process-boundary save is verified, the ARM replay is not |

## Other products (path, nothing implemented)

Reavers cannot be trained in custom games on the Terran, Zerg Z9/Z10 or
Showcase ROMs (their menus hide the Support Bay and admission rejects raw 83
without the flag). What exists there is the Zerg late-campaign enemy Reaver
(Z8 to Z10 towns), which still fires the generic hit. To extend:

1. Compile the service: needs the Carrier hangar command (menu hook), the
   siege-bounds table, and an appended `U_SCARAB` id (Terran uses 44 of 63
   persistent ids, Zerg 42, Showcase 49).
2. Saves: the state fits existing unit-row fields, but the Zerg packed row
   (35 bytes) and the Terran packed row (38 bytes) must be checked field by
   field for `carry` and `work_timer` width; `RaceSave` free bytes are Terran
   512, Zerg Z9 72, Showcase 2,140 (P10 has 36).
3. Money: Z8 and late-Zerg towns have distinct source banks (`race_zl_ai_minerals`);
   restock must follow the same rule as P7/P8 (timer without payment) or pay a
   town bank, plus capture and controller-transfer rules.
4. Memory at the base (IWRAM end against `0x03005000`, BSS end against
   `0x0203e000`): Terran `0x03004838` / `0x020399e8`, Zerg Z9 `0x03004c88` /
   `0x02038938`, Showcase `0x03004f10` / `0x02039a14`; Showcase has only 240
   IWRAM bytes, the seven compares above cost about 150.
5. Requalify the campaign controllers of each product (Z10 has no winning
   fast-path with Reavers today) and rerun the custom matrix.

## Checks

| Command | What it covers |
| --- | --- |
| `make test-sc64-paid-scarab` | sanitized HOST product test: economy, combat, splash tiers, loss (target lost, expiry, walled-route detour, Reaver death, slot reuse), 21 malformed worlds, in-process and fresh-process saves, campaign restock and mid-restock save, and (with `PSC_BASE_TREE=<unchanged checkout>`) a base-written save |
| `make test-sc64-paid-scarab-ordinary` | pad-only scenario from the title screen: build the chain, train two Reavers, buy Scarabs from the order list, attack, save through the pause menu with a Scarab in flight, fresh-process load, continue through impact |
| `make test-sc64-protoss-px` | all ten Protoss mission controllers |
| `python3 tools/sc64_skirmish_ai_matrix.py --build build --output OUT --profiles protoss-p10` | Reaver cases with the spending audit (the default profiles keep their cases) |
