# Combat and damage fidelity audit

This is a targeted audit of attack damage and related visuals, not a complete
engine audit. The GBA game is an authored adaptation. The pinned OpenBW
revision below is a **PC behavioral reference**, not proof of StarCraft 64
behavior. Native N64 combat, Firebat flame geometry and attack event timing
have not been traced. The pinned BWAPI tables provide stock numeric facts;
no reference implementation code is incorporated. See [References](REFERENCES.md)
and [Validation](VALIDATION.md) for provenance and existing limits.

## Findings and follow-up tests

- **Firebat collateral:** The old area loop subtracted 3 HP directly, including
  from the primary target, bypassing shields and common hit/death handling.
  It has been removed. In race builds the selected target now uses the stock
  Flame Thrower damage, upgrade and damage class once per weapon damage factor;
  the UnitType's maximum three hits are not assumed to be three hits on one
  target. Flame collateral is currently unimplemented, including in the
  default profile. Trace N64 flame events and target geometry before adding it;
  exercise separate primary and collateral targets, allies, airborne and
  invincible actors, shields and death callbacks. `tests/sc64_firebat_damage_game.c`
  covers the current race primary and absence of invented collateral.
- **Projectile timing:** Normal stock unit attacks emit visual effects and
  immediately deal damage in `game.c`'s `tick_combat`. A traveling effect can
  therefore appear after its target dies. OpenBW instead resolves a normal
  bullet hit at impact ([pinned `bwgame.h` lines 14086–14099](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L14086-L14099)).
  Projectile-specialized adapters are separate. A future bounded event design
  needs tests for launch versus impact, target death/motion, slot reuse and
  pool overflow, and independent N64 timing evidence.
- **Fractional damage and shields:** `sc64_stock_combat.h` uses integer HP
  and a minimum of one per positive hit. The PC reference uses fixed-point
  damage and a half-point minimum, applies plasma shield upgrades, and handles
  shield overflow before HP armor and size scaling ([pinned `bwgame.h` lines
  13658–13714](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L13658-L13714)).
  Test repeated weak hits, shield armor, shield overflow, and each size class;
  fractional storage and save compatibility need design before changing this.
- **Shield impact visuals:** The race damage path reduces shields without a
  directional shield impact overlay. OpenBW creates one for qualifying shield
  hits ([pinned `bwgame.h` lines 13650–13655 and 13710–13714](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L13650-L13655)).
  The existing blue shield bar is distinct. Verify N64 art, direction and
  lifetime before implementing; test clipping and bounded effect capacity.
- **Default armor initialization and legacy load, fixed:** The
  non-race profile now stores base armor at spawn and applies the current SIDE
  upgrade once in the hit path. `tests/sc64_default_armor_game.c` compares old
  and new Marines, Firebats and Medics across upgrade levels, and checks an
  enemy unit. Legacy default saves normalize derived base armor at load while
  preserving HP and upgrades. Race builds keep their stock-data codecs.
- **Invincible shield ordering, fixed:** `hit_target_impl_ex` checks the
  campaign invincibility arrays before subtracting shields, so a direct hit
  cannot deplete an invincible unit's or building's shields. The PC reference
  also checks invincibility before applying damage ([pinned `bwgame.h`
  lines 13658–13660](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L13658-L13660)).
  Normal target selection may reject these before they reach the hit path.
- **Building damage and fire:** Shared burn, fractional saves, repair and
  N64-derived damage overlays are implemented in [BUILDING-DAMAGE.md](BUILDING-DAMAGE.md).
  Native N64 burn logic, random overlay history, blending and alternate-pose
  anchors remain unqualified.

The current normal Terran qualification also exposes a scripted T5 loss. It
reproduces on earlier mainline `9ff7f7c` and pre-burn `07de5d0`; those baseline
traces/logs match byte for byte. This remains a campaign qualification blocker,
not evidence that the new building mechanics complete the campaign. Candidate
controller behavior differs, so it still needs a normal gameplay investigation.
