# Canonical Zerg Z1–Z6

Canonical `zerg` now builds a six-entry Zerg campaign. It exposes Z1–Z6
through the normal campaign frontend and keeps both skirmish maps, every
human/AI race and one to three independently selected AI opponents. With
Terran 1–10 and Protoss 1–5, canonical coverage becomes 21/30. Later chapters
remain staged; this is not complete campaign or native-engine equivalence.

## Products, identities and saves

| Product | ROM / ELF | Title / code | Save generation | Emulator saves |
| --- | --- | --- | --- | --- |
| Canonical `zerg` | `StarCraft-Zerg-Z6-GBA.gba` / `starcraft-zerg-z6.elf` | `SC ZERG Z6` / `SCZ6` | 32 (`0x53435a50`) | `.local/play/zerg-z6` |
| Explicit `zerg-z4` | `StarCraft-Zerg-Z4-GBA.gba` / `starcraft-zerg-z4.elf` | `SC ZERG Z4` / `SCZ4` | 20 | `.local/play/zerg-z4` |
| Explicit `zerg-z3` | `StarCraft-Zerg-GBA.gba` / `starcraft-zerg.elf` | `SC ZERG Z3` / `SCZ3` | 5 | `.local/play/zerg` |

The Z4 product keeps its flags, tests and files; its ROM is byte-identical to
the same-source baseline. Generation 32 is the Z6 storage/transport layout
also used by the internal `zerg-z6-probe` (`SCZE`). The two use identical
storage and runtime flags, so the layout is genuinely shared, but they have
separate ROM codes and emulator save folders. No file is relabelled and no save
is converted; Z4 and Z6 loaders reject each other's generations.

The canonical flag vector is the Z6 storage/runtime/query vector
(`Z6_PROBE_FLAGS`) with three changes. A word-packed copy of the six-chapter
sprite bank (`.local/sc64/zerg-z6/generated`) takes the probe bank's place.
The canonical drawing, Zerg tuning, minimap, directional-sprite and
source weapon-effect flags are added, matching the Z4 product on base.
Like P5 Shuttle and Z7, it also restores the defense-target lists
(`-DSC64_ZP_DEFENSE_TARGETS`) that Z4 already uses. With the source weapon
effects inlined, the full-scan defense fallback would otherwise sit in IWRAM
and end it at `0x03005138`, past the guard. Preparing
`zerg` now also generates the Z5/Z6 programs and that bank. Readiness checks
require the chapter 5/6 programs, the Z6 map gate and the real packed word
payload, not just markers.

## Z5 and Z6 source audit

Both chapters run their authenticated source trigger programs. Victory and
defeat are the source gates, with nothing relaxed.

**Z5** (global 16; 40 triggers, 155 actions, 44 conditions):

- Triggers fire on Command, Bring, Commands-the-most/least, Switch and Elapsed
  Time conditions. Selectors are the current player (13), All Players (17) and
  Forces 1–2 (18–19).
- The extraction is trigger 0: Bring Kerrigan (raw 51) into location 20. Its
  41 actions include PauseGame, transmissions and switch 110. Trigger 1
  (switch 110) is the victory. Trigger 2 (Force 1 commands at most zero
  Kerrigans) is the defeat.
- Preserved triggers 7, 8 and 34 keep polling.
- There is no AI script. Doors, traps, detectors and doodad states come from
  the source.
- Kerrigan's cloak and Consume use the authenticated ability facts. Consume
  is limited to the authenticated human target roster. Ensnare remains
  modelled but not integrated.

**Z6** (global 17; 9 triggers, 48 actions, 13 conditions):

- Trigger 2 requires no Force 2 units in source locations 4 and 5.
- Trigger 4 requires Kerrigan on the island (location 1), with at most one
  unit in location 6.
- Trigger 5 is the victory: after trigger 4, no player still commands raw 79.
- Trigger 0 (the player commands no units) and trigger 6 (Kerrigan dead) are
  the defeats.
- The Zer6 AI script is an authored paid-production and order policy, not
  native AI bytecode.
- Hallucinated Tassadar actors have a bounded lifetime, deal no damage and
  take doubled damage.
- Kerrigan starts with full energy and has Cloak and Psionic Storm. The source
  disables Consume.
- Ventral Sacs research and Overlord transport use the authenticated
  transport facts.
- Ensnare and Shield Battery recharge remain incomplete.

These are bounded GBA adaptations of the source programs. They do not execute
native AI or reproduce native timing.

## Why the staged controllers failed, and the repairs

The game was not at fault; both failures came from controller assumptions that
the current source no longer met. The repairs change only controller
strategy. Objectives, enemies, resources, statistics and mission state are
untouched, and every world change is an ordinary key input.

