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

Measured on 2026-11-12 at the base of the Gantrithor change. No additional
ROM currently enables these services. A standalone [compact packet](#compact-packet-foundation)
is available for future Terran and Zerg integration. A Protoss player on
the Terran, Zerg or Showcase ROM gets no working Carrier. What stops it,
and the smallest sound way to start:

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

Showcase can reuse the fixed packet layout only if its exact worst-case
journal fits with the other advertised features. It still needs a way to
authenticate the changed record length and safely load predecessor saves
with an empty projectile pool. Terran needs a variable-length packet;
Zerg needs the packet and a verified bound for simultaneous shots.

### Compact packet foundation

The asset-free `sc64_compact_*` API is separate from the existing fixed
2,103-byte projectile packet and is not wired into RaceSave. The
`SC64_INTERCEPTOR_COMPACT_CODEC` compile flag enables it for the host fixture;
no production profile sets this flag. An enclosing save generation specifies
a fixed simultaneous shot cap (0 to 100) and passes the exact packet length. The packet starts with two bytes (allocated groups of ten,
active count), followed by all allocated ordinals in active-list order then
free-list order, seven low-bit-first bits per ordinal. The active rows follow
in list order, 154 bits each: the legacy row fields up through the pending
damage bit. The list reconstructs both heads, tails and links; the omitted
reserved bits and final byte padding must be zero.

The capped acquire preserves ten-slot growth and the original list operations.
Retire resets the free slot's fields to known values without changing its
identity or list location. This is safe for the game path that initializes all
fields on the next acquisition. Compact encoding refuses old arenas with
noncanonical free fields; it never silently discards stale data. The future
game adapter must use compact retire in place of both legacy retirement and
the subsequent write of the retired row. An exact compact roundtrip restores
every arena halfword for any conforming pool.

With cap `C`, the largest allocated size is
`A = min(100, 10 * (floor(C / 10) + 1))` for nonzero caps, because early
growth can happen when the next shot uses the last existing free slot. For
cap zero, `A = 0`. The exact worst case is
`2 + ceil((7 * A + 154 * C) / 8)` bytes. Examples, before enclosing size tags,
alignment and any other fields:

| Active cap | Worst case |
| ---: | ---: |
| 0 | 2 bytes |
| 3 | 69 bytes |
| 20 | 414 bytes |
| 21 | 433 bytes |
| 25 | 510 bytes |
| 100 | 2,015 bytes |

Using the measured space above, Terran's existing 512 free bytes could fit
the 25-shot packet itself; Zerg's theoretical 432 bytes would fit the 20-shot
packet itself only if the enclosing layout frees enough of that space. Zerg's
unchanged 72-byte margin fits three shots before enclosing overhead. These
numbers describe storage bounds, not recommended caps or combat
qualification. Showcase can use the existing fixed packet if its measured
layout still fits.

Detached preflight rejects malformed lengths, caps, slot permutations, row
domains and padding. It can check the destination for overlap and run a
candidate-only world predicate over active rows after the structural checks.
A rejected packet leaves the destination untouched. The asset-free sanitizer
suite checks every cap, compares all arena words across saves, compares the
next allocation sequence, and exercises malformed packet rejection.

Future integration still needs a save generation and exact byte budget,
candidate actor and projectile validation, persistent unit IDs, generated
facts, and per-product memory and gameplay checks. No profile, runtime
projectile cap, or existing save format changes with this foundation.

### Showcase Carrier probe

With the prepared Showcase assets, run `make showcase-carrier-probe` and
`make test-sc64-showcase-carrier`. The isolated cartridge is
`StarCraft-Showcase-Carrier-Probe-GBA.gba` (game code `SCCP`).
It runs the same shared Carrier and Interceptor services as the Protoss build.
The regular Showcase target and its save format stay unchanged.

The probe retains every existing Showcase actor ID and appends raw 73 at
type 49. The custom-game Carrier uses the existing Stargate production rule
and Fleet Beacon requirement; the ordinary selected-unit command pays for
an Interceptor. The probe generates its own fact and rectangle tables from
the matching actor vector.

The probe's save tag distinguishes its projectile extension while retaining the
predecessor's inner state version. The largest packet has 95 simultaneous
Interceptor shots. The pool refuses a launch while all 95 slots are active. The
1,924-byte extension includes the compact packet's length and zero padding.
It raises the 14,228-byte predecessor base, including both Yamato rows, to
16,152 bytes. Adding the 8-byte impact header, 32 six-byte impacts and 16-byte
transport trailer gives exactly 16,368 bytes, the SRAM slot's payload limit.
The saved shape cannot reduce the impact queue or truncate active Yamato rows.

Old journals load through the detached selector, which reads their actors,
impacts and Yamato rows at the original offsets and supplies an empty
Interceptor pool. The test uses a real predecessor writer and fresh readers.
It trains a paid Carrier and Interceptor, observes flight through the real
firing path, saves all three queues at once and checks that a checksum-correct
invalid extension falls back to the intact predecessor. With that
predecessor removed, the invalid record changes neither live state nor SRAM.

A bounded mGBA run drove the probe with the pinned physical-button runner
(`4f655601…`), without writing guest memory. A blank guest reached frame 241
of an authored-map custom game with a Protoss human and one Terran bot.
Separate fresh guests loaded the predecessor and full-journal **host
fixtures** with title L, then saved with START/B. Both ARM writes used the
16,368-byte payload. The predecessor's original bank stayed byte-identical;
the full journal's re-encoded 1,919-byte projectile packet matched its input
byte for byte. This is ARM load/write evidence for constructed fixtures.
This fixture does not exercise paid production or combat through ordinary
controls; the separate replay below does. The full Showcase matrix, hardware
stack high-water and battle timing remain open.

A separate ordinary-input ARM run started a blank authored-map custom game
as Protoss against one Terran bot. Probes gathered minerals and gas and
completed the Assimilator, Pylon and Forge. The guest saved at frame 3,004
with 476 minerals and 112 gas. From that save, the Gateway completed by
frame 4,151, Cybernetics Core by 5,369 and Stargate by 6,754. A further
normal-origin continuation completed the Fleet Beacon at (184, 120) by
frame 8,800, with 316 minerals and 74 gas. START/B produced a 16,128-byte
Carrier journal (tag `0x764c5058`); a fresh title-L load restored the same
frame, wallet, supply and observed unit and building rows. This ordinary
tech path is separate from the synthetic full-journal smoke. Waiting for
Carrier gas did not reach a purchase before the match left play. The next
ordinary-input run needs an earlier gas and defense plan to pay for the
Carrier and Interceptor, attack, and save during flight.

A defended ordinary-input run against one Zerg bot completed the next
checkpoint on probe ROM SHA-256 `2a8a3849…`. From a blank custom game,
ordinary buttons built the tech chain and kept Probes gathering gas. A
Stargate queued a Carrier at frame 9,753 for exactly 350 minerals and 250
gas. The Carrier completed at 12,386. Its selected-unit command paid 25
minerals at frame 12,431; the Interceptor completed at 12,811 and was outside
the hangar by 12,864.

START/B at 12,888 wrote a 16,128-byte journal with one moving projectile in
a 30-byte compact packet. The packet named the player's Interceptor (slot
12) and a Hydralisk (slot 43). A fresh title-L load restored frame, wallet,
supply and observed actors, and a guest re-save reproduced the entire
1,924-byte compact extension. After 80 more updates the observed
Hydralisk had 68 HP, down from 80 at the save point. This checks paid
production, attack and a live projectile across a real ARM save. It does not
attribute every hit to the Interceptor or qualify return and docking, other
matchups, hardware stack use, or battle timing.

