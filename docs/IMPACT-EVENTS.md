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
resolves them on the same updates as the uninterrupted run; the fixtures
check this. Fresh-process SRAM qualification has not been rerun.

## Memory

| Product | EWRAM headroom (base → candidate) | IWRAM headroom |
| --- | --- | --- |
| Showcase | 200 → 272 B | 56 B, unchanged |
| Terran | 28,480 → 28,536 B | 536 B, unchanged |
| Zerg Z6 | 28,372 → 28,224 B | 96 B, unchanged |
| Zerg Z7 | 2,256 → 2,108 B | 648 B, unchanged |
| Protoss P7 | 23,828 → 23,680 B | 960 B, unchanged |

These are the four normal 32 MiB ARM link maps, read from `__bss_end` and
`__iwram_end` against the unchanged guards.

- The queue costs 196 B: 32 × 6-byte rows plus 4 bytes of counters.
- Race builds store `Tracer` endpoints as 16-bit values. Tracers are absent
  from RaceSave, and race maps are at most 128 cells per side. That frees
  256 B.
- The extended save record lives in each product's existing route-scratch
  union (asserted), and the load candidate (at most 66 B) is on the stack.
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

Results, mGBA 0.10.5:

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
- **Campaign controllers.** See [Validation](VALIDATION.md) for the results
  and the pre-existing failures reproduced on the unmodified base.

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
- In P7 and Z6/Z7, a save with more than 10–11 pending impacts is refused.
- Longer battles from overkill change campaign trajectories. Controller
  qualifications that passed on base were rerun; ARM replays and fresh-process
  SRAM qualification were not.
