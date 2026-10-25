# Carrier projectile development

The optional `SC64_P7_PROJECTILES_ENABLED` generation-41 profile connects the
authored weapon-79 projectile model to P7's game update, ordinary damage
handler and save/load validation. That projectile-only profile does **not**
enable Carrier combat: it has no child training, launch orders, native
Interceptor flight or IScript attack events, and no gameplay caller of the
firing entry point. Generic Interceptor instant damage is explicitly disabled.

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
playable Carrier release. Canonical race ROMs and the filming ROM leave this
gate off. No assets, generated headers, ROMs or saves belong in Git.

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
training links with IWRAM/BSS ends `0x03004a30` / `0x0203daf4`, within the
unchanged guards. It has no production, native Carrier-combat, hardware, fps or
P7-win qualification claim. Those gates remain required before promotion.