The probe was also composed with release `cb210bc`. The standard Showcase
ROM rebuilt byte for byte (`8d0d9f9464629ac3c…`); the optional ROM
(`9d5dca67d8387a12…`) linked within the unchanged EWRAM and IWRAM limits
(ends `0x0203aefc` and `0x03004d68`). A fresh ARM process read the earlier
frame-12,888 active-flight journal and re-saved the 1,924-byte extension
unchanged. After another fresh title-L load, ordinary buttons selected the
Carrier and moved it away from the fight. Its Interceptor returned to the
same Carrier and docked by frame 13,046 with 40 HP and 40 shields. A third
fresh title-L load restored the observed game and docked child exactly.
The private read-only observation receipt is pinned by SHA-256
`546f0954a418b2a15568ba5da004d0ba6e4aa09f25a1a08a27493e812d4e8baa`.
This is one emulator matchup with ordinary controls; the regular builds do
not switch to the new tag and the physical GBA frame budget remains open.

### Terran custom journal study

The Terran storage study is separate from the Showcase probe. With prepared
Terran assets, run:

```sh
make -f Makefile -f tests/sc64-terran-carrier-envelope/Makefile test-sc64-terran-carrier-envelope
```

The host-only study wraps a current Terran custom save under a distinct
checksum-verified SRAM tag. Its worst case is **15,929 of 16,368 bytes**:

