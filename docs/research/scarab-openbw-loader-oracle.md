# Pinned OpenBW IScript loader probe

This source-only probe is an early prerequisite for a headless Scarab scheduler
oracle. It does not execute a game frame or prove movement, impact timing,
allocation, splash, or native N64 behavior.

`sc64_openbw_iscript_oracle_prepare.py` verifies the pinned `bwgame.h` hash,
extracts the actual `global_init` IScript loader lambda into this worktree's
ignored `.local`, and adds its invocation. `sc64_openbw_oracle_input.py`
authenticates a user-supplied USA ROM and its IScript BOLT entry before staging
that private entry in a new ignored file. It refuses to overwrite a file.
`sc64_openbw_oracle_run_loader.py` verifies the staged entry hash before
launching the probe. No ROM-derived payload is committed.

The C++ probe supplies only that IScript input to the actual pinned loader and
checks that scripts 160, 286, 287 and 292 and the relevant animation entry
points exist and point inside the relocated program. No IScript signal or event
is injected. Loader execution alone does not independently validate the source
semantics or initialize the game world. Full world types, image and sprite
pools, map/pathing, unit lists and authenticated transitive inputs remain to be
constructed before `process_frame()` can be meaningfully exercised.

The probe intentionally has no default build target. Its ignored local receipt
and input hashes must accompany any experimental result.

For a local reproduction, choose fresh paths under `.local/oracle-run` and set
`OPENBW_REF` to the pinned checkout and `SC64_ROM` to the authenticated USA
ROM. The preparation commands fail rather than overwrite existing files:

```sh
mkdir -p .local/oracle-run
python3 tools/sc64_openbw_iscript_oracle_prepare.py \
  --bwgame "$OPENBW_REF/bwgame.h" --out .local/oracle-run/openbw_iscript_loader.inc
python3 tools/sc64_openbw_oracle_input.py --rom "$SC64_ROM" \
  --extractor references/sc64-maps/extract_sc64_maps.py \
  --out .local/oracle-run/iscript.bin
c++ --version > .local/oracle-run/compiler.txt
c++ -std=c++17 -O1 -I"$OPENBW_REF" -I.local/oracle-run -M \
  tests/sc64_openbw_iscript_loader_probe.cpp > .local/oracle-run/dependencies.txt
c++ -std=c++17 -O1 -I"$OPENBW_REF" -I.local/oracle-run \
  tests/sc64_openbw_iscript_loader_probe.cpp -o .local/oracle-run/loader-probe
python3 tools/sc64_openbw_oracle_run_loader.py \
  --script .local/oracle-run/iscript.bin --probe .local/oracle-run/loader-probe
```

Record SHA-256 hashes of the compiler, all dependencies listed by `-M`, the
probe binary and the staged input alongside the command and output. The
preparer checks the pinned `bwgame.h` hash; the remaining header hashes bind a
particular build rather than asserting that those headers match the reference
revision. Input extraction also verifies the normalized ROM SHA-1 and the
script SHA-256. Run `python3 -m unittest discover -s tests -p 'test_sc64_openbw_loader_prepare.py'`
to exercise the asset-free preparation safety checks.

## Experimental flat-world probe

The optional world probe stages the authenticated N64 DAT, IScript, GRP and LO
entries into a fresh ignored directory and maps the original image indices to a
synthetic filename table. It rechecks every staged input against the ROM before
running. It uses the pinned `global_init`, `load_map_data`, `reset`, unit
creation, player selection, fighter training and attack actions, and
`next_frame`. Its 32-by-32 flat map and walkable VF4/CV5 tilesets are synthetic;
no game map or native N64 execution is represented. The trigger blob is empty,
and the probe uses a non-melee setup with no trigger execution.

Given the same `OPENBW_REF` and `SC64_ROM` variables above, use fresh paths:

```sh
python3 tools/sc64_openbw_oracle_tables.py --rom "$SC64_ROM" \
  --extractor references/sc64-maps/extract_sc64_maps.py --out .local/oracle-run/tables
python3 tools/sc64_openbw_oracle_images.py --rom "$SC64_ROM" \
  --extractor references/sc64-maps/extract_sc64_maps.py --out .local/oracle-run/images
c++ -std=c++17 -O0 -g0 -I"$OPENBW_REF" \
  tests/sc64_openbw_world_probe.cpp -o .local/oracle-run/world-probe
python3 tools/sc64_openbw_oracle_run_global.py --rom "$SC64_ROM" \
  --extractor references/sc64-maps/extract_sc64_maps.py \
  --tables .local/oracle-run/tables --images .local/oracle-run/images \
  --probe .local/oracle-run/world-probe
```

One authenticated source run observed paid fighter completion, native Reaver
release, projectile creation and victim damage. It is a single synthetic-map
scenario, not proof for other headings, pool exhaustion, timing boundaries,
movement geometry, saves or the native game. Reproduction should retain source,
compiler, dependency and input hashes as described above. The global-init
compatibility probe also compares every IScript PC and program word to the
extracted loader after reading the real image and LO entries, then intentionally
stops before tileset loading.

## Bounded action-driven scenarios

The world probe accepts `--scenario` through `sc64_openbw_oracle_run_global.py`.
`east`, `south`, and `far` check paid training, parent/child linkage, release,
actual attack initialization and weapon/target attribution. The far case checks
seven no-collision movement passes and subsequent clearance. `stop` exercises
the real Reaver STOP action; `targetdead` and `parentdead` invoke the reference
kill API and check target cleanup and detached-shot behavior. `poolfull` and
`initfail` deliberately exhaust a source pool through OpenBW allocation APIs;
these are controlled test setups, not natural player input scenarios. Every
scenario begins from the same synthetic flat map, and none injects a script
signal or an order-state value. Run each as the previous world command with
`--scenario east` (or another listed name), saving the source, binary, input
hashes and full output for each run.

The synthetic CHK contains no UNIS or UNIx unit overrides. The probe checks
the loaded Reaver (100 HP, 80 shields), Scarab (20 HP), Marine (40 HP), and
weapon 82 (100 damage, 25 upgrade bonus) values, as well as initial actor
ownership and fighter linkage. Health in the trace is raw 8.8 fixed point:
`shot_hp` names the Scarab and `victim_hp` the Marine. Earlier `hp=10240`
observations named the victim, not the shot. The detached-parent case checks
that weapon 82 retains player ownership, has no parent unit and targets the
Marine. The target-death case has no second enemy and therefore does not
establish behavior in the presence of alternative targets.

These scenarios are finite witnesses. They do not cover all trajectories,
blocked-route lifetime expiry, target disappearance timings, all allocation
failures, splash, save integration, or native N64 execution. They do not define
a general impact delay. Synthetic test-created projectiles and sprites in the
allocation-failure cases must not be interpreted as ordinary game actions.
The pool-exhaustion cases establish saturation before arrival and no observed
weapon or damage in the bounded window; they do not instrument the exact
allocation attempt or prove the pool remained exhausted at that instant.
