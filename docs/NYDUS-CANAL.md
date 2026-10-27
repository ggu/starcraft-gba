# Nydus Canal transport (optional)

`SC64_NYDUS_ENABLED` adds linked Nydus Canal transit to the unified race
engine. It is off in every shipped profile, and no canonical menu, mission or
campaign promotion changes. Z8 needs it for its preplaced raw-134 pairs. The
Z8 mission itself is not implemented here.

| Piece | File |
| --- | --- |
| Freestanding rules and bounded link state | `starcraft/sc64_nydus_rules.h` |
| Engine adapter: commands, orders, loss, ownership, save | `starcraft/sc64_nydus_adapter.h` |
| Asset-free rules test | `tests/sc64_nydus_rules.c` |
| Actual-engine test (Z7 runtime host) | `tests/sc64_nydus_game.c` |
| ARM probe driver (mGBA debug log) | `tests/sc64_nydus_arm_probe.c` |

## Provenance

**Source data.** The authenticated Z8 scenario has four raw-134 UNIT rows
(`tools/sc64_z8_pack.py`; record indexes 192, 194, 195 and 197). Each carries
relation flags `0x0400` and a serial naming one partner. The pairs are
reciprocal (192↔194, 195↔197), and every endpoint has source owner 3, the
human Zerg slot (OWNR 6, SIDE 0). The rows were read privately from the
retained CHK. No coordinates or other scenario values are committed.
Treating `0x0400` as the Nydus-link flag follows the CHK format convention.
The validator also requires the reciprocal serial links, so that bit is
not trusted on its own.

**Numeric facts.** HP 250, armor 1, 150 minerals, 0 gas, 600 build frames,
Large size, 2×2 tiles and 32/32/31/31 dimensions come from the existing
stock-profile transcription of pinned BWAPI `UnitType.cpp`. The same file
supplies sight 8 tiles (`sightRangeTiles`, 256 pixels) and `unitSpaceRequired`
255, which means not transportable.

**Behaviour.** This is the pinned PC reference policy from OpenBW
`bwgame.h` (`4b046d5`). It is not traced from native N64 code. The native
EnterNydusCanal/BuildNydusExit handlers in the USA ROM have not been
located, so any N64-specific difference remains unknown.

| Rule | Reference | GBA runtime |
| --- | --- | --- |
| Eligibility | `unit_can_enter_nydus` (2634–2642): both endpoints complete and linked; passenger has the entrance's owner, is not flying, is Zerg | Same predicate. Units that cannot move are also rejected: burrowed, carried, immovable, Egg or Chrysalis. |
| Command | `order_Follow` (4762–4766): right-click Follow on a canal becomes EnterNydusCanal | Press A on an owned, linked endpoint with units selected (the GBA smart command). Ineligible selected units are ignored. With no eligible unit, A selects the building as before. |
| Approach | `move_to_target_reset`, then wait until at the move target | Walk to the nearest walkable point on the canal's contact ring, then enter within ±32 px of its centre. **GBA policy:** imported-map routes cannot target a blocked centre. |
| Transit | `order_EnterNydusCanal` (6724–6758): hide, place near the exit and finish in one order step | Atomic in the passenger's order step. No frame, save, query or render ever sees a unit inside a canal, so no cargo storage exists. |
| Exit placement | `find_unit_placement(u, exit, false)` within ±128 source px (16692–16694) | ±64 GBA px (the same box at half scale), with 8 px rings searched nearest first and strict destination clearance, including when the passenger starts overlapped. **GBA policy:** the candidate order is authored, not OpenBW's. |
| Blocked exit | Failed placement restores the unit and retries next step | The passenger holds at the entrance with its order intact and retries. **GBA bound:** after one failed search at an exit in a tick, later passengers there with an equal or larger radius wait one tick. |
| Simultaneous passengers | Unit-list order | Ascending unit slot. Each placement sees earlier passengers' new positions. |
| Targeting during transit | `hide_unit` clears visible units' references (17045–17071) | Attack-target and Consume orders on the passenger end. Attack-move, idle and move acquisitions drop it. Direct-attack references clear. Selection is kept (`hide_unit(u, false)`). |
| Endpoint death | 16101–16107: clear both links, then `kill_unit(exit)` | The partner dies through the engine damage path in the same update, even if marked invincible, with no killer credit. Orders toward either endpoint are released. |
| Removal outside damage | — | Reconciled at the next tick's start, and before a building slot is reused. |
| Ownership | `give_unit_to` carries the linked exit (3223) | The transferred endpoint's owner and raw owner are copied to its partner on the next tick. The former owner's passengers are released. |

Not implemented: Drone morph into a canal, `BuildNydusExit`, construction
and cancel rules, creep, Hive prerequisites, canal sounds, AI canal use,
and Move-mode targeting of a canal. A canal also has no imported art: the
sprite roster lists raw 134 as intentionally art-free, and the engine draws
its generic fallback. These are open work, not claimed behaviour.

## Z8 integration hook