| Region | Bytes | Treatment |
| --- | ---: | --- |
| Prefix through Backwater state | 13,618 | Exact bytes: 192 unit and 96 building slots, full fog, wallets, campaign state and Backwater state |
| Invincibility flags | 36 | Packs all 288 binary unit/building flags |
| Packet and tail lengths | 4 | Two little-endian lengths |
| Full compact Interceptor packet | 2,015 | 100 active projectiles |
| Existing Yamato, impact and transport tail | 256 | Both active Yamatos, all 32 impacts and transport, in the existing wire order |

The 1,910 bytes between the flags and the Yamato tail are T3/late-mission
storage and padding. The writer refuses a custom save if any omitted byte
is nonzero, and refuses nonbinary invincibility flags. No fog cells are
omitted and no engine capacity changes. A separate ARM compile confirmed
the same prefix, omitted range and total size.

The host fixture saturates the three queues on both custom maps and
reconstructs each original 16,072-byte save byte for byte. It writes and
selects the study envelope through the ordinary SRAM journal routines, then
runs the real Terran game's detached candidate preflight on the reconstructed
save. Both a malformed compact packet and a structurally valid envelope with
an invalid map fall back to the preserved ordinary predecessor. The fixture
also round-trips saves after 90 gameplay updates in all 18 map/human-race/bot
configurations, and checks that truncation, excess length and insufficient
output capacity fail before changing the destination.

This fixture proves the storage and prepared selection bounds; the separate
runtime probe below connects them to paid Carrier gameplay.

### Terran custom Carrier runtime probe

The opt-in Terran build attaches shared paid Carrier production, Interceptor
training and flight, and the projectile arena to its custom game roster.
The original Terran actor IDs stay fixed; Carrier and Interceptor append
as 44 and 45. The existing source-derived Terran art bank already has both
actors and their directional frames. The probe also enables the shared
Protoss bot planner. Campaigns keep the ordinary writer and save layout.

```sh
make -f Makefile -f tests/sc64-terran-carrier-runtime/Makefile \
  test-sc64-terran-carrier-runtime build/StarCraft-Terran-Carrier-Probe.gba
```

New custom records use the distinct authenticated envelope above. The probe
can select ordinary or compact banks. It expands the winning record in
place in the existing save scratch, keeps the compact packet detached,
checks the hangar and actor references in the candidate, and restores the
projectile arena only after the ordinary preflight succeeds. The first
upgrade writes to the other bank, retaining the ordinary predecessor.
Neither the SRAM ceilings nor the linker memory guards change.

With prepared Terran assets, the sanitizer fixture paid for a Carrier
through the real Stargate queue, bought an Interceptor through the Carrier
menu, and saved a live shot through the game's SRAM API. Loading restored
every arena word and the hit reduced its target's HP. The fixture uses
placed prerequisite buildings and a placed target. It also verifies that a
malformed compact successor or a valid packet with a missing shooter falls
back to its ordinary predecessor, and that a Terran campaign save in the probe still loads under the ordinary tag.
When composed with release `cb210bc` as `e1058ca`, both HOST fixtures passed
and the ARM link left 10,456 bytes before the EWRAM guard and 1,544 bytes
before the IWRAM guard. Built with the probe flags off, the packaged Terran
ROM matches release `cb210bc` byte for byte
(SHA-256 `a7ce64dadab8b334bbc2f6ccdbbc0aeff06378d2f44d6ecd39702ade2825aa59`).

The composed ARM probe
(`eedcfd0e8fab20e566c8089f730fdc0986f73096c6c5410e95476deddfaeffdc`)
also passed an ordinary-input authored-map custom match against one Zerg
bot. A blank journal started the Protoss economy. The player paid 350
minerals and 250 gas for a Carrier at frame 9,753 and 25 minerals for an
Interceptor at frame 12,431. Ordinary controls launched the Interceptor
at frame 12,870. A START/B save at frame 12,891 wrote a 13,704-byte
custom record with one moving shot in a 30-byte packet, linked to the
player's Interceptor (slot 12) and a Hydralisk (slot 33). Fresh title-L
continuation restored the frame, wallet, supply and observed actor rows.
An immediate guest re-save reproduced the entire payload byte for
byte. After 80 more updates the Hydralisk had 56 HP, down from 80 at
the save; the observation does not isolate the sources of damage.