- **Z5.** On the current route, Kerrigan meets a detector on the extraction
  path (raw 203). After she destroyed it, the controller left her idle
  instead of resuming the source goal. Its stall guard then marked the
  extraction blocked, and it chased optional gates. After gate 0 started,
  one cursor loop could not progress while the source PauseGame froze the
  field. The controller now resumes the goal after a destroyed detector, and
  once gate 0 starts it only acknowledges the pause and waits for the source
  victory.
- **Z6.** After the stock unit profiles were aligned with the pinned
  references, an enemy Reaver's 8-tile range now covers the fixed post-Storm
  rest point, and it killed Kerrigan there. The controller now holds her at
  the nearest walkable point no attacker reaches while she recovers energy.
  The slow unupgraded Overlord also outlasted fixed 3,000/6,000-tick ferry
  waits, so those waits now follow flight progress.

## Normal-input qualification

The six host controllers run under the canonical flags with ASan/UBSan
(`make test-sc64-zerg-carried-missions`). Their recorded key transitions
become input-only replay tapes. A private driver runs those tapes from a
fresh boot through the title menu, one `game_update` per tick, with no state
writes. The same tapes win on the host and on ARM in isolated mGBA processes
(no debugger), ending on identical frames:

| Mission | Input updates | Transitions | Final frame | Result |
| --- | ---: | ---: | ---: | --- |
| Z1 | 43,604 | 1,602 | 43,438 | won |
| Z2 | 7,735 | 732 | 7,537 | won |
| Z3 | 51,653 | 1,217 | 51,360 | won |
| Z4 | 59,270 | 812 | 59,111 | won, outcome WON |
| Z5 | 50,301 | 896 | 49,786 | won, outcome WON, Kerrigan alive |
| Z6 | 309,357 | 10,859 | 305,772 | won, outcome WON, fault 0, Kerrigan alive |

Compiler dependency files for every replay ROM resolve `game.c`, all
`starcraft/` headers and `common/` to this checkout. Every private header comes
from this worktree's own ignored inputs, and the sprite bank is the canonical
word-packed bank; neither probe bank is referenced.

## Fresh-process SRAM continuity

Each round trip uses two separate mGBA processes. The first replays ordinary
input to a mid-mission checkpoint, then pauses with START and saves with B.
The second process boots fresh, loads with title-screen B, and finishes the
same tape.

- **Z5:** saved at update 25,000 / frame 24,684. The fresh process restores
  identical world, semantic and full Z5 chapter-state hashes, then wins at
  frame 49,786, the same as the uninterrupted run.
- **Z6:** saved at update 306,500 / frame 302,941, while Kerrigan was
  loaded in the Overlord mid-flight. The fresh process restores identical
  world, semantic and full Z6 chapter-state hashes, including the cargo link.
  It then unloads and wins at frame 305,772, the same as the uninterrupted
  run.

Both round trips were rerun at the tested commit, with identical hashes. The Z6 SRAM image is
byte-identical before and after the save-validator change below.

## Z6 save cost

The first ARM pass showed that every Z6 save cost about 93–109M cycles, or
roughly six seconds on GBA timing. That included the automatic victory save
and the pause-menu save. A no-GDB diagnostic ROM timed each stage of that
save at update 20,000. The candidate validator, `race_z6_save_candidate_valid`,
took about 100M cycles. It re-decoded every packed unit, building or mineral
slot for each source row and for each live unit's census.

The validator now records each unit and building slot's identity as part of
the all-slot decodes it already performed. Mineral slots are cached the
first time they are visited, so a decode failure surfaces at the same point
as before. Every key holds only fields that the decoder already bounds to the
packed width, so comparisons are exact and accept/reject results do not
change.

| Diagnostic at update 20,000 (pre-merge build) | Before | After |
| --- | ---: | ---: |
| Z6 candidate validator | 100,229,093 | 6,121,690 |
| Whole `save_game()` | 109,467,683 | 16,864,162 |
| Peak save-path stack | — | 3,632 bytes (7,432 of the 11 KiB reserve unused) |

The Z6 save, game-save, transport, spell, runtime, controller and profile host
tests still pass. Their transactional-rejection cases cover malformed
candidates.

## Performance

All figures are mGBA emulated cycles for the full ordinary-input route, one
`game_update` plus render per row. Columns give the mean update, draw and
full-frame cycles, the worst 1,000-update window mean, the worst single
update before the winning one, the winning update itself (which includes the
automatic victory save), and the share of updates over 561,792 cycles (two
59.7 Hz frames). The baseline is the legacy Z4 product's flag vector and inputs on the
same tapes, built from the same checkout. Z5 and Z6 have no Z4 baseline.

