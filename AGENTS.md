# Agent guidance

## Project map

- The default original-art skirmish is in `starcraft/game.c` and its supporting headers in `starcraft/`. Start there for game behavior.
- GBA startup, memory setup, display, and input support are in `common/`.
- Optional ROM-derived modes compile the same StarCraft sources under `SC64_*` flags. Campaign, race, and profile adapters live in headers such as `starcraft/race_campaign_adapter.h`, `starcraft/race_adapter.h`, `starcraft/sc64_backwater_adapter.h`, and `starcraft/skirmish_adapter.h`. Imported maps, art, and generated headers stay in ignored local folders.
- Host-side rule and behavior checks are in `tests/`. Build and verification recipes are in `Makefile`; the `dev` wrapper provides the documented workflows. Import, conversion, replay, and smoke-check utilities live in `tools/`.
- Read `CONTRIBUTING.md` before changing asset handling or licensing, and `docs/DEVELOPMENT.md` for setup and profile-specific commands.

## Make changes safely

- Core engine fixes and improvements must apply across Showcase, skirmish and
  campaign products. Put shared mechanics in the shared engine and common
  build capability set; do not enable them only in a Showcase flag list or
  behind a mission-active predicate. Mission objectives, source permissions,
  content and storage/placement choices may remain profile-specific.
- Validate shared changes across the production build and gameplay-mode
  matrix. Missing assets or save-state dependencies are unfinished work to
  resolve and document, not a reason to advertise an actor with incomplete
  mechanics. Do not widen a mission-active predicate to activate objectives
  or reuse incompatible persistent state in other modes.

- Keep runtime data and work bounded for GBA memory and CPU limits. Use integer arithmetic, fixed-size storage and explicit limits; avoid unbounded allocation in game code.
- Preserve the default profile's independence from game ROMs and imported data. Never add ROMs, extracted game data, generated asset headers, or proprietary screenshots to the source tree.
- Keep the pinned reference revisions and imported reference checkout unchanged. Preserve the project's GPL-3.0-or-later license and existing third-party and file-level notices; do not relicense code without the owner's authorization.
- Keep claims about original StarCraft fidelity and hardware performance within the limits documented in `starcraft/README.md` and `docs/VALIDATION.md`.

## Verify changes

For the default profile, run:

```sh
./dev build
./dev test
```

`./dev build` builds and verifies `build/StarCraft-GBA.gba`. `./dev test` runs the asset-free host checks. When a change affects optional unified code, and its ignored generated inputs are already available, also run:

```sh
make unified
make test-unified
```

For changes to the full Terran campaign, when its private inputs are prepared,
also run:

```sh
./dev sc64 build --profile terran
./dev sc64 test --profile terran
```

The build checks the ARM ROM and unchanged RAM reserve guards. The test command
runs all ten mission controllers, startup and save checks, and the skirmish
matrix under host sanitizers; it does not require an emulator. Normal-menu ARM
replay and fresh-process SRAM qualification are separate checks described in
`docs/VALIDATION.md`. See `docs/RACE-CAMPAIGNS.md` for campaign controls and
profile limits.

Do not fetch or generate private game inputs just to complete a default-profile task. If an optional check cannot run because its local inputs are absent, say so in the handoff and report the default checks completed.

## Local verification and Actions usage

- Run normal source-boundary, doctor, build and test checks on the local machine.
  Use `python3 tools/check_source_tree.py`, `./dev doctor`, `./dev build` and
  `./dev test`, plus the applicable prepared-profile checks above.
- The source-check workflow is intentionally disabled in repository settings
  and accepts only manual dispatch. Do not enable or dispatch it, add automatic
  push/PR/scheduled runs, or select paid runners without an explicit user request.
- Local evidence is sufficient for review and merge when applicable checks pass;
  do not wait for or create a redundant Actions run. Record the tested commit,
  commands and results in the PR or handoff.
- A requested Linux compatibility check may use the standard Ubuntu runner for
  this public repository. Keep its manual-event guard and cancellation of
  superseded runs. Do not automatically run public PR code on personal runners.