A [second input receipt](../tools/sc64_terran_carrier_dock_receipt.py)
resumes that same active-shot save, then uses the ordinary MOVE command
to withdraw the Carrier toward (101, 134) at frame 12,945. Interceptor
slot 12 was already on its return leg at the start of this sequence.
It docks with Carrier slot 5 by frame 12,972 and returns to full 40 HP
and 40 shields. The docked custom save has a 13,685-byte payload and no
active projectiles. Fresh title-L continuation restores the docked actor,
and the immediate re-save reproduces the entire payload byte for byte.

The memory guards and these receipts do not establish physical stack
use, timing, or coverage of the other custom and campaign configurations.
Keep the probe disabled in the standard build.

### Shared release selector

The Carrier release candidate is opt-in. From a tree with the prepared
inputs, use the same selector for each supported product:

```sh
make -f Makefile -f mk/carrier-release/Makefile carrier-release-plan
make -f Makefile -f mk/carrier-release/Makefile test-carrier-release
make -f Makefile -f mk/carrier-release/Makefile carrier-release
```

Set `SC64_CARRIER_RELEASE_PROFILE` to `terran`, `zerg-z10`, `showcase`
or `protoss-p10` to run only that row; the default for these explicit
targets is `all`. The first three write their existing preview filenames.
Protoss uses the regular P10 artifact and the existing Gantrithor test.
The ordinary targets and their feature flags remain unchanged.

The single `SC64_CARRIER_RELEASE` compile capability is applied through
`SC64_CARRIER_RELEASE_FLAGS`, which includes the selector header in every
translation unit. It infers the envelope from the canonical product flags
and selects the shared Carrier orders, training, flight and projectile
codec. It also selects the Showcase rectangle facts. The existing profile
guards still require the appropriate actor, asset and journal vector.
An unsupported or ambiguous product, or combining the selector with a
direct preview gate, is a compile error. On P10 the capability adds no
service flags, actor IDs, codec switches or journal changes.

| Product | Existing actor identities | Journal written by the candidate |
| --- | --- | --- |
| Terran T10 | Carrier 44 and Interceptor 45 append after the ordinary roster | Custom: `0x764d5442`, at most 15,929 bytes, 100 projectile slots and 32 impacts. Campaign: ordinary tag and layout. |
| Zerg Z10 | Keeps Carrier 45; appends Interceptor 49 | Custom: `0x564e5a61`, at most 16,285 bytes, 100 projectile slots and 32 impacts. Campaign: ordinary tag, layout and impact save limit. |
| Showcase | Keeps Carrier 47; appends Interceptor 49 | Custom: `0x764c5058`, at most 16,368 bytes, 95 projectile slots, all 32 impacts and both Yamatos. Exhibit scenes still do not load or save a match. |
| Protoss P10 | Existing Carrier, Interceptor and Gantrithor identities | Existing P10 payload and accepted predecessor tags. |

The new readers accept each product's current ordinary SRAM tag and its
already supported worker-cargo predecessor. An ordinary save has no
Interceptor packet or persistent child, so candidate preflight supplies an
empty arena. The first START/B write after loading a predecessor uses the
other bank and retains the source record. A checksum-valid compact record
still has to pass detached actor and projectile preflight before title L
can publish it. Each reader accepts only its own product's tags; the
three new formats are not interchange formats. Terran and Z10 campaign
records still carry the ordinary tag and mission data, even in a
Carrier-enabled build.

A future regular build must keep accepting its current preview tag and
preserve its existing actor IDs and inner version. Do not rebrand the
compact bytes with the ordinary tag; an older build would interpret those
bytes as a different layout. Once both banks have been rotated to the
compact format, a pre-Carrier build cannot continue that custom match.
Even with an older bank left, the old writer can refuse to overwrite the
recognized-as-committed but unsupported newer bank. Back up the regular
save before trying a candidate. Keep preview and standard filenames
separate during qualification.

Before switching a standard target, add the shared flags and required
Interceptor orders source to its normal compiler invocations, including
the Showcase fact generator where applicable. Keep the current probe
reader logic active as the regular reader rather than writing a one-time
conversion. Leave P10 on its own current build vector. Compare a flag-off
build to the shipped ROM. With the selector on, use the following matrix
on the exact guarded ARM candidate before changing defaults:

- For each new product: blank-journal paid match, active-shot and docked
  START/B saves, fresh title-L continuation and an immediate re-save;
  then repeat with a preview-authored save and compare the normalized
  payload and scene or campaign context.
- For each new product and both regular predecessor generations: test
  title-L load, a first migrated write that leaves its original bank
  intact, and fallback to it when a newer compact record is checksum-valid
  but semantically invalid. Reject foreign product tags and malformed
  records without publishing state or changing SRAM.
- Terran and Z10: run the full campaign startup and save/load matrix
  through the ordinary frontend, confirm the campaign tag and actor IDs,
  and run both custom maps and all lobby configurations.
- Showcase: check exhibit navigation and its save/load fence, plus both
  custom maps. Protoss: verify the ordinary binary and existing paid
  Carrier and Gantrithor behavior and continuation.
- Measure guarded memory, stack high-water and dense battle timing on the
  selected candidates. The selector itself does not qualify those gates.

The asset-free selector test checks the actual Make profile flags and
dispatch. Its prepared HOST target runs the existing per-product tests;
neither check constitutes the full ARM activation matrix above.

On release `bb348edd`, all three selector-on ARM links passed the
ordinary memory guards and their packaged ROMs matched the direct
preview flags byte for byte. The Showcase fact header also matched
across the two separate builds. The regular Protoss P10 fact-generation
and ARM commands were identical inside and outside the selector target.

| Product | Identical packaged SHA-256 | IWRAM end | EWRAM BSS end |
| --- | --- | --- | --- |
| Terran | `965ceacbcd5059230a90c8a0dc49c2ced6d19d95ebb08eaa493e8608c9d984d6` | `0x030049f8` | `0x0203b728` |
| Z10 | `2719456571efc210638a832bcca8b069078a39475075e97a4193e63df66b58ed` | `0x03004d90` | `0x0203a5d8` |
| Showcase | `9d5dca67d8387a12f98c370d8cb43734be75a56f2586c8cc2282ad28e210139e` | `0x03004d68` | `0x0203aefc` |

### Campaign and lobby HOST matrix

Run the prepared-input checks sequentially:

```sh
make -j1 -f Makefile -f mk/carrier-release/Makefile test-carrier-release-matrix
```

The optional `SC64_CARRIER_RELEASE_PROFILE` setting limits the matrix to
one product. The Terran and Z10 fixtures open each of their ten missions
from the ordinary menu, take 30 idle updates, and save with START/B.
They reset the game in the same HOST process, load with title L, compare
the world digest and frame, then repeat START/B and compare the entire
journal payload. The ordinary and selector-on fixtures produce the same
ten length-prefixed payloads; they never write the custom Carrier tag in
the campaign. SHA-256 of the combined HOST payloads:

| Product | Ordinary campaign tag | SHA-256 |
| --- | --- | --- |
| Terran T10 | `76435442` | `fedd2755ed68978b4bd21f6f46ada27ff7ca49975e2bd0fc6b5f1630036a39ac` |
| Zerg Z10 | `56435a61` | `17f8d66cba460b8c29e573b4deefd769096eb355f0170f4310ab09be90ac5e4b` |

The ordinary lobby fixture selects both maps, each human race, and all
named enemy race combinations for one, two or three opponents (234 starts
per product), then exercises separate dense and sparse save/restart cases.
The Showcase suite additionally navigates all eight scenes in the standard
and selector-on profiles. An exhibit refuses both save and load; later,
the ordinary frontend still loads the independent custom match. The P10
matrix row runs the existing Gantrithor test with its unchanged flags.

These checks use actual engine input handling and HOST SRAM under
ASan/UBSan. They do not prove a mission win, another process restoring a
guest SRAM image, longer skirmishes, late-campaign saves, or GBA speed.
The ARM and hardware gates above still apply to regular activation.

### Pinned ARM release gate

`tools/sc64_carrier_release_physical.py` provides a single bounded case
per invocation for the exact selector-equivalent Terran, Z10 and
Showcase ROMs listed above. Pass their matching direct-gate debug ELFs
and the pinned port-free physical observer. Use `--plan` first: it checks
the three hashes, frontend and actor layouts and (for a campaign) the
compiled campaign catalog. It neither launches an emulator nor opens the
shared native lock. The command reads the existing inputs in place.

