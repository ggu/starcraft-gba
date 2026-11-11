# Protoss Forge and Cybernetics Core research on SRAM

Forge and Cybernetics Core research is enabled in the normal Protoss P10 ROM
(`make protoss`) for custom games with a Protoss human. It uses the staged
`0x45` producer-bound job journal and the padding-bit level codec that were
merged default-off in #371, with SRAM only. No Flash path, no Q8 fractional
combat and no change to a persistent enum ID is involved. Everything below was
measured on the HOST or by linking the ARM ROM; nothing ran on hardware or in
an emulator.

## What the player gets

- Select a completed Forge or Cybernetics Core and press `B`. The menu has four
  rows: for the Forge Ground Weapons, Ground Armor and Shields; for the Core
  Air Weapons, Air Armor and Singularity Charge; the fourth row cancels.
- A job costs the quote table's price (`sc64_pr_quote_v45`), the same amount of
  minerals and gas, rising per level: Ground Armor 100 + 75, Air Armor 150 + 75,
  Ground Weapons 100 + 50, Air Weapons 100 + 75, Shields 200 + 100, and
  Singularity Charge 150 once. Time is the 5:4-scaled source time: 5,000,
  5,600 and 6,200 ticks for levels one to three, 3,125 for Singularity Charge.
  Maximum level is three (one for Singularity).
- Levels two and three need the same prerequisites the staged policy encodes:
  Templar Archives for ground weapons/armor, Fleet Beacon for air, Cybernetics
  Core for shields.
- One job per building. The same family cannot run in two buildings of one
  owner. A refused command shows a reason (`Need more minerals`, `No gas`,
  `Research unavailable`, `Tech prerequisite missing`) and takes nothing.
- The selected building's panel shows three slots (weapons, armor, shields or
  Singularity) with their current effective levels and a bar under the slot
  whose job is running. Selected units show the levels they fight with.
- Cancel refunds the recorded price in full. A destroyed building pays back
  three quarters (the PC rule, as already staged). Without Pylon power the job
  pauses, stays paid and bound, and resumes when power returns. A building with
  a running job cannot change owner (the existing transfer veto).

## Combat

Levels apply to the owner's units only (a computer opponent has its own levels,
none of which is ever started today). The old generic human weapon and armor
levels remain floors, never added twice.

| Effect | Where it applies |
| --- | --- |
| Weapon damage per level (the stock table's per-weapon bonus) | `tick_combat` stock weapons, therefore the impact queue; Archon splash; Interceptor projectile packets |
| Armor, one point per level | every packet that reads `sc64_unit_armor_upgrade_level`: ordinary, Archon, Explosive adapter, siege packet |
| Plasma Shields, one point of shield armor per level while the target still has shields | `hit_target_impl_ex` (units and buildings, and every adapter that finishes through `hit_target`), the siege packet |
| Singularity Charge: Dragoon range +32 (64 source px at half scale) | acquire and attack range |

Integer rounding, compared with the original game's fractional rules: the
upgrade bonuses themselves are integers in the original, so weapon, armor and
shield-armor points are exact. The engine keeps whole hit points. A hit never
does less than 1 (the original's floor is 1/2). Explosive and concussive size
ratios floor per impact (the original carries the fraction). Shield armor
reduces a hit once, before the shield absorbs it and before any overflow
reaches the hull. Psionic Storm bypasses shield armor (its adapters edit
shields directly) and nothing here changes that. Q8 mode stays off.

## Save, load and in-progress jobs

Levels: 2 bits per owner and family, 12 bytes for the 5 custom owners' worth of
data, stored in the 5 spare bits of each building row (`SC64_PR_ROW_WIDTH` 25,
3 bits used) in the `0x45` tag. Jobs: no extra bytes. The producer row already
holds the family (`level`), payer (`queue[0]`), target level (`queue[1]`) and
remaining ticks (`queue_progress`); payer, family and the frozen price are
reconstructed from those on load and the whole record is validated before it
is published. So an **in-progress job is saved and resumes**: it keeps ticking
after the load and cancelling it afterwards refunds the exact recorded price.
Decision 3's fallback (cancel at load with a full refund) was not needed.

Save size: the record is unchanged. `RaceSave` plus the impact record and the
transport trailer are the same bytes before and after (see the table in the
branch report); the alternating slot pair is kept (`save_write_selected`), and
a test corrupts the newest slot and loads the previous one.

Compatibility: only custom games (`race_mode` 1) write the `0x45` tag.
Campaign saves keep the old tag and layout. Every save written by the previous
P10 ROM, campaign or custom, loads with all levels zero; the test writes one with
the unchanged default branch and compares a digest of the loaded world.
A save written by the new ROM in a custom game is not readable by the old ROM.

## What does not work

- Campaign missions: the staged policy needs the generated
  `sc64_protoss_upgrade_policy_generated.h`, which is not among the private
  inputs available here. Campaign research stays the old generic Forge path.
- Computer opponents never start research. The engine already supports it (any
  active Protoss owner can be a payer, the codec holds five owners): the
  planner in `race_skirmish_ai.h` would have to build a Forge/Core, choose a
  family from its army mix and call `sc64_pr_stage_begin_slot` for its own
  slot when its bank allows.
- Terran, Zerg Z9, Z10 and Showcase ROMs are unchanged (byte-identical). A
  Protoss human in a custom game on those ROMs does not get research. Their
  save rows use a 28-byte layout with one used padding bit and different RAM
  margins; each needs its own ARM and controller qualification.
- Psionic Storm ignores shield armor; no HUD slot shows the shield level on
  selected units.
- Hardware: untested.
