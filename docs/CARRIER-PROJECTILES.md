# Carrier projectile development

The optional `SC64_P7_PROJECTILES_ENABLED` profile connects the authored
weapon-79 projectile model to P7's game update, ordinary damage handler and
save/load validation. It does **not** enable Carrier combat: child training,
launch orders, native Interceptor flight and IScript attack events still need
integration. There is currently no gameplay caller of the firing entry point.
Generic Interceptor instant damage is explicitly disabled. P7 remains outside
the qualified campaign count, which stays at 22/30.

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

The host game fixture injects shots through the firing API using a distinct
raw73 actor. It checks delayed ordinary damage, saves through flight and
impact, no repeated hit after reload, dead-target and dead-shooter behavior,
unit/building slot reuse, nine malformed-save rejections without live mutation,
pause behavior and 18 map/human-race/bot-count setup/save combinations. The
slot-reuse checks isolate projectile ticks from unrelated campaign combat;
the main continuation check runs full `game_update` calls.

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

The ARM probe passes the unchanged link guards. This does not prove in-flight
ARM SRAM continuation, stack high-water, physical hardware behavior, a P7 win
or near-30-fps Carrier battles. Those gates remain required before promotion.
