# Canonical skirmish SRAM continuation — 2026-10-27

Base main: `c3c8ac956aa8fbc69a09562d629e7c3897c7d06c`.
This change adds verification and status documentation. It changes no game
rules, ROM assets, control bindings, save layouts or product selection.

## Coverage added

`tests/sc64_skirmish_resume.c` is compiled three times with the actual
`TERRAN_ROM_FLAGS`, `ZERG_ROM_FLAGS` (Z6) and `PROTOSS_ROM_FLAGS` (P5), using
those profiles' prepared input dependencies and canonical presentation flags.
It deliberately does not substitute the legacy Z3/P3 match-test builds.

Each profile runs 24 scenarios:

- Both map choices (authored Outpost and imported Blood Bath), all three
  human races, and bot counts 1, 2 and 3: 18 configurations. Bot races rotate
  deterministically through all three factions as player slots are added.
- Both maps with three Terran, three Zerg or three Protoss bots, each against
  a different human faction: six more configurations.

Every launch uses ordinary HOME/custom-game/frontend button input. No units,
resources, orders, save payloads or world state are directly injected.
The test observes menu fields to steer buttons and inspect the resulting
configuration. All active bot slots are checked immediately after launch;
all three stored bot-race slots are checked after loading.

For each scenario:

1. Advance 1,800 ordinary gameplay updates. Require player mineral income
   and actual mineral depletion on the map.
2. Open pause and save with B. Observe the saved world before any release
   input can accidentally advance the live simulation.
3. Resume through A and run exactly 900 gameplay updates as the reference.
4. Call `game_init()` to reset actors and the frontend, retain host SRAM,
   then load through HOME L. Require the saved configuration and live-world
   hash immediately, before the next gameplay update.
5. Run 900 neutral updates and require the same world hash and counters as
   the reference. Player minerals must increase and remaining map ore must
   decrease during this continuation too.

The hash includes frame/RNG state, supply, training and research state,
player race identities and banks, mineral amounts/positions, geyser locations,
explored cells, and semantic fields of living units/buildings. Unit fields
include health, shields, armor, cargo, order/target, work and attack timers,
path/waypoint state and raw source identity. Building production queues are
hashed in logical circular order. Stale dead-slot bytes and unused queue
entries are omitted deliberately; derived current visibility is not persisted.

Canonical race AI is `race_tick_ai()`, which plans from saved world state and
frame cadence. It does not use the separate default-only `skirmish_ai[]`
planner/save record.

## Commands and local evidence

With the three race profiles prepared:

```sh
make test-sc64-canonical-skirmish-resume
```

This target is also a dependency of `test-race-campaigns`. The asset-free
`./dev test` does not require proprietary data or run the optional fixture.

The local prepared banks are shared with other worktrees. Validation executed
the target while marking three already-prepared banks as old, to prevent
unrelated importer regeneration:

```sh
make \
  -o .local/sc64/weapon-vfx/generated/sc64_weapon_source_generated.h \
  -o .local/sc64/attack-poses/generated/sc64_unit_attack_source_generated.h \
  -o .local/sc64/death/generated/sc64_death_sprites_generated.h \
  test-sc64-canonical-skirmish-resume
```

All 72 cases pass under ASan/UBSan. `./dev build` and `./dev test` also pass,
including all 548 Python checks and the asset-free host regressions. The
original-art ROM is unchanged (SHA-256
`2b89b77d3ab284d42bdf862d55ed85e37ceae2c70e785634055f9f3231cec19a`).
The target compiles the normal game and
host GBA implementation without modifying prepared assets. Private command
logs are under `.local/build/skirmish-resume-20261027/`; imported headers,
ROMs and SRAM remain ignored. No Actions were enabled or dispatched.

## Limits and campaign qualification finding

This is an in-process host SRAM reload. `game_init()` resets actors and several
globals, not every byte of process memory. It does not establish fresh-process
or emulator SRAM continuity, physical GBA behavior, final match victory,
every bot-race permutation, audio/visual identity, or near-30-fps performance.
It compares a defined semantic projection of live gameplay, not a byte-for-byte
snapshot of every runtime structure.

Canonical campaign coverage remains T1–T10 / Z1–Z6 / P1–P5: 21 of 30 entries.
The separate current P6 probe still links within its guards: IWRAM end
`0x03004580`, BSS end `0x0203db68`, and `sizeof(RaceSave) = 16348` against the
16368-byte cap. Linking alone does not qualify a chapter for promotion.

A fresh P6 controller built from the base source lost at frame 143714 after
147212 host ticks, with zero adapter faults. Its previous winning tape has
15554 input transitions through host tick 174855, winning at frame 172327 on
an older frozen build. Replaying those exact inputs on the current runtime
no longer reproduces that state. The first *observed* difference in the
900-frame checkpoint stream is selected count at frame 900 (old 2, current 0);
by frame 5400 Tassadar's position also differs. These checkpoints do not locate
the exact first divergent update or prove a game-engine defect.

Private P6 failure/replay evidence is under
`.local/build/p6-readiness-20261027/{root,agent}/`. The next campaign task is
to repair selection and steering against current actor trajectories, preserve
source stock facts and mission conditions, then obtain a fresh host win,
ARM playthrough and SRAM proof before promoting P6. Historical P6 wins remain
frozen-build evidence, not a current canonical completion claim.

A private follow-up prototype replaced the first failed Zealot selection with
live-position selection and verified both attack-move orders. It then reached
the 180000-frame limit without a win, with missing early trigger/hero-route
progress and no eastern Marine group. This isolates a further route-repair
task; the prototype is not a mission qualification or a shipped controller fix.
