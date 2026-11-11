# Carrier projectile development

## Current status (2026-11-12)

The three layers below (projectiles, hangar training, Interceptor flight) are
compiled into the Protoss ROM, whose P7, P8, P9 and P10 flag vectors include
`SC64_P7_PROJECTILES_ENABLED`, `SC64_P7_CARRIER_TRAINING_ENABLED` and
`SC64_P7_INTERCEPTOR_FLIGHT_ENABLED`. They run for every Carrier-class actor of
a Protoss ROM, not only in P7. The generation descriptions below are the
history and the contract of each layer; where they call a layer "default-off"
or say the canonical ROMs leave it off, that was true of the development
probes, not of the Protoss ROM.

| Parent | Where | What works |
| --- | --- | --- |
| Carrier (raw 72) | Protoss ROM: custom games, P7 to P10 | Paid Interceptor orders, release every 7 source ticks, attack passes, return and dock, rebuild, death of the parent or the target, saves. |
| Gantrithor (raw 82) | Protoss ROM, P10 finale | The same service and rules (same hangar of four, 25 minerals, release cadence). It joins the human side with a full hangar, launches at enemies it faces, rebuilds lost Interceptors on the ordinary "BUILD INTERCEPTOR" command, and its children follow it if it changes owner and die with it. No save field was added: a P10 save written before this change loads with a Gantrithor that has no children. |
| Computer opponents | Protoss ROM, custom games | A Protoss opponent builds a Fleet Beacon, trains Carriers and pays for their Interceptors through the same service ([Skirmish AI](SKIRMISH-AI.md#carriers-protoss-rom)). |
| Everything above | Terran, Zerg Z9/Z10 and Showcase ROMs | Not compiled: see [Carriers in the other ROMs](#carriers-in-the-other-roms). |

How the Gantrithor is a parent: `sc64_p7_carrier_parent_type()` and
`sc64_p7_carrier_parent_raw_of()` in `starcraft/sc64_p7_carrier_training.h` name
the Carrier-class types (type `U_P7_CARRIER` with raw 72; in builds with the P10
roster, `U_P10_GANTRITHOR` with raw 82). Every parent check, queue, release,
death and save validation goes through them, so there is one service and no
mission-only branch; the only mission-specific code is the P10 rescue tick,
which calls `race_p7_carrier_training_owner_changed()` and stocks the hangar
once, when the hero joins the human (`starcraft/sc64_p10_runtime_adapter.h`).
The hangar size is the ordinary four: the authenticated P10 scenario data carries
no Carrier Capacity upgrade rows, so nothing in it says otherwise. That the
Gantrithor starts with a full hangar is an assumption (a placed Carrier starts
loaded); the source data does not say.

Actor slots: Interceptors are Unit rows and count against the 240-slot pool. In
the P10 finale controller run the pool already reaches 240 of 240 at update
35,789 without the Gantrithor's children; with them the peak is 239 of 240.
Nothing stalls on a full pool: Interceptor training and ordinary production
wait for a free slot (`race_p7_carrier_training_slots_available`), and the
mission's source triggers create their units at start-up, before the pool fills.

The optional `SC64_P7_PROJECTILES_ENABLED` generation-41 profile connects the
authored weapon-79 projectile model to P7's game update, ordinary damage
handler and save/load validation. That projectile-only profile does **not**
enable Carrier combat: it has no child training, launch orders, native
Interceptor flight or IScript attack events, and no gameplay caller of the
firing entry point. Generation 43 (below) adds those. Generic Interceptor instant damage is explicitly disabled.

The separate generation-42 `SC64_P7_CARRIER_TRAINING_ENABLED` profile adds
player-commanded Carrier hangar training and persistent hidden child rows. It
does not add Carrier production, launch/flight, Interceptor attacks or enemy
AI auto-queueing, and does not establish full native Carrier behavior. Neither profile qualifies a P7
win or campaign mission; staged campaign progress remains 22/30.

## Build and check

The asset-free checks need no ROM or reference checkout:

```sh
make test-sc64-interceptor-pool-codec test-sc64-interceptor-motion
```

With the existing private P7 inputs prepared:

```sh
make test-sc64-p7-projectile
make protoss-p7-projectile-probe
```

The probe is `build/StarCraft-Protoss-P7-Projectile-Probe-GBA.gba`, title
`SC P7 SHOT41`, code `SC7J`. It is a development profile, not an additional
playable Carrier release; the Protoss ROM now carries this layer together with
the two that follow. The Terran, Zerg and Showcase ROMs leave the gate off. No
assets, generated headers, ROMs or saves belong in Git.

The separate training-stage checks and probe are:

```sh
make test-sc64-p7-carrier-training
make protoss-p7-carrier-training-probe
```

The probe is `build/StarCraft-Protoss-P7-Carrier-Training-Probe-GBA.gba`,
title `SC P7 HANG42`, code `SC7K`. It builds on the generation-41 projectile
profile, but remains a development test profile, not a production or playable
Carrier-combat release. The training-only stage cannot launch, so its save
validator requires an empty projectile pool. The current test fixture creates
a human-owned Carrier because the authenticated mission's starting Carriers
belong to enemy raw owner 4; it does not demonstrate player control of those
starting enemy Carriers.

## Generation 43: Interceptor flight

The `SC64_P7_INTERCEPTOR_FLIGHT_ENABLED` profile builds on generation 42 and
makes Carriers fight:

- **Launch.** A Carrier with an attack target, or an attack-move or idle
  Carrier with an enemy within its 8-tile acquisition range, stops. It then
  releases its healthiest completed inside child (at least half HP) every
  7 source ticks (native `order_Carrier`/`release_fighter`). Out of range,
  an attack-ordered Carrier flies toward its target.
- **Attack passes.** Each launched child runs the authored order model in
  `sc64_interceptor_orders.c`: the launch offset, a 15-tick climb, then
  attack, strafe and return-strafe passes.
- **Firing.** A fire request starts the decoded attack script: the weapon
  event runs one source tick later and the lock clears one tick after that.
  The cooldown is 1 + (rand & 3) − 1. The event launches a moving weapon-79
  projectile through the existing pool; damage arrives on impact.
- **Return.** A child returns when its shields fall below a quarter, when it
  has no target it can reacquire, or when the target leaves the Carrier's
  leash. It docks within 10 source pixels and is repaired at once.
- **Carrier death.** Hangar children die with the Carrier. Launched children
  detach and are removed after 15–45 source ticks.

Run the host fixture and build the probe with:

```sh
make test-sc64-p7-interceptor-flight
make protoss-p7-interceptor-flight-probe
```

The probe is `build/StarCraft-Protoss-P7-Interceptor-Flight-Probe-GBA.gba`,
title `SC P7 FLY43`, code `SC7L`. It is a development profile; the Protoss ROM
carries the same flags. The Terran, Zerg and Showcase ROMs leave them off, and
with the flags off those ROMs are byte-identical to the base revision (checked
again for the Gantrithor and Computer-opponent change: Terran, Zerg Z9, Zerg
Z10 and Showcase build to the same bytes).

Save format: RaceSave stays 16,292 bytes. Launched-child state uses actor
fields the typed codec already stores:

| State | Field |
| --- | --- |
| attack or return order | `order` |
| order state | `path_retry` |
| order timer | `work_timer` |
| waypoint, in runtime pixels | `tx`/`ty` |
| attack-animation phase | `path_timer` |
| weapon cooldown | `cd` |
| detach countdown | `carry` |

The Carrier's release timer uses its `work_timer`. Generation 43 accepts
in-flight projectiles and launched or detached children. Its validator bounds
each of these fields, and it rejects any generation-42 save.

Approximations (deterministic and save-stable):

- Random pass offsets, cooldown jitter and the detach timer hash the saved
  frame counter and slot instead of using the native RNG stream.
- Range checks use centre distances, not unit-box distances.
- Waypoints keep half the source precision.
- The muzzle heading is the current flight direction rather than a turned
  flingy heading.
- Flight uses the stock flyer step without acceleration or turning.
- A docked child is repaired at once instead of gradually.
- Carrier Stop does not recall children explicitly; they come back through
  the leash or when no target remains.
- Interceptors attack through the shared sight, detection and invincibility
  rules, so Arbiter-cloaked units need detection, as in the source.

`make test-sc64-p7-interceptor-flight` passes under ASan/UBSan. It covers:

- out-of-range hold-off and 7-tick release cadence;
- flight within the leash, and damage only through projectiles;
- a mid-flight save/load that restores every flight field and the
  projectile pool;
- return and docking;
- shield retreat;
- hangar rows that cannot be targeted or fire;
- Carrier death detaching children, with removal inside the 15–45 tick
  window;
- 11 malformed-save rejections without live mutation;
- full `game_update` combat, periodic save/load and pause;
- every mission and the 18 skirmish setups.

These are host scenarios, not campaign or ARM playthrough results.
Generation 43 has no fps, hardware or P7-win qualification. Enemy Carriers
still have no AI training, so in the authenticated mission only player
Carriers launch.

## Runtime and save contract

- The pool retains 100 slots and ten-slot growth with native-compatible list
  order. Its 4,816-byte arena occupies `0x06014000` onward in Mode 4 OBJ VRAM.
  Reads and writes use halfwords. The experimental tile renderer uses the
  same memory, so combining the profiles is a compile-time error.
- Positions retain signed source-coordinate Q8 precision. Launch uses the
  integer sprite origin and separately rounded muzzle components. Source
  fixed-point integer conversion rounds negative values down.
- The existing P7 24/30 source clock advances projectiles after unit orders.
  Arrival or expiry creates a same-update damage event; the caller consumes
  and clears it before returning to the save boundary. The next source update
  retires the dying projectile. Damage uses the current engine's integer
  HP/shield/armor and upgrade policy, not complete native combat semantics.
- Unit and building slot reuse clears references before replacement. Dead
  references are also cleared at the completed-update boundary. A projectile
  retains its source-player identity after shooter death.
- Generation 41 adds the 2,103-byte packet and explicit padding to RaceSave:
  16,292 bytes of the 16,368-byte journal payload. Unit/Building remain 56/48
  bytes and the pools remain 240/160. Candidate validation checks packet lists,
  lifecycle fields, raw73 identity, source ownership and live references before
  committing. Other missions and skirmishes require a canonical empty pool.
- Generation 42 retains the 16,292-byte RaceSave and 56/48-byte Unit/Building
  rows. Carrier queue, child training/inside state, parent links, and rank are
  encoded in existing actor fields; candidate validation reuses the unused
  tail of the existing route-save scratch union. The stage adds no save-payload
  or BSS allocation. Training children are actual raw73 Unit rows but stay
  hidden, unselectable, untargetable, non-attacking and absent from sight until
  a separate flight/IScript implementation exists. Each costs 25 minerals,
  each Carrier has four hangar slots (inside + training + paid orders). The
  native five-order queue guard remains, with the four-slot cap limiting the
  currently reachable queue. A child
  completes after 300 source ticks with 30 Q8 HP accumulated per source tick.
  Paid training waits for a free actor slot while reserving capacity for
  authenticated script creations and existing building/Carrier queues.
- In the authenticated mission data, the initial Carriers have raw owner 4
  (enemy); the human force is raw owner 7. Neither owner can improve the
  effective Carrier Capacity upgrade in this mission: the human PUPx entry is
  current/max 0/0 and the enemy's default entry is also 0/0, so the implemented
  capacity is four for both. This stage does not change those upgrade rules.

The generation-41 host game fixture injects shots through the firing API using a distinct
raw73 actor. It checks delayed ordinary damage, saves through flight and
impact, no repeated hit after reload, dead-target and dead-shooter behavior,
unit/building slot reuse, nine malformed-save rejections without live mutation,
pause behavior and 18 map/human-race/bot-count setup/save combinations. The
slot-reuse checks isolate projectile ticks from unrelated campaign combat;
the main continuation check runs full `game_update` calls.

The generation-42 `make test-sc64-p7-carrier-training` fixture passes under
ASan/UBSan. It checks the player command, 25-mineral price, four-child cap,
300-source-tick training, save/load at training and inside boundaries, 15
malformed-save rejections without live actor/pool/runtime/wallet mutation,
slot reservations, cleanup/reuse, hidden-unit damage and sight, paused updates,
mission setup/save checks and all 18 skirmish setups. Enemy source-owner queues
advance only when explicitly supplied by the fixture; no AI ordering is added. The
fixture deliberately spawns a human-owned Carrier because the mission's
starting Carriers are enemy-owned. These are test scenarios, not campaign
playthrough results. Construction-time assertions use isolated source ticks;
the initial clock and pause checks exercise full game updates.

## Reference evidence and limits

Numeric facts come from pinned BWAPI
`d727fed68558c506163048ea889131d8cbb33915` and behavior checks use pinned OpenBW
`4b046d5f65302b10cb0a745f0fecd37ec85b20a8`. Public motion code and numeric
tables are project-authored; no reference implementation or game data is
included. Raw73 has 40 HP/40 shields, zero supply, 25 mineral cost and a
300-source-frame training time. Pulse Cannon is weapon 79, damage 6+1 upgrade.
Its table cooldown of one is not the Interceptor IScript firing cadence.

Private ASan reference fixtures compare all 256 launch headings and fractional
origin rounding against actual native bullet creation, then compare C motion
and codec continuation for near impact, endpoint, bounded expiry, target
removal and four reused-bullet headings. Those worlds and decoded-script
fixtures are synthetic; they do not establish loaded-campaign or whole-world
native parity. Native UBSan has diagnostics in the reference intrusive-list
code; only the authored C and game host suites claim clean ASan/UBSan results.

The generation-41 ARM projectile probe passes the unchanged link guards. This
does not prove in-flight ARM SRAM continuation, stack high-water, physical
hardware behavior, a P7 win or near-30-fps Carrier battles. Generation 42
training originally linked with IWRAM/BSS ends `0x03004a30` / `0x0203daf4`.
The later shared weapon-effect pool expands BSS to `0x0203dcd4`, retaining
the same IWRAM end and unchanged guards. It has no production, native Carrier-combat, hardware, fps or
P7-win qualification claim. Those gates remain required before promotion.

## Carriers in the other ROMs

Measured on 2026-11-12 at the base of the Gantrithor change; nothing here is
implemented. A Protoss player on the Terran, Zerg or Showcase ROM gets no
working Carrier. What stops it, and the smallest sound way to start:

| Fact | Terran | Zerg Z9 / Z10 | Showcase | Protoss P10 (has it) |
| --- | --- | --- | --- | --- |
| Carrier (raw 72) as a unit type | none (raw 72 maps to no type) | none | `U_P7_CARRIER`, producer 255 (not trainable) | trainable at a Stargate |
| Interceptor (raw 73) type | none | none | none | `U_P7_INTERCEPTOR` |
| P7 flags compiled | none | none | `SC64_P7_ACTORS`, battery, observer; no projectiles, training or flight | all |
| Generated fact tables (`SC64_FACT_TABLES`) | no | no | no | yes (per-vector header) |
| Persistent unit IDs now (six-bit field, 63 max) | 44 | 42 | 49 | 53 |
| `RaceSave` bytes / free of 16,368 | 15,856 / 512 | 16,296 / 72 | 14,228 / 2,140 | 16,332 / 36 |
| Unit row | packed 38 bytes, all fields | packed 35 bytes (`path_timer` <= 24, `path_retry` <= 12) | typed 28 bytes (`flags` 14 bits) | typed 28 bytes |
| OBJ VRAM terrain cache | all 16,384 bytes (68 rows) | none | all 16,384 bytes (68 rows) | 11,568 bytes (48 rows) |
| IWRAM end (limit `0x03005000`) | `0x030044a8` | `0x03004850` | `0x03004a68` | `0x03004818` |
| BSS end (limit `0x0203e000`) | `0x0203748c` | `0x0203b1ac` | `0x020372d0` | `0x02039758` |

The blockers, in order:

1. **Flags and companion sources.** The services need `SC64_P7_ACTORS`
   (types and facts), `SC64_P7_PROJECTILES_ENABLED` (Interceptor type, arena,
   save field), `SC64_P7_CARRIER_TRAINING_ENABLED`,
   `SC64_P7_INTERCEPTOR_FLIGHT_ENABLED` and the three companion translation units
   (`sc64_interceptor_motion.c`, `sc64_interceptor_pool_codec.c`,
   `sc64_interceptor_orders.c`). Showcase has the first only. The Terran and Zerg
   products have none of the P7 flags.
2. **Persistent IDs.** `U_P7_*` and `U_P7_INTERCEPTOR` are inserted in the middle
   of the unit enumeration (`race_rules.h`). In the other products that would
   renumber every later type and break their saves. New types must be appended
   (the way `U_SCARAB` and the shared Shuttle are). Headroom exists: the unit
   type field is six bits and the products use 42 to 49 IDs.
3. **Generated fact tables.** `SC64_FACT_TABLES` headers come from
   `tools/sc64_fact_tables_gen.c` per Protoss flag vector only. Each other
   vector needs its own regenerated header with the Carrier and Interceptor
   rows; PR #212 found that the generator's gate list and signatures must change
   with it.
4. **Save record.** Interceptor state already fits the existing unit rows: the
   Terran and Zerg packed rows store `flags` (16 bits), `build` (10 bits),
   `work_timer`, `path_retry`, `path_timer` and `cd` losslessly (Zerg's narrower
   `path_timer`/`path_retry` still hold the Interceptor's 0 to 2 and 0 to 5), and
   Showcase's typed row is the P10 row. What does not fit is the projectile pool
   packet, 2,103 bytes plus one padding byte: Terran would need 17,960 bytes
   (1,592 over), Zerg 18,400 (2,032 over). Showcase would land at 16,332, the same
   36 free bytes P10 has, so it needs no format change.
5. **OBJ VRAM.** The arena takes 4,816 bytes of the 16,384 (`sc64_obj_vram.h`).
   Terran and Showcase use all of it for the VRAM terrain cache; the arena would
   leave 11,568 bytes, 48 rows instead of 68, the figure the P10 ROM already has.
   Zerg has no VRAM cache, so the arena fits in unused memory there. The
   experimental tile renderer owns the whole area and is excluded.
6. **Code memory.** Not measured for these products in this pass. The change that
   gave the Gantrithor its hangar linked P10 with an unchanged IWRAM end
   (`0x03004818`) and BSS end (`0x02039758`) and 1,428 more bytes of ROM text. The
   experimental #210 link of Showcase with everything enabled ended at IWRAM
   `0x03004d18`, EWRAM `0x0203abf4`, inside the guards.

Smallest sound way, by product:

- **Showcase first.** Add the P7 services to its flag vector (projectiles,
  training, flight, fact tables and rect tables, the three companion sources).
  Its typed rows and save layout already equal P10's, so no codec change and no
  new save generation: the record grows from 14,228 to 16,332 bytes. Accept the
  20 fewer terrain-cache rows (or measure a cheaper cache), and keep
  `U_P7_INTERCEPTOR` out of the middle of its enumeration if any Showcase save
  records type IDs (append it).
- **Terran and Zerg, custom games only.** A Protoss player on those ROMs only
  exists in custom games, where the campaign-only regions of `RaceSave` are zero.
  Append Carrier and Interceptor IDs, add the flags and fact tables, and give
  the custom-game record a new generation tag whose projectile packet is
  variable length (active rows only) and whose campaign-only regions are
  omitted. The arithmetic, as upper bounds: Terran has 512 free bytes plus at
  most the 2,280 between its `campaign` member (offset 13,536) and the Yamato
  shot rows (15,816), part of which (the invincibility bits, for one) is live in a
  custom game; the packet needs 1,592 more than it has, so at least 1,592 of those
  bytes must prove omittable member by member, or a shot cap must make up the
  difference. Zerg has 72 free plus at most the 360 from `campaign` (offset 15,936)
  to the end of the record (Backwater and the 180-byte `z5` union), 432 bytes, about
  20 rows of the packet. Both therefore need a hard cap on live shots that the
  packet's budget can hold, with the weapon event refusing to fire when the
  budget is full (the pool already rejects a shot when it is full), a candidate
  check that rejects a larger packet, and the old generation loading unchanged.
- **Do not** edit the existing legacy reader's commit path: PR #212 found its
  mission rebinds are not atomic and fenced predecessor loads off instead. A new
  generation should preflight the whole candidate and leave the legacy path
  alone. Competing designs in #212 and #210 (a shared "namespace" codec versus a
  separate typed arena) should be settled before either product adopts one.

That is also the order the work has risk: Showcase needs no new format, Terran
needs a variable-length packet, Zerg needs the packet and a low shot cap.
