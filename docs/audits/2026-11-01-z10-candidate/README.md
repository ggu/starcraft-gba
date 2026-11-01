# Z10 candidate profile (2026-11-01, not qualified)

Zerg mission 10, "Full Circle", now runs in an explicit `zerg-z10`
candidate. It is the Z1–Z9 candidate plus Z10 as the tenth campaign entry,
with code `SCZA`, title `SC ZERG Z10` and generation-49 saves. It stacks on
the [Z9 candidate](../2026-11-01-z9-candidate/README.md) and reuses its
late-Zerg scheduler, runtime adapter, compact save and town AI. The
canonical `zerg` alias, the existing `zerg-z3`…`zerg-z8` ROMs and the
`zerg-z9` candidate are unchanged (hashes below). Z10 has no ordinary-input
win, ARM replay or timing yet, so this is not a mission qualification.

## Source

`tools/sc64_zerg_late_pack.py pack --mission Z10` verifies the retained
scenario against its pinned contract:
- global catalog entry 21, map path `008/01D`, 96×128 tiles;
- CHK SHA-256 `1a8f0e1d…`;
- descriptor fingerprint `97c32263…`.

`make zerg-z10` renders it into ignored tables at
`build/generated/z10/sc64_z10_program_generated.h`:
- 10 triggers, 10 conditions, 30 actions and 64 locations;
- active owners 0, 2, 3 and 7, with forces 2, 3, 1 and 4.

The map, with 337 retained rows, was already in the shared staged race-map
header behind `SC64_Z10_DATA_ENABLED`.

The program, as decoded from the authenticated rows:
- **Defeat:** the human commands no units (T0).
- **Temple:** when the human has killed the Protoss Temple (raw 174, slot 0),
  T2 creates a human Drone and the Khaydarin Crystal at location 1.
- **Victory:** the crystal is brought to location 4, the Temple's site (T3).
- **Towns:**
  - Slot 2 (`Z10a`/`Z10b`, 8,000 resources) and slot 7 (`Z10c`/`Z10d`,
    8,000) start at once.
  - Slot 0, the Temple's guard (20,000), runs `Z10e` once switch 10 is set.
    Switch 10 is set by the guard's first death (T7) or by the absence of any
    Temple (T8).

## Engine additions

- Raw 174 gets a `SC64_Z10_RUNTIME`-only building identity with pinned BWAPI
  facts: 7×3 tiles, 112/48/111/47 source pixels, 1,500 HP, armor 1. It is
  destructible.
- The Z10 mission table binds the program through the shared adapter.
- **Kill census:** the starting Temples minus those alive. This is the same
  authored stand-in as Z8's Nexus census, since only the human fights the
  Temple.
- **Deaths census:** slot 0's starting actors minus those alive. Slot 0
  produces nothing until this row has fired, so the count is exact up to
  that point. Like any disappearance, a removal would also count; Z10
  removes no slot-0 actor.
- The Drone, crystal, crystal carry, timed presentation, compact save and
  town AI are the Z9 candidate's.

## Evidence

Run at base `72b6de1f` (`codex/p8-playable-20261029`) plus the Z9 change
and this change.

- `make zerg-z10` links:
  - BSS end `0x0203df88`, IWRAM end `0x03004bf0`, within the `0x0203e000` /
    `0x03005000` guards, with 120 bytes of BSS spare;
  - ROM SHA-256 `4037575b…`, with the Z8 sprite bank and the inputs listed
    in the Z9 audit.
- `make test-zerg-z10` runs two ASan/UBSan host fixtures:
  - **Carried Z9 fixture** under the Z10 flags. It passes with the same
    save-continuation digests as in the Z9 build (`cce4ad17`, `21b66920`).
  - **Z10 fixture:**
    - Z10 is launched through the ordinary campaign menu as the tenth Zerg
      entry, with no unsupported rows.
    - It checks the binding: 10 instances, active mask `0x8d`, forces, the
      human's starting units, the destructible 1,500-HP Temple, and 60
      starting slot-0 actors. The authored FourCC constants match the
      authenticated action rows.
    - The opening triggers run: 1,000/1,000 human resources, towns 2 and 7
      started, and 20,000 for the guard.
    - Removing one guard Zealot (a fixture-only state write) satisfies the
      Deaths row. T7 sets switch 10, and T9 starts the guard's town.
    - An ordinary pause-menu save with all three towns running reloads from
      the title screen and matches the uninterrupted run for 3,000 updates.
      Corrupt candidates are rejected: instance flags, a non-zero union tail,
      and a moved location that Z10 cannot have.
    - From that save, destroying the Temple (fixture-only) satisfies the
      Kill row. T2 creates the Drone and crystal at location 1, and the Drone
      carries the crystal.
    - A second pause-menu save, with the crystal carried, matches for 1,000
      updates.
    - The carrier placed in location 4 (fixture-only) completes T3's
      presentation and Victory.

## Byte identity

To be filled from the final run.

## Not qualified

- **No ordinary-input host controller win.** The fixture removes a guard,
  destroys the Temple and moves the carrier by state writes.
- **No input-only ARM replay and no fresh-process SRAM continuation.**
- **No ARM frame timing.**
- **Sprite art:** the default bank is the Z8 bank, which has no art for raw
  174 (Temple) or raw 219 (crystal). The renderer draws its generic fallback
  for them.
- **The Kill and Deaths censuses are authored stand-ins**, not native
  ledgers. The town AI, timed presentation and crystal carry are authored,
  as in Z9.
- Mission text, sound and physical-GBA behavior are not claimed.
