# P10 "Eye of the Storm" candidate (2026-11-01)

This audit records an explicit `protoss-p10` candidate: the P9 candidate
(#155) plus P10 as the tenth Protoss campaign entry. It is a runtime and
save integration with host checks only. **P10 is not qualified as winnable
by ordinary input, and nothing is promoted.** Canonical Protoss stays the
qualified P7 product.

## Why it is stacked on P9

P10 needs services that exist only on the P8/P9 stack, not on
`codex/source-release`: the shared Protoss unit services
(`sc64_protoss_services.h`), the Z8 live-pool world bridge with the BWAPI
dimension fallback, the P8 request heap and admission gate, and the Protoss
mission save union that P8 and P9 extend. P10 also dispatches the shared
service hooks after P9's, the way P9 does after P8's. The branch therefore
starts from #155's head (`ced313a8`) and the pull request targets
`codex/p9-playable-20261029`. P10's own code sits behind its own flags, so it
can be rebased onto whatever P8 and P9 land as.

## What is wired

- **Profile.** `make protoss-p10` builds `build/StarCraft-Protoss-P10-GBA.gba`
  (`starcraft-protoss-p10.elf`, title `SC PROTOSS10`, code `SCPA`) from the
  P9 candidate's flags plus `SC64_P10_DATA_ENABLED`, `SC64_P10_RUNTIME` and
  `SC64_P10_CAMPAIGN_ENTRY`, with fact tables generated for its roster in
  `build/generated/p10-fact-v2`. Its rules live in `mk/protoss-p10/Makefile`,
  which the top-level Makefile includes: the top-level file is within a few
  hundred bytes of `tools/check_source_tree.py`'s 512 KiB limit.
- **Catalog and menu.** Catalog entry 29 ("EYE OF THE STORM") is playable
  after P9.
- **Saves.** Generation 47; the record tag is `0x5343505f` with the profile's
  flag bits. The P10 state is 110 bytes inside the existing 180-byte Protoss
  mission union, so the 16,292-byte save record keeps its size. P7, P8 and P9
  saves are rejected by tag; nothing is converted.
- **Runtime.** `starcraft/sc64_p10_runtime_adapter.h` validates the
  authenticated compact program (10 triggers, 75 actions, 9 conditions,
  64 locations) before any cursor indexes it. It runs one runner per
  executing source owner: the human (slot 1, force 1), the Overmind's brood
  (slot 3, force 2) and the Zerg brood (slot 0, force 3). The runner follows
  the P9 adapter's cursor, pending and blocked contract.
  - **Start-up trigger.** CreateUnitWithProperties builds Raynor's Terran
    base for the human (Command Center, three Supply Depots, four SCVs,
    twelve Marines, two Firebats, Barracks, two Bunkers, Engineering Bay,
    Missile Turret) and the rescuable Hyperion. A strict decoder accepts only
    the 28 used UPRP rows: full HP, plus full energy on the Hyperion's row,
    with no owner, shield, resource, hangar or status field. Buildings are
    created complete. SetResources gives the human 100 minerals and 100 gas
    and each computer wallet 8,000 of each.
  - **Wait and Transmission** share one timer per runner, as native waits do.
    A Transmission uses an authored 4,000 ms duration (the sound lengths are
    not imported), as in P8. A trigger whose wait is queued behind another
    waits its turn: the defeat triggers' one-second wait follows the opening
    transmissions.
  - **CenterView** snaps the camera. **MoveLocation** centres location 32 on
    the Gantrithor; that location is saved and only presentation reads it.
  - **Switches** 0–15 are stored (P10 uses 0 and 8).
  - **Victory**: player 3 commands no Overmind (raw 147), followed by the
    source epilogue. **Defeat**: no player commands Zeratul (raw 75), the
    Gantrithor (raw 82) or the Hyperion (raw 28).
- **Rescue.** Zeratul, the Gantrithor and the Hyperion start rescuable
  (slot 2). Contact with a human unit (32 engine pixels, the P4/P6 rule)
  transfers them to the human slot, as native rescue and the P9 adapter do.
  Zeratul and the Gantrithor start beside human units, so they join at once;
  the Hyperion waits by Raynor's base.
- **Bindings.** P10-gated bindings add raw 82 (`U_P10_GANTRITHOR`) and raw
  147 (`B_P10_OVERMIND`) with pinned BWAPI values: Gantrithor 800 HP, 500
  shields, armor 4, flying, Large, 9-tile sight, bounds L32/U32/R31/D31;
  Overmind with shell 5,000 HP, armor 1, 5×3 tiles, bounds L80/U32/R79/D40.
  The Gantrithor draws with the Carrier's art (raw 72), the way P8's
  Hyperion reuses raw 12; its own N64 art chain is not separately verified.
  Every other P10 raw type already had a binding and source art in the
  canonical Protoss bank.
- **Typed save row.** The P7 typed unit row's canonical shield bound was 400
  (Zeratul). Its shield field is 9 bits, so P10 builds raise the bound to 511
  for the Gantrithor; without P10 the bound is still 400. Before this change
  every P10 save failed once the Gantrithor existed.

### AI backend

The native SC64 AI script programs are undecoded, so
`starcraft/sc64_p10_ai.h` is a bounded authored backend built like P9's. It
accepts only the five authenticated RunAIScriptAtLocation rows, matched on
script ID, trigger, action, owner and location (one ID runs at three
locations):

| Script | Owner | Location | Role |
| --- | --- | --- | --- |
| `P10o` | slot 3 | 2 | Produces; holds its ground |
| `P10C` | slot 0 | 6 | Produces; attacks |
| `Pr10` | slot 0 | 4, 5, 3 | Produces; attacks |

Every 120 updates each town builds a request heap from Zergling, Hydralisk
and Mutalisk requests. It trains from its own Hatcheries and Hives, paid from
the source wallet through `race_queue_unit()`. The heap passes the native
cap/count admission gate, is rebuilt on every step and is never saved. Caps
count the whole source slot. Attacking towns send idle units near the town at
the human structure nearest to the town, from update 9,000 and then every
4,500 updates. Waves start at four units and grow by two, up to twelve.
Native composition, cadence and targeting are not claimed.

## Not active in P10

- **Raynor's base cannot grow.** Production and construction check the
  side's race identity, which is Protoss in this profile. The human's
  Terran units fight and move, but the human cannot train Terran units or
  build Terran structures.
- **The Gantrithor has no attack.** Its Interceptors are not launched: the
  Carrier hangar and flight services are bound to the P7 Carrier type.
- The Hyperion's full-energy property has no effect, because raw 28 has no
  energy in this engine.
- Defiler spells (the Zerg have Defilers) and AI use of detectors.
- Mission text, transmissions and sounds are not presented.
- The P10 PUNI, UPGR and TECx availability tables are not applied.

## Qualification

Inputs are the locally verified USA ROM (normalized SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`) through the pinned extractor
(`references/sc64-maps/extract_sc64_maps.py` @ `09b57f787a24`), read from the
frozen 2026-10-31 prepared-input set. The P10 scenario header (from the P10
CHK, global map 29) has SHA-256
`7844f796f42011dd0a33ef9f74fe69bd51eb1b3c12e031153035d57e18d0961a`; the
race-map descriptor `1d4e8999…7c39`; the canonical Protoss sprite bank
`db921084…54c5`. The audio is the shared codec-2 bank
(`SC64_AUDIO_DIR=.local/sc64/audio-codec2/generated`). No ROM, CHK, generated
header, trace or save is tracked, and no input was regenerated.

### Host mission checks

`make test-sc64-p10-mission` runs `tests/sc64_p10_mission.c` under ASan and
UBSan. It starts P10 through the ordinary campaign menu and drives the game
through `game_update()`:

1. **Start-up.** The first source pass runs the start-up trigger. The test
   checks every created actor, the source slot and live owner of each, both
   wallets, all five town scripts, the human's resources, the immediate
   rescue of Zeratul and the Gantrithor, and the Overmind owned by slot 3.
   The opening transmissions set switch 8 at frame 457, and trigger 6
   completes.
2. **Save continuation.** With no further input the mission runs to frame
   8,000 and saves through the pause menu (START, B, START). The SRAM record
   is tag `0x7343505f`, 16,292-byte payload, sequence 1. The run continues to
   frame 18,000 or the end, recording a state digest every 500 frames. Zerg
   waves begin at update 9,000; with no defence the Gantrithor, which has no
   attack, is killed and the mission is lost at frame 13,361. The test then re-executes itself as a
   fresh process, quick-loads the SRAM image with L on the title screen and
   continues. Every digest and the final defeat state match the
   uninterrupted run.
3. **Outcomes.** Separate runs kill the Overmind, Zeratul, the Gantrithor or
   the Hyperion directly (a test fixture, not ordinary input). The source
   triggers then end the mission: victory at frame 1,017 after the full
   epilogue, and defeat at frame 497 for each hero. During the victory
   epilogue, with a Transmission pending, an in-process save and load
   continues to the same digest.

Two runs gave byte-identical output, saves and digests (save SHA-256
`7f21ac0a949b00ea9a465bbb0d29195a09107a0bd1151c7f7afd4db131339b61`, digest
timeline `b9653454…7098`).

`make test-sc64-p10-carried-profile` runs the promoted-profile check with the
P10 flags: each of the ten Protoss entries starts from the normal menu, runs
60 updates, then saves, loads and re-saves to an identical record. 234
skirmish startup/save configurations pass.

### Builds and guards

| Build | `__bss_end` | `__iwram_end` |
| --- | --- | --- |
| P10 candidate | `0x0203913c` | `0x03004a50` |
| P9 candidate (same tree) | `0x02038d7c` | `0x03004a48` |

Both are below the unchanged guards (`0x0203e000` and `0x03005000`). The P10
ROM (`make protoss-p10`) has SHA-256
`b37e448610b6e7fad1fa7cf08eb153a2dad8c024bf8a99eeb8e62c3c431603f7`.

### Non-regression

With the P10 flags off, these products were built from this branch and
from its base `ced313a8` with the same inputs and compared:

| Product | Target | SHA-256 (branch = base) |
| --- | --- | --- |
| Default original-art | `make verify` | `73c0deeeb36cca4cb8614248a7e60cadac11a0ec542f2ece31c37b171e2c56b8` |
| Canonical Protoss P7 | `make protoss` | `f93387e392a66831150737701e40cd8406aa182f6744690fbd9a2062a8234516` |
| P8 candidate | `make protoss-p8` | `e502991119e7d7cbb22f4a4ba6c39ed597d47461a8b76f554efb20cec0f49394` |
| P9 candidate | `make protoss-p9` | `34b2e8d3486deef22a0264d242bac31be4fb17d5590da2ceb8ecd5e541cc4b5f` |
| Showcase | `make showcase` | `93f25e38ef07e9b43cfdb41ab3d7383b72cd1738388ef85986e240f8825b87b9` |
| Zerg Z7 | `make zerg-z7` | `3729a62463c4644ab5c787ef637f9fd9a8590e27ed6dfb4f5719c32091f06514` |
| Terran T1–T10 | `make terran` | `7087f9b0ede4cbf90e2fc13b991937c7d0afd4da98079922cdd347b5589e8346` |

All seven are byte-identical. Every fact-table header was regenerated from
each tree's own source. The frozen 2026-10-31 copies predate #155's P9 and
Scarab gates, and a first comparison that let the base reuse them gave a
different canonical P7 image. This branch adds the P10 gate to the
generator's fingerprint list, which changes the generated header text but
not the code that consumes it.

## Reproduce

With the private P1–P10 inputs already prepared, and without regenerating
any shared input (pass the prepared headers to `make -o`):

```sh
make protoss-p10 SC64_AUDIO_DIR=.local/sc64/audio-codec2/generated
make test-sc64-protoss-p10 SC64_AUDIO_DIR=.local/sc64/audio-codec2/generated
```

`./dev sc64` does not list `protoss-p10`; use the make targets above.

## What is not qualified

- **No ordinary-input win.** No controller has been written for P10. With no
  input the mission is lost to the first Zerg waves. Mission completion is shown only
  with the kill fixture above.
- **No ARM evidence.** The P10 ROM builds and passes its memory guards, but
  no emulator or hardware run, replay or SRAM continuation has been made.
- **One save point.** Only the idle pause-menu save at frame 8,000 and the
  in-process epilogue save are checked. Saves with selections, open menus,
  construction or combat in progress are not.
- The AI backend, trigger cadence and Transmission duration are authored.
- Raynor's base cannot grow, and the Gantrithor has no attack (see above).
- Performance and frame rate are not measured.
- The candidate is not promoted. Canonical Protoss remains P7.

## P9/P10 cursor preflight follow-up

At source `6b0d91bff2a562dde09c930ff83dd9cb35857258`, source-aware cursor bounds are checked on P9 and P10 candidate states before load commit using the same pure predicates as live validation. HOST fixtures check the last valid and first invalid action cursor for each applicable runner and source trigger; they do not construct guest SRAM images. The P9 fixture's `--cursor-preflight` runs only this focused check, without claiming an ordinary-input victory.

The focused P9 and P10 carried startup/save matrices each passed 234 skirmish configurations; the P10 mission HOST fixture and bounded P9 cursor check passed. P9 ARM ROM SHA-256 `c210f663ad1b04e0f3674700e8f3b2c2d4e09e2c43878f38e3856dab8da25d67`, ELF `f8cb3c84a45d402f9fc8c1f32da7b57a9ac5f7b69f70bcd59b7c256d0af0d773`, BSS/IWRAM ends `02038d7c`/`03004a48`; P10 ROM `21801b16b7e37a68667810072089740767a9a20b06a7086846557492edbb79f5`, ELF `d60f1e57904af581e75987839d75cbba0710d528897f0f51615652641baf5cae`, ends `0203913c`/`03004a50`. These are this candidate's profile-specific inputs and binary receipts, not current experimental ROM identity claims. No long P9 controller or P10 ordinary-input win was run.

Clean unprepared source-boundary (1406 indexed files), doctor, default build and test passed under the native lock; Python reported 873 tests with 23 skipped, plus separate suites of 5 and 8 passing. Two earlier prepared-input dry-runs stopped before compilation because walk-pair and walk-word auxiliary inputs had not been marked old; their logs were retained. The successful run pinned these existing auxiliary inputs and did not regenerate shared assets. Evidence remains private under `/tmp/sc64-p10-cursor-final-6b0d-results/`.

## Current release integration qualification

Source `440718c21af3047b2dab41558b37103928a72850` composes the P9/P10 candidate and cursor preflight fixes with released source `c210775ec48096902231184c89913f80d24247a2`. P9 and P10 remain explicit candidates; canonical Protoss stays P7. Their current Transport1 profiles use distinct ROM/save basenames and do not automatically import legacy SRAM.

Fresh authored fact headers were generated in isolated profile outputs with the current P9/P10 and shared transport flags: P9 SHA-256 `0ec816c56d3ed693fc4659085a2abf53cbca2f47ab4b6c29622a9ce630b2a455`, P10 `bd13f2f29a5e49c4f97124c0ed627a5476eb98583f277eca4072d1d747df5a7e`. Pinned private inputs and generated facts were checked before and after; no private assets were regenerated.

Both carried startup/save matrices passed 234 configurations each, the P10 mission HOST fixture and bounded P9 cursor fixture passed, and both ARM verifiers passed. P9 Transport1 ROM SHA-256 `86bee8a6fe838047f25bb69d5532e6b659684d2e4d189dd149f611f9978f992c`, ELF `f1d19450652e61282c5a1ca3e0aa24d722b6f125e7c800ad672acd14fbdb7666`, BSS/IWRAM ends `020390bc`/`03004a58`; P10 ROM `b126f9ad346034376ceeff4aa94b2654b8967312b732f2225b67ea41ca3cbab4`, ELF `58884decf4d27cf22a05d898e617f4fc32eee31093deece44c1f4956354156ff`, ends `0203947c`/`03004a58`. These checks do not establish an ordinary-input P10 win, full source AI fidelity or physical-device performance.

A separate clean unprepared worktree passed source boundary (1541 indexed files), `./dev doctor`, `./dev build` and unchanged `./dev test` under the native lock; Python reported 951 tests with 26 skipped, plus separate suites of 5 and 8 passing. The long P9 controller was not rerun. The qualification logs and private input receipts remain local.