| Mission | Product | Mean update | Mean draw | Mean full | Worst window | Worst in-mission update | Victory update | Updates over 561,792 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Z1 | Z4 baseline | 234,147 | 337,176 | 653,426 | 1,017,139 | 8,938,825 | 6,682,872 | 45.0% |
| Z1 | Z6 | 258,128 | 374,069 | 706,531 | 1,052,812 | 9,107,145 | 11,626,630 | 63.0% |
| Z2 | Z4 baseline | 260,547 | 337,066 | 695,558 | 779,788 | 7,014,346 | 5,629,248 | 52.7% |
| Z2 | Z6 | 310,792 | 377,596 | 752,680 | 865,457 | 7,047,535 | 10,386,646 | 66.5% |
| Z3 | Z4 baseline | 361,838 | 329,702 | 776,967 | 1,330,618 | 16,110,685 | 7,108,061 | 75.4% |
| Z3 | Z6 | 422,643 | 367,985 | 878,102 | 1,392,679 | 16,276,353 | 11,161,713 | 88.4% |
| Z4 | Z4 baseline | 294,269 | 345,929 | 727,348 | 897,481 | 4,356,649 | 6,000,150 | 87.4% |
| Z4 | Z6 | 414,220 | 386,221 | 869,437 | 1,019,320 | 4,467,748 | 11,356,804 | 99.7% |
| Z5 | Z6 | 533,684 | 302,825 | 931,768 | 1,226,121 | 5,207,785 | 12,537,676 | 99.0% |
| Z6 | Z6 | 453,742 | 348,453 | 869,018 | 1,809,270 | 12,180,086 | 17,148,202 | 92.8% |

On the same tapes, the six-chapter product costs 8.1%, 8.2%, 13.0% and 19.5%
more mean full-frame cycles than the Z4 product for Z1–Z4. It also roughly
doubles each victory update, because the larger save carries the Z5/Z6 state
and its validators. The in-mission worst updates are mission-load and
scripted-event spikes that both products share.

Z6's 1,809,270-cycle worst window is the heaviest in the campaign; it falls
early in the mission. Before the save change, its victory update was
93,097,165 cycles.

Before the base merges, the same defense-target flag was measured as an
experiment. It cut Z4's mean by 8%, added about 2% to Z1 and Z5, left Z6
unchanged, and gave identical wins. It is now part of the canonical vector,
for the IWRAM reason above.

None of these numbers is a hardware measurement or a frame-rate claim.

## Memory and identity guards

| Check | Z6 product | Limit |
| --- | --- | --- |
| IWRAM end | `0x03004f18` | `< 0x03005000` |
| BSS end | `0x0203de78` | `< 0x0203e000` |
| Save payload | 16,296 bytes (`sizeof(RaceSave)`) | `<= 16,368`, static assert |
| ROM | 33,554,432 bytes, SHA-256 `d54d7fee…2bb9a5b` | `<= 32 MiB` |

The legacy Z4 ROM rebuilt at the tested commit is 33,554,432 bytes, SHA-256
`fafa8018801ddbbcc220cee7e2c865e0de84d883e88027f41f4e1d2d5ff2b986`. That is
byte-identical to the canonical Z4 build from `codex/source-release` at
`6f440a4`, which includes directional sprites, source Tank effects and the
Protoss warp-in glow. The
explicit Z3 ROM is identical to base as well
(`57b309a48f4f11310cf6b4fffec603b1e4b660cbd8af3ec749f8913031e453ed`).

## Commands run at the tested commit

All of the following ran at `9ea062d18cf7d37a29e8963f4eddb8cf38378427`, the
merge of `codex/source-release` at `6f440a4`. Every
command exited 0.

```sh
python3 tools/check_source_tree.py
./dev doctor
./dev build
./dev test
make zerg zerg-z4 zerg-z3
make test-sc64-campaign-spells test-sc64-z6-runtime test-sc64-z6-controller \
     test-sc64-z6-profile test-sc64-z6-save test-sc64-z6-transport
make unified && make test-unified
make test-zerg-z4
make test-zerg
python3 -m unittest tests.test_sc64_dev
```

`make test-zerg` runs the following:

- The six carried mission controllers under ASan/UBSan.
- The split skirmish profile: 18 launches covering two maps, three human
  races and 1–3 AI opponents.
- The promoted startup/save matrix: 234 configurations.

The ARM replays and SRAM round trips use private, uncommitted drivers built
from the recorded tapes. Their logs, ROMs and saves stay in ignored local
folders.

## Remaining limits

- **Coverage:** canonical coverage is 21 of 30 chapters. Zerg 7–10 remain
  staged, and their probes are not part of this product.
- **Source approximations:** the Z5/Z6 approximations listed above remain.
  These include authored AI, incomplete Ensnare and Shield Battery recharge,
  and bounded hallucinations.
- **Frame time:** emulated frame time is over a two-frame budget for most
  updates in Z4–Z6 on both products. On Z1–Z4, the six-chapter product is
  8.1%–19.5% heavier than the Z4 product.
- **Victory saves:** each victory save remains a single ~10–17M-cycle stall.
- **Hardware:** no 30 fps or hardware performance claim is made.
- **Inputs:** the replay tapes come from host controllers. Each one is one
  winning ordinary-input route, not proof that every route is winnable.