```sh
python3 tools/sc64_carrier_release_physical.py --profile terran \
  --case campaign --chapter 9 --rom "$TERRAN_ROM" --elf "$TERRAN_ELF" \
  --runner "$PHYSICAL_RUNNER" --plan
```

With the native lane free, replace `--plan` with `--output` and a new
directory name. The run takes the existing exclusive lock without
waiting. Each boot is capped at 512 physical updates; the
default wall deadline is 180 seconds. It produces a small JSON receipt
and the test's SRAM files. It makes no guest memory writes. Only
`"status": "passed"` in the receipt qualifies as a completed case.

| Case | Product(s) | Ordinary input |
| --- | --- | --- |
| `campaign` | Terran and Z10 | Open chapter 1–10 in menu order; the default is chapter 10 (IDs 13 / 21). Begin play, clear any active opening dialogue with A, and save after 30 advancing updates. |
| `outpost-mixed`, `blood-bath-mixed` | All three | Protoss human with named Terran, Zerg and Protoss opponents. |
| `outpost-sparse`, `blood-bath-sparse` | All three | Zerg human with Protoss, Closed and Random on the three rows. |
| `migration` | All three | Use the matched regular release to save Blood Bath as human Zerg with Protoss, Closed and Terran CPU rows. Use the candidate to load, advance, save in its compact format, and load the new bank on another boot. |

For skirmish cuts the fixture advances 120 updates before START/B. Both
case types compare the resumed world against an uninterrupted 60-update
reference. Fresh title L must restore the watched world before any live
update. An immediate re-save must use the same ordinary campaign or
compact custom tag, leave the original bank byte-identical and write the
same checksummed payload in the other bank. Skirmishes also use R from
pause after the continuation; the restart must preserve the chosen map,
resolved races, seat mask and starting halls while resetting the frame.

The initial 14 cases passed on the pinned ARM pair for each product. Both late
missions saved at frame 31 and matched their uninterrupted world at
frame 91. The 12 skirmishes saved at frame 121, matched the uninterrupted
world at frame 181, then restarted on the same map with the same resolved
seats and halls. Every immediate re-save reproduced the complete payload
while preserving the original bank.

| ARM case | Passed | Journal tag | Payload bytes |
| --- | ---: | --- | ---: |
| Final Terran mission | 1 | `0x76435442` | 15,858 |
| Final Z10 mission | 1 | `0x56435a61` | 16,312 |
| Terran custom | 4 | `0x764d5442` | 13,676 |
| Z10 custom | 4 | `0x564e5a61` | 14,072 |
| Showcase custom | 4 | `0x764c5058` | 16,128 |

The local index of all 14 receipt hashes has SHA-256
`e45d6b0a11d38facc72d5a8d24c1c9cef402a8da265baea45d288f0376dcd987`.
These bounded cases qualify the sampled ARM boots, saves and continuations.
They do not cover the other campaign chapters, end-to-end wins, full
battles, physical stack use or GBA SP speed.

The extended plans cover Terran chapter IDs 4–13 and Z10 chapter IDs
0, 1 and 14–21; their order must match each pinned ROM's compiled catalog.
The migration case uses the pinned ROM and matching ELF from the regular
November 16 Transport1 package (source `cb210bc`) to produce a fresh save.
It reads those files in place and never takes a player's SRAM as input.

```sh
python3 tools/sc64_carrier_release_physical.py --profile terran \
  --case migration --rom "$TERRAN_ROM" --elf "$TERRAN_ELF" \
  --stock-rom "$REGULAR_TERRAN_ROM" --stock-elf "$REGULAR_TERRAN_ELF" \
  --runner "$PHYSICAL_RUNNER" --plan
```

| Product | Regular tag in bank 0 | Compact tag in bank 1 |
| --- | --- | --- |
| Terran | `0x76435442` | `0x764d5442` |
| Z10 | `0x56435a61` | `0x564e5a61` |
| Showcase | `0x76435058` | `0x764c5058` |

The case takes three boots. The first must produce a valid regular-tag
sequence 1 journal in an erased image. The candidate must restore the
watched world, advance 60 updates, and commit a compact-tag sequence 2
record while leaving the entire first bank intact. The third boot must
restore the newer world without changing either bank: the 60-update
gap catches a silent fallback to the preceding bank. These extended
plans and checksum contracts were verified offline. No ARM result from
these additional cases is counted in the 14 passes above. The selected
regular writer represents one save generation and one opening cut per
product.
