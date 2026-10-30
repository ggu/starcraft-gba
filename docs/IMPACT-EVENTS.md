# Pending projectile impacts

Ordinary attacks whose source weapon is a moving projectile now deal damage
when that projectile's impact is drawn, not on the update the shot is fired.
Melee, appear-at-target and appear-at-source weapons keep their immediate
damage. The mechanism is a bounded, saved queue in the shared race engine.

- Core: [`starcraft/sc64_impact_events.h`](../starcraft/sc64_impact_events.h)
  holds no game state: row packing, launch-order stepping, slot-reuse
  clearing, and the save codec.
- Game binding: [`starcraft/sc64_impact_adapter.h`](../starcraft/sc64_impact_adapter.h)
  and [`starcraft/sc64_impact_save_types.h`](../starcraft/sc64_impact_save_types.h).
  `game.c` calls it from `tick_combat`, `hit_stock_building_target`,
  `update_tracers`, `spawn_unit`/`spawn_build` and `clear_tracers`; the race
  save adapter stores and restores it.

It is enabled wherever the source weapon effects are compiled
(`SC64_RACES` with `SC64_WEAPON_SOURCE_VFX`). That covers Showcase, including
its custom games, and the canonical Terran, Zerg Z4/Z6/Z7 and Protoss P7
products, in both campaign and skirmish modes. Race builds without source
effects (`races`, `unified`, legacy skirmish) draw authored tracers and have no
source timing, so they keep immediate damage. The original-art default
profile is unchanged. `-DSC64_IMPACT_EVENTS_DISABLED` restores immediate
damage for A/B measurement.

## Evidence

**Native N64 data.** Timing uses the private bank that already draws each
projectile, imported from the authenticated USA ROM by
`tools/sc64_weapon_source_audit.py` and `tools/sc64_weapon_source_sprites.py`:

- `weapons.dat` bullet type, lifetime and flingy;
- `flingy.dat` top speed and acceleration;
- the attacker's IScript release event (`attackwith`, `attack`, `attackmelee`);
- the projectile's Init `sigorder`;
- the projectile's Death `domissiledmg` (opcode 27).

These records are N64 content. In the current bank, every moving-projectile
profile (bullet types fly, follow-target and bounce) has `domissiledmg` at
Death tick 0.

**PC behavioral proxy.** The interpretation of those records follows the
pinned OpenBW `bwgame.h` (`4b046d5f`):

- A normal-hit bullet damages its target when its script reaches
  `domissiledmg` (`bullet_hit` 14086–14099; opcode dispatch 14878–14881).
- Moving bullets die on arrival or at lifetime expiry (`bullet_state_move`
  14266–14272). Follow-type bullets track the target's position
  (`bullet_state_follow` 14274–14284).
- A bullet keeps its owner player separately from its attacker unit
  (`initialize_bullet` 14397–14478; `bullet_deal_damage` 13735–13741).
- Unit death and `hide_unit` clear bullet targets (`remove_target_references`
  12753–12756, called at 16121–16127 and 17046–17051).
- `create_bullet` returns null when the bullet pool is empty (14494–14498).

No OpenBW code is incorporated.

**Not established.** Native N64 bullet-state, hit-resolution and miss code
has not been traced, so N64 equivalence is not claimed for any of the
following:

- the launch-order resolution of same-update impacts;
- the deferral policy below;
- any timing beyond the bank's records.

## Timing

A shot's impact source tick is:

> release + projectile startup + flight + Death `domissiledmg` tick

Flight comes from the same distance/acceleration model the renderer uses,
`sc64_weapon_source_trace_flight_ticks`, measured from the heading-adjusted
launch origin to the target position captured at launch. Source ticks (24 Hz)
convert to 30 Hz updates with `sc64_weapon_source_vfx_first_gba_age`.

A packet launched on update N, whose impact first appears at presentation age
k, resolves at the end of update N+k. That is the same update on which the
renderer switches the projectile to its Death animation. The fixture asserts
this for each production vector: the projectile has reached Death on the
resolving update and had not one update earlier.

Two-shot weapons use the later of their two shots' timings for one packet that
carries both hits. Measured delays are 5–25 updates for 40–80-pixel shots,
and at most 255 steps are representable.

## Targeting and ownership

- The packet stores what the immediate path would have applied: target,
  per-impact damage, impact count, stock damage class, launch owner and
  launch actor type. Damage, including the owner's weapon upgrade, is fixed
  at launch. The PC reference instead reads upgrades at impact.
- Shields, invincibility (unit flag 16, campaign arrays and intrinsic types),
  armor, size scaling, burn ownership, mission death notes and death cleanup
  all run in the ordinary `hit_target_impl` path at impact.