Compile the Z8 profile with `-DSC64_NYDUS_ENABLED`, which requires
`SC64_RACES`, `SC64_UNIFIED`, `SC64_ZP_CAMPAIGN_ACTORS` and
`SC64_ZP_CAMPAIGN_STORAGE`. Then:

1. Generate a private table of `Sc64NydusSourceRecord` from the descriptor's
   raw-134 `unit_records`. Each record takes `record_index`,
   `linked_record_index`, `relation` and `owner_raw`, plus `x`/`y` at the
   **runtime spawn centre** produced by Z8's own source-to-GBA conversion.
2. After spawning the source actors, call
   `race_nydus_set_source(Z8_GLOBAL_ID, rows, 4)` and then
   `race_nydus_bind_world(1)`. Fail closed if either returns zero.
3. Define `SC64_NYDUS_SOURCE_FOR_MISSION(mission, &rows, &count)` for the
   profile. Save preflight, using the candidate's own mission, and a
   fresh-boot load both resolve links through it before mission code runs.

The runtime does not depend on `sc64_z8_source.h` or the Z8 descriptor
format.

## Save and storage

The link table is derived, not saved. A load rebinds each source pair from
the restored buildings' identity: raw type 134 at the source position. A
pair must have both endpoints alive, or neither (destroyed). A lone
survivor, a duplicate or moved endpoint, mixed owners, or a passenger
aimed at a non-endpoint, a flying unit or a foreign mission's canal all
reject the candidate before live state changes.

`ORDER_NYDUS_ENTER` (value 11 in Z-profiles) fits the packed four-bit order
field, and `B_NYDUS_CANAL` fits the six-bit building type field. Both are
appended last, so disabled profiles keep every existing enum value.

Measured on the Z7 controller profile, with and without the flag:

| Resource | Off | On | Cost | Guard |
| --- | --- | --- | --- | --- |
| ROM text | 20,086,668 B | 20,092,020 B | +5,352 B | ROM ≤ 32 MiB |
| Initialized data | 492 B | 500 B | +8 B | — |
| BSS | 252,944 B | 253,016 B | +72 B | end `0x0203de00` → `0x0203de50` < `0x0203e000` |
| IWRAM | end `0x03004cd0` | end `0x03004da8` | +216 B | < `0x03005000` |
| SRAM payload | 16,296 B | 16,296 B | 0 | ≤ 16,368 B |

Runtime state is a 20-byte link table, the installed source pointer and pair
list, and a 40-byte per-tick search memo. Binding uses at most 16
stack-resident endpoint views. Shared actor structs are unchanged.

## Verification

```sh
make test-sc64-nydus-rules      # asset-free; also part of ./dev test
make test-sc64-nydus-game       # needs the private Z7 inputs
make nydus-arm-probe            # off/on ELFs and the probe ROM
```

`test-sc64-nydus-game` launches Z7 through the normal menu. It places two
linked pairs, as Z8's adapter would, and drives every passenger with
ordinary input under ASan and UBSan. It covers single transit,
eligibility, unavailable and unlinked destinations, six concurrent
passengers with replay-identical exits, a blocked exit that holds and then
releases, endpoint death and removal, slot reuse, ownership transfer,
mid-approach save/resume, six malformed saves, a foreign-mission source,
and hook-resolved fresh-boot loads. Extra regression cases reject separating
overlap at an exit and moving both endpoints of the unreferenced second pair;
the latter must fail before live state changes. A genuinely dead pair still
loads successfully. This is host proof only.

The probe ROM boots the same flag-on profile with an embedded key tape and
reports through the mGBA debug log; no debugger reads or writes state. In
mGBA 0.10.5 it passed six transfers, an in-process save/load rebind, and a
fully blocked return trip. Results are emulated cycles. A vblank is
280,896 cycles.

| Phase | Frame avg | Nydus service avg / peak |
| --- | --- | --- |
| Z7 opening, no canals (240 frames) | 996,603 | 0 |
| Six passengers approaching (134) | 1,340,981 | 21,698 / 32,445 |
| Transfer frames (6) | 1,213,740 | 36,437 / 47,025 |
| Six passengers at a blocked exit (900) | 1,122,709 | 69,979 / 71,409 |

Approach frames include ordinary movement and a camera move, so the
whole-frame difference is not a Nydus cost. Before the search bound and the
division-free scan, the blocked case cost 1.13 M cycles of Nydus service
per frame. This is emulator evidence only. No physical GBA was tested,
no fresh-process SRAM continuation was run, and no Z8 mission was played.

The table above was rerun after the strict-clearance and displaced-pair fixes
on 2026-10-27. All six transfers, blocked passengers, and the in-process save
rebind passed. The strict query shares the bounded bucket traversal with
ordinary collision checks; it adds no full-pool scan per exit candidate.
Feature-off preprocessing remains byte-identical. Local evidence is retained
under `.local/build/z7-live-20261027/pr67-fixed-root/`, outside the source tree.
