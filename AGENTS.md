# Agent guidance

## Project map

- The default original-art skirmish is in `starcraft/game.c` and its supporting headers in `starcraft/`. Start there for game behavior.
- GBA startup, memory setup, display, and input support are in `common/`.
- Optional ROM-derived modes compile the same StarCraft sources under `SC64_*` flags. Campaign, race, and profile adapters live in headers such as `starcraft/race_campaign_adapter.h`, `starcraft/race_adapter.h`, `starcraft/sc64_backwater_adapter.h`, and `starcraft/skirmish_adapter.h`. Imported maps, art, and generated headers stay in ignored local folders.
- Host-side rule and behavior checks are in `tests/`. Build and verification recipes are in `Makefile`; the `dev` wrapper provides the documented workflows. Import, conversion, replay, and smoke-check utilities live in `tools/`.
- The N64 parity harness (`./dev parity`) is in `tools/sc64_parity*.py`, `tools/parity_decode.py`, `tools/parity/` and `tests/sc64_parity_run.c`. It is described in `docs/PARITY-HARNESS.md`.
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

## Check gameplay against StarCraft 64 with the parity harness

The ROM-derived (`SC64_*`) profiles aim to match StarCraft 64 on the N64.
The parity harness checks this against reference values recorded once from
the N64 cartridge. It runs only the GBA host engine and never needs the N64
or an emulator. Always use it:

- Run `./dev parity` before and after any change that can affect gameplay:
  `starcraft/` game code, SC64 adapters and tables, AI, movement, economy,
  combat, fog, animation or sound timing. It takes under a minute. Report
  the scorecard line (counts per verdict and the fixture-set hash) in the PR
  or handoff.
- A `REGRESSION` fails the check. Fix the change; do not widen a band or
  edit a golden to make it pass.
- A `STALE-GOLDEN` also fails. It means a golden was decoded against older
  definitions of the fields it reads, usually after a change to
  `tools/parity/sc64-usa-v1.json` or a decoder. Regenerate it from its
  recorded runs with `tools/parity_decode.py golden`, as
  `docs/PARITY-HARNESS.md` describes. While your change is unmerged, write
  regenerated goldens to a private copy of the fixture home: set
  `SC64_PARITY_HOME=DIR` for both `tools/parity_decode.py golden` and
  `./dev parity`. Regenerating them in the shared home makes them stale for
  main and for every other worktree. Update the shared home only after your
  layout or decoder change lands.
  If the change also alters what the
  capture frontend records, the old runs cannot be re-decoded: make two
  fresh captures with the updated frontend. `--allow-stale` is for
  recording work only, never for a verification run. If a field a golden
  gates on is no longer `proven`, regenerating does not help. Restore the
  field's evidence in the layout file, or change the check so it no longer
  reads that field.
- The GBA follows the N64 ROM, not PC StarCraft or BWAPI values (owner
  decision, 2026-11-26). Every `KNOWN` line in `tools/parity/baseline.json`
  and every ROM-table difference is a GBA defect to fix toward the N64
  value, not an accepted difference. The exceptions are table rows the
  engine never uses for play, such as hero costs or doors that are never
  offered for production. Say why in the PR. `./dev parity` prints
  ROM-table differences only as counts, which do not gate. When you change
  tables, compare the detailed `make parity-tables` report before and
  after.
- When a fix moves a check inside its band, it shows as `IMPROVED`. Then run
  `./dev parity --only ID --accept` for each scenario you fixed, check that
  the baseline diff only removes or narrows the lines you fixed, re-run the
  full `./dev parity` without `--accept`, and commit the baseline with the
  fix. Never use `--accept` to hide a gap that your change opened or
  widened.
- Use `--only ID` and `--explain ID` while iterating on one scenario.
  `--keep DIR` keeps the GBA traces.
- When you add a gameplay feature with no scenario, say so in the handoff.
  Record new references only with the recipe in `docs/PARITY-HARNESS.md`
  (two fresh captures with identical hash chains), and run at most two
  captures at a time on this machine. Keep a new scenario's goldens in a
  private fixture home the same way until its measurement code lands. A new
  scenario's out-of-band gating
  results start as `REGRESSION`. Before accepting them, check each against
  the N64 evidence. List each one in the PR as a GBA defect it now records
  as `KNOWN`. Accept them with `./dev parity --only ID --accept`, in a
  commit that changes no game code.
- The reference fixtures are private. They live in one home shared by every
  worktree, `$SC64_PARITY_HOME` (by default the main checkout's
  `.local/parity`). Never copy them into the source tree. A scenario shows
  `SKIP` when its fixtures are absent (for example in a fresh clone or a
  cloud session). It shows `ERROR` when its GBA runner cannot build because
  the worktree lacks the prepared SC64 profile inputs. Read the reason the
  scorecard prints: a `SKIP` can also mean the checkout has no measurement
  code for a golden, and an `ERROR` can be a real runner failure, which
  must be investigated. Report absent inputs in the handoff, along with the
  checks that did run. Scenarios come from the golden files present, so a
  missing golden gives no row at all. Check that
  the scorecard lists every scenario you expected, and name any that is
  missing. Do not fetch or generate private inputs only to run it for a
  default-profile change.

See `docs/PARITY-HARNESS.md` for scenarios, verdicts and the golden format.

## Clean up scratch data when the work is done

Builds, emulator runs and replays here leave ROM copies, saves and traces of
about 1 GB per run. Many agents share this machine, and leftover runs once
filled its disk. Every agent is responsible for removing the scratch data it
created:

- Put scratch runs in folders that are clearly yours, named after your branch
  or task, for example `/tmp/sc64-<branch>-<what>`. Do not write into another
  agent's folder.
- When your task ends (the PR is merged or closed, the work is handed off or
  abandoned, or a run is superseded), delete what you created: your
  `/tmp`/`/private/tmp` and system temp folders, extra build trees and ROM
  copies, and large run folders under your worktree's `.local/` and `build/`.
  Keep only what a reviewer still needs, and say in the handoff where it is.
- Delete only what you created. Never remove another agent's folders, the
  shared emulator runners (`/tmp/sc64-headless-mgba-*`), the main checkout's
  `.local/` inputs, or anything a running process still uses. Remember that a
  worktree's `.local/` can hold symlinks into other checkouts: delete the
  link, never what it points to.
- Check free space (`df -h /System/Volumes/Data`) before a large matrix or
  replay batch. If the disk is nearly full, stop, clean up your own data, and
  tell the user, rather than retrying.

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