- A moving target is still hit at impact, wherever it is.
- An attacker that dies after launch still delivers its packet with its launch
  owner. A later owner change of either attacker or target does not cancel it.
- A target that dies or becomes untargetable (`target_alive` false: loaded,
  hidden, burrowed where the engine excludes it) loses its pending impacts at
  the next step, even if it returns before arrival.
- Slot reuse clears references immediately in `spawn_unit` and `spawn_build`,
  so a new occupant is never hit.
- Impacts due on the same update resolve in launch order. A later packet on a
  target the earlier one killed does nothing. The death path runs once.
- Hallucinations launch no packet.
- Splash, Scourge and specialized adapters keep their own paths: Siege
  splash, P7 Archon splash, P7 Interceptor projectiles, the Reaver/Scarab VFX
  mapping, P6/T4 traps and T5 mines.

Glave bounces, miss chance, follow-target homing correction, attack wind-up
for appear-at-target weapons, and flame collateral remain unmodeled, as
before.

## Overflow

The queue has 32 rows in every enabled product. The 48-unit stress battle
below peaked at 29.

A launch that finds the queue full is refused before any side effect: no
effect, facing change, cooldown or damage. The attacker retries on the next
update, and towers do the same through `hit_stock_building_target`. Damage is
neither dropped nor applied early. A diagnostic counter records refusals; it
is not saved.

This differs deliberately from the PC reference, which drops the shot when its
bullet pool is exhausted. Structurally unrepresentable packets (damage ≥ 4096,
more than 16 impacts, owner ≥ 8) cannot arise from stock weapons, and target
and type ranges are compile-time asserts.

## Save and load

A save with no pending impacts writes the historical RaceSave-length record,
byte for byte. With impacts pending, the record is RaceSave followed by an
8-byte header and 6 bytes per live row, in launch order.

Loading reads the newest record for the save tag at either length:

- A RaceSave-length record restores an empty queue.
- An extended record's block is validated (magic, count, length, every row
  field) before any live state changes. A malformed block rejects the load.
- Legacy saves load unchanged; they predate in-flight state.

`common/gba.c` now alternates slots and orders sequences across lengths for
one tag, and adds `save_read_latest()`. Exact-length `save_read()` is
unchanged for the default profile.

Each product stores as many rows as its SRAM record leaves room for:

| Product | RaceSave | Rows saved |
| --- | ---: | ---: |
| Showcase | 14,188 B | 32 |
| Terran | 15,816 B | 32 |
| Protoss P7 | 16,292 B | 11 |
| Zerg Z6 / Z7 | 16,296 B | 10 |

A save attempted with more pending impacts than the product can store is
refused before SRAM changes, showing SAVE FAILED. The impacts land within
about a second of play. Pending rows are restored exactly, so a resumed game
resolves them on the same updates as the uninterrupted run. The fixtures and
the two-process ARM checks below verify this.

Real play stays well under these limits. In the T10 replay and in neutral
three-bot skirmishes, at most 4–6 impacts were pending at once; only the
constructed 48-unit stress battle reaches 29–30.

## Memory

| Product | EWRAM headroom (default → candidate) | IWRAM headroom |
| --- | --- | --- |
| Showcase | 200 → 272 B | 56 B, unchanged |
| Terran | 28,480 → 28,536 B | 488 B, unchanged |
| Zerg Z6 | 28,372 → 28,444 B | 64 B, unchanged |
| Zerg Z7 | 2,256 → 2,328 B | 640 B, unchanged |
| Protoss P7 | 23,828 → 23,900 B | 952 B, unchanged |

These are the five normal 32 MiB ARM link maps of default `cfdbe16` and this
branch merged with it, read from `__bss_end` and `__iwram_end` against the
unchanged guards. IWRAM holds the same symbols at the same sizes and ends at
the same address; only linker interworking veneers are placed differently.

- The queue costs 196 B: 32 × 6-byte rows plus 4 bytes of counters.
- Race builds store `Tracer` endpoints as 16-bit values. Tracers are absent
  from RaceSave, and race maps are at most 128 cells per side. That frees
  256 B.
- The extended save record lives in each product's existing route-scratch
  union (asserted), and the load candidate (up to 192 B) is on the stack.
- No queue code is placed in IWRAM.

## ARM timing (emulation)

`tests/sc64_impact_arm_probe.c` replaces `main.c` and runs a 1,200-update
battle in Showcase: 16 Dragoons and 8 Goliaths against 14 Mutalisks and 10
Hydralisks. It reports update and draw cycles from the cascaded timer through
mGBA's debug log, with no debugger. Drawing excludes the present step's
deliberate VBlank pacing. Run it with:

