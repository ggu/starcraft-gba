# Z8 candidate profile (2026-10-29, in progress)

Zerg mission 8, "Eye for an Eye", now runs in an explicit `zerg-z8` candidate:
the Z1–Z7 candidate plus Z8 as the eighth campaign entry, with code `SCZ8` and
generation-46 saves. The canonical `zerg` alias and the `zerg-z7` ROM are
unchanged; a rebuilt `zerg-z7` ROM is byte-identical to its base
(`33664d31…`). Z8 has no ordinary-input win yet, so this is not a mission
qualification.

## Source

The private descriptor was regenerated with `tools/sc64_z8_pack.py pack` from
the user's ROM and the retained scenario. It verifies with the pinned
fingerprint `f80b5f9e9419c324a6b86b9a11bbe3dab0ec545d20817fb0f26509e82c85a986`.
`tools/sc64_z8_program_pack.py` renders it into ignored ROM tables at
`build/generated/z8/sc64_z8_program_generated.h`:
- 39 triggers, 55 conditions, 122 actions and 64 locations;
- active owners 1–3, with forces 2, 3 and 1;
- the two linked Nydus pairs at their spawn centres.

The program, as decoded from the authenticated rows:
- **Victory:** both Protoss Nexuses are destroyed (T24).
- **Defeat:** the human has no units (T0), Kerrigan dies (T1), or a hero Dark
  Templar reaches one of three hive locations (T15–T17).
- **Timed raids:** elapsed-time triggers T13 and T25–T38 release Dark Templar
  raids (T10–T12, T19–T21). Each raid creates the hero and runs `MvTe` toward
  a hive.
- **Towns:** `Zer8` (source slot 2) and `Ze8T` (slot 1) start the two Protoss
  towns, each with an 8,000 mineral and gas wallet.

## Binding

`starcraft/sc64_z8_runtime_adapter.h` connects the merged scheduler, world
adapter and game bridge to the live pools:
- **Owners:** source slot 3 is the human, slots 1 and 2 are the enemy, and
  slots 5 and 11 are neutral.
- **Provenance:** every spawned actor keeps its source slot, and products
  inherit their producer's slot.
- **Nydus:** links bind through `race_nydus_set_source`, and saves resolve
  them through `SC64_NYDUS_SOURCE_FOR_MISSION`.

Authored parts, all documented in the code:
- Transmission lasts 4,000 ms.
- CenterView snaps the camera.
- The Kill census counts destroyed starting Nexuses.
- The AI backend (`starcraft/sc64_z8_ai.h`) queues normally paid Gateway,
  Stargate and Robotics products from each town's wallet. It sends periodic
  attack-move waves, more often for `Zer8`, whose program has ten attack
  groups to `Ze8T`'s four. `MvTe` orders idle hero Dark Templars to the
  script location. This is not a decode of the native AI programs.
- Hero Dark Templars are cloaked to the human. They are not drawn or
  targetable unless a human Overlord (11-tile sight) or a complete Spore
  Colony (the pinned seven-tile cap for grounded detectors) is in range.

The private sprite bank `.local/sc64/zerg-z8/generated` adds the hero Dark
Templar to the Z7 bank's sets. Nydus Canals keep the existing art-free
fallback.

## Saves

`starcraft/sc64_z8_state.h` keeps a 180-byte compact form of the 876-byte
scheduler. It exactly fills the shared Zerg union, so `RaceSave` stays
16,296 bytes. Restore rebuilds the derived fields from the program and
mission setup, then validates the result before use. Load validation borrows
the existing route scratch union instead of reserving a second image.

## Evidence

- `make zerg-z8` links:
  - BSS end `0x0203dbf0`, IWRAM end `0x03004f18`, within the `0x0203e000` /
    `0x03005000` guards;
  - 1,040 bytes of BSS and 232 bytes of IWRAM spare.
- `make test-zerg-z8` (ASan/UBSan host fixture):
  - Z8 is launched through the ordinary campaign menu as the eighth Zerg entry.
  - It checks the binding: 39 instances, both Nydus links, Kerrigan, five
    Overlords, neutral beacons and a valid source identity for every actor.
  - The opening triggers run: the 350/350 start resources, both town wallets
    and both towns started.
  - An ordinary pause-menu save reloads from the title screen into a fresh
    game and matches the uninterrupted run for 3,000 updates.
  - Corrupt candidates are rejected without changing live state.
  - The hero Dark Templar cloak holds until an Overlord is in range; the
    Overlord placement here is a fixture-only state write.
- With no inputs, the human loses at frame 2,231 (139 s): the first raid's
  hero Dark Templar reaches location 18 and T17 fires.
- An uncommitted local ARM smoke ROM, built with the same flags and the
  probe's own main loop, reaches the same result in mGBA (isolated, no
  debugger). It takes the menu route to Z8, then runs with no input to the
  same defeat at frame 2,231, with no fault.
  - The opening averaged about 960,000 guest cycles per frame, with an
    18.3M-cycle peak.
  - This is an idle opening, not representative mission timing.
- The asset-free `tests/sc64_z8_state.c` round-trips the compact state on
  every tick of an authored program and rejects malformed records.
  `tests/test_sc64_z8_program_pack.py` covers the table generator.

## Remaining before Z8 is qualified

- An ordinary-input host controller win.
- An input-only ARM replay and a fresh-process SRAM continuation (the Z7
  replay tooling in PR #154 is the template).
- ARM frame timing.
- An audit of the authored AI against the static decode.

Native AI semantics, the native kill ledger, mission text and sound, Nydus
art and physical-GBA behavior are not claimed.