```sh
make build/StarCraft-showcase-Impact-Probe-GBA.gba
python3 tools/sc64_impact_arm_probe.py build/StarCraft-showcase-Impact-Probe-GBA.gba --out <fresh dir>
```

Results, mGBA 0.10.5, measured on `761597d` against that base:

- **Queue cost.** Computing a shot's timing takes 6,161 cycles, about 1.1 % of
  the 561,792-cycle 30 fps budget. A push takes 271
  cycles. One step over a full 32-row queue takes 1,794 cycles.
- **Disabled build.** The `-DSC64_IMPACT_EVENTS_DISABLED` build reproduces
  the base build's final world digest exactly and matches its update average
  (347,980 vs 348,081 cycles). The 16-bit tracers and the save change do not
  alter simulation.
- **Candidate battle.** Comparable 100-update windows (same live-tracer and
  unit counts) cost the same as base. The battle runs longer, though: shots
  in flight overkill targets already doomed instead of retargeting at once,
  so the heavy phase lasts about 650 updates instead of 400.
- **Budget.** This stress scene exceeds the 30 fps budget on base as well, so
  the probe gives no 30 fps evidence either way. It does not establish
  hardware frame rate.

## ARM replay and fresh-process saves

`tests/sc64_impact_replay.c` replaces `main.c` in a canonical race build and
writes nothing but the held/pressed keys `game_update()` receives. Its host
build, compiled from the same source and flags, is the oracle. Every world
line carries a digest of the saved world fields used by
`tests/sc64_skirmish_resume.c`, plus the pending-impact rows and result
flags. `tools/sc64_impact_replay.py` runs each ROM in fresh mGBA 0.10.5
processes with an isolated HOME, log mask 7 and no debugger port, then
compares every ARM line with the oracle.

- **Mission replay.** `tests/sc64_input_tape_record.h` records the held keys
  of the T10 host controller (237 changes over 3,432 calls) into a tape. The
  Terran ROM, with the controller's internal T10 entry, replays it. All 35
  checkpoints, one every 100 calls plus the terminal state, match the host
  oracle. The win comes at frame 3,361, the same frame the controller
  reports, with no mission fault. Impacts are pending on 1,012 calls,
  peaking at 6. The main stack uses 2,088 bytes, leaving 9,688 above the
  IWRAM guard.
- **Two-process saves.** Process one starts a skirmish through the normal
  menus. It plays neutral input until enough impacts are pending, saves from
  the pause menu (START, B), then records a neutral continuation: every frame
  for 60 frames, then every 50, up to 900. mGBA exits and flushes SRAM.
  Process two boots the same ROM with that SRAM and presses L at HOME. The
  normal loader validates the record. The restored world must equal the saved
  one, and every continuation checkpoint must match process one and the
  host oracle.

| ROM | Skirmish | Pending at save | Record bytes | Checkpoints |
| --- | --- | ---: | ---: | ---: |
| Terran | Blood Bath, 3 bots | 5 | 15,854 | 234 |
| Protoss P7 | Blood Bath, 3 bots | 4 | 16,324 | 234 |
| Protoss P7, capacity | Outpost, 1 bot, spawned battle | 11 (all rows) | 16,366 | 234 |

The capacity variant spawns the timing probe's battle after the normal
launch. At frame 24, with 12 impacts pending, its first pause-menu save
shows SAVE FAILED and SRAM stays empty. At frame 48 it saves all 11 of P7's
rows. Each `.sav` holds one sequence-1 record with an empty second slot.
Process two writes nothing, so its SRAM hash equals process one's.

Diagnostic ROM SHA-256: T10
`1d1740682642ad5ddf203e85493aa24e3d205d2eca1773bd1f1ca90edb14312c`;
Terran save
`3f9357723de7027afc8d7a57962b47384c6b55edf34fe68cf7b7168112de1896`;
P7 save `3ba4db75df46a9f52330251987c3e5fa86ce52d9ec66c4483ebca86e487cba62`;
P7 capacity `79ed07ac460e7ed099b690ef4029e1abb603a661690ed14b0a8181c035c52e5d`.

The T10 call windows average 0.89–1.14 M emulated cycles. These include
rendering and presentation waits; they are not a frame-rate result. Limits:
- T10 enters through the controller's compile-time probe entry, not the
  campaign menu.
- The saves are skirmishes, not campaign missions.
- The capacity battle is a constructed scene.

These diagnostic ROMs replace the product's `main`, so they are not
byte-identical to the shipped ROMs. To reproduce with prepared private
inputs:

```sh
# record: a copy of tests/sc64_t10_campaign_run.c that includes
# tests/sc64_input_tape_record.h right after game.c, built with the T10 flags
SC64_TAPE_RECORD=.local/traces/t10/tape.txt ./t10-record --limit 100000
python3 tools/sc64_impact_replay.py header .local/traces/t10/tape.txt .local/traces/t10/tape.h
make impact-replay
./build/test-sc64-impact-replay-t10 > t10-host.txt
python3 tools/sc64_impact_replay.py run build/StarCraft-t10-Impact-Replay-GBA.gba --out <fresh dir>
python3 tools/sc64_impact_replay.py compare t10-host.txt <fresh dir>/phase1/lines.txt
# saves: --phases 2, then compare against both phase lines files
```

## Validation

- **Asset-free.** `./dev build` and `./dev test` pass. `./dev test` includes
  `make test-sc64-impact-events`, the queue contract: packing limits,
  launch-order stepping, refusal on full, slot clearing, and every save-block
  rejection.
- **Production vectors.** `make test-sc64-impact-events-game` passes under
  ASan/UBSan for Terran, Zerg, Protoss P7 and Showcase, covering:
  - launch versus the drawn impact frame and exact packet damage;
  - moving targets, dead targets, untargetable-then-returning targets, T5
    transport loading, and slot reuse;
  - attacker death and owner changes;
  - simultaneous impacts and single death;
  - shields read at impact, invincibility and death cleanup;
  - overflow refusal for units and towers, and tower projectiles;
  - immediate melee and appear-at-target weapons;
  - save/load continuation, malformed blocks, newest-record selection across
    lengths, and the per-product save capacity.

  The `races` and `unified` vectors check that immediate damage is unchanged.
  `make test-unified` includes the unified case.
- **ARM.** The T10 replay and the three two-process saves above match their
  host oracles at every checkpoint.
- **Campaign controllers.** See [Validation](VALIDATION.md) for the results
  and the failures reproduced on the unmodified default branch.

## Remaining limitations

- Native N64 bullet behavior, resolution order and miss chance are not traced.
  Timing is the source records under the PC proxy's interpretation.
- Damage and weapon upgrades are fixed at launch.
- Follow-target bullets use launch distance, not homing correction. Two-shot
  weapons resolve both hits together.
- Bounces, splash on moving projectiles, Valkyrie and other actors without
  source profiles, P6/T4 traps and T5 mines remain immediate or unchanged.
- Appear-at-target weapons still deal damage on the attack decision, before
  their source wind-up and impact frame. They are 3–8 updates early, as
  before.
- In P7 and Z6/Z7, a save with more than 10–11 pending impacts is refused
  (SAVE FAILED) until enough of them land. Observed play peaked at 4–6.
- Longer battles from overkill change campaign trajectories, so scripted
  controller routes can change outcome (see the Z7 diagnosis below). No
  canonical Zerg Z7 normal-play win is currently qualified, with or without
  this change.
- ARM evidence covers one probe-entry Terran mission (T10) and skirmish saves,
  not every campaign, a normal-menu campaign entry, or hardware frame rate.

## Z7 controller diagnosis

On `761597d` the scripted Z7 normal-input controller won at host tick
821,565 with impacts disabled and hit its 1,000,000-tick limit with them
enabled. The outcome turns on a controller heuristic, not on a defect in the
impact path:

- `z7_townhall()` in `tests/sc64_z7_campaign_run.c` treats the lowest-numbered
  live Hatchery slot as home. Both runs build the expansion Hatchery at
  (256,1400) by frame 21,000–23,600, and it survives in both.
- In the disabled run the army had already destroyed the enemy Sunken Colony
  in building slot 49, so the expansion reused slot 49, below the original
  Hatchery's slot 57, and became home.
- With impact-time damage that Sunken died later, so the same Hatchery took
  slot 59. Home stayed at the original Hatchery, whose minerals run out;
  drones then walked past the western Sunken Colonies (40 damage every 40
  frames, an immediate weapon) to distant fields, and the economy collapsed
  to one drone and 21 minerals.

Choosing the completed Hatchery with the most nearby minerals removes that
slot-order dependence, but the controller belongs to the Zerg campaign lane
and is not changed here.

On the current default branch the comparison no longer applies. Default
`cfdbe16` itself fails the controller (limit, frame 999,093, 45 enemy
buildings), and this branch with `-DSC64_IMPACT_EVENTS_DISABLED` reproduces
that trace byte for byte. A bisect over default's merges places the change at
PR #161 (`6e1cb67`, loaded-worker depot routing): `ed632dd` still wins at
821,565 and `6e1cb67` does not. Z7 qualification therefore needs the Zerg
lane's controller repair on current default in any case; it is not gated here.
