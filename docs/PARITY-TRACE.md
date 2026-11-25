# Parity trace and command injector (GBA side)

The N64 parity harness compares this engine with StarCraft 64. N64 behaviour is
captured once into fixtures; the checks after an engine change run on the host
only. This page covers the GBA half: a command injector, a per-step trace
emitter, a demonstration runner and its self-gates. The comparator, golden
files and baseline are separate work.

Everything here is host-only. Two shipped headers gain one hook each, and both
hooks compile to nothing unless a host build defines them (`GAME_AUDIO_REQUEST`
in `starcraft/game_audio.h`, `SC64_TEXT_BOX_POSTED` in
`starcraft/sc64_text_box_runtime.h`; either one defined without `GBA_HOST` is
an `#error`). `game.c` is unchanged.

| File | Role |
| --- | --- |
| `tests/sc64_parity_inject.h` | Semantic command verbs |
| `tests/sc64_parity_trace.h` | Trace emitter and the schema |
| `tests/sc64_parity_run.c` | Runner: menu launch, scenario, trace, world hashes |
| `tools/sc64_parity_selfgate.py` | Self-gates and throughput |
| `mk/parity/Makefile` | Builds, included from `mk/skirmish-controller/Makefile` |

## Running

The profiles need their prepared private inputs. Zerg Z10 needs the same
descriptor override as its ROM.

```sh
make Z8_DESCRIPTOR=.local/build/z9-inputs/z8-descriptor.json test-sc64-parity
make Z8_DESCRIPTOR=.local/build/z9-inputs/z8-descriptor.json bench-sc64-parity
```

Each parity binary compiles the simulation its shipped ROM runs: the
preprocessor options (`-D`, `-U`, `-I`, `-include`) are read from a dry run of
the ROM ELF's own recipe (`build/starcraft-terran.elf`,
`build/starcraft-zerg-z10.elf`) into `build/parity/<profile>.romflags`, so
target-specific additions such as `SC64_NATIVE_MELEE_STARTS` and
`SC64_PERSISTENT_COLLISION_BUCKETS` carry over. Host flags replace the ARM
code-generation options.

`test-sc64-parity` runs the four self-gates on the ASan builds of both
profiles and writes `build/parity/<profile>-selfgate.json`. Each result holds
the gate verdicts, the four runs' reports, the exact compile command of each
binary (`<binary>.flags`, written after a successful build), and the SHA-256
of every generated input header its dependency file (`<binary>.d`, from a
separate `clang -M` pass over all its sources) names. A change to the compile
command rebuilds the binary (`<binary>.cmd`).

One runner run:

```sh
build/parity/terran-fast-trace --scenario t1 --steps 3000 --trace t1.trace
```

The runner hashes all route state (`route_purity_hash()`) around every update,
scenario dispatch and trace step. That costs about 1 ms per update, more than
the step itself; `--no-route-checks` drops those per-update checks for
throughput runs (the verbs keep their own). The gates never use it.

## Time

A *step* is an update in which the simulation advanced. Menus, the result
screen and updates while a text box is open take none. Source frames are
`step * 4 / 5`, rounded down (5 updates per 4 source frames). The trace also
gives the number of `game_update()` calls (`host_tick`) and the engine's
`frame`. Known limit: P4's PauseGame freeze advances `frame` without combat or
production and is counted as steps.

The parity builds keep the host default for text boxes: a box opens and is
acknowledged inside the update that posts it, so `boxes` in `S` reads 0 and
the trace's `X` record is the evidence that one opened. A box takes no
simulation step on the device either, so step numbers match the ROM; only the
count of `game_update()` calls differs, and that count is not a parity
timebase (`tests/sc64_text_box_game.c` holds the modal and host builds to the
same world). The runner also works with `SC64_TEXT_BOX_HOST_MODAL`: it
dispatches each step's commands once and stops with exit 4 when no step is
taken for too long, but it does not press A to dismiss a box.

## Verbs

| Verb | Engine path | Profiles |
| --- | --- | --- |
| spawn | `spawn_unit` | all |
| move, attack-move | `issue_order`, one selected unit | human units |
| attack a target | `sc64_direct_attack_issue_target` | `SC64_DIRECT_ATTACK` builds (Zerg Z10) |
| attack a target (fallback) | `issue_order(ORDER_ATTACK_MOVE)` at the target | builds without it (Terran) |
| stop | `stop_selected`, one selected unit | all |
| harvest minerals | `sc64_worker_mineral_issue_selected` (builds without `SC64_WORKER_CARGO_KIND`: the same assignment the A-on-minerals command makes) | human workers |
| harvest gas | the A-on-refinery assignment | human workers |
| train | `race_queue_unit` | `SC64_RACES` |
| build | `race_start_build` | `SC64_RACES` |

Order verbs select exactly one unit, so `issue_order` never spreads a
formation, and restore the selection, the selected building and the toast line
afterwards. Orders for computer-owned units are not supported: the human
order functions ignore them. A refusal (no money, no supply, bad site, full
queue, missing prerequisite) is a setup failure; the runner exits 3 on any.

Terran builds have no targeted attack order (`SC64_DIRECT_ATTACK` is off in
the shipped Terran vector). The product's own A-on-enemy command is an
attack-move at the cursor, and the verb does the same; the trace header says
`attack=attack_move_fallback`.

## Trace format

`sc64-parity-trace/1` is line-oriented text. The full schema, field by field,
is the comment at the top of `tests/sc64_parity_trace.h`; in short:

- `S` per step (step, source frame, host tick, frame, screen, won, lost,
  text-box held outcome, campaign phase, countdown, open boxes);
- `U` and `B` rows per unit and building, written when any field changes,
  with `+U`/`-U`/`+B`/`-B` births and removals and `OU`/`TU`/`OB`/`TB` owner
  and type changes found by diffing the tables between steps;
- `P` banks and supply for owners 0..4, `M` mineral amounts;
- `FV`/`FE` keyframes over the active map size and `F` cell changes of the
  human fog and explored sets, read through `SC64_GRID_GET`;
- `V` death-effect pool rows, and facing, walk phase and an animation kind in
  each unit row;
- `D` hits from `SC64_TEST_HIT_OBSERVER`, `A` sounds requested before the
  on-screen, budget and voice gates, `X` text-box rows posted (a repeat post
  of an open or queued row leaves no record);
- `I` injected commands and `Z` totals with the world hash. A trace is
  complete only with its `Z` line showing `overflow=0 write_errors=0`; the
  self-gate rejects any other.

Presentation fields advance only while `game_render()` runs, so traces come
from runs that draw after every update, as `common/main.c` does.

## Throughput

`make bench-sc64-parity` reports host updates per second of process CPU time
(wall time on a shared, loaded machine mostly measures other processes), best
of three, with `--no-route-checks`. Measured 2026-11-26 on Apple silicon; the
Z10 skirmish ends at step 13,522, so its fast runs are shorter.

| Profile | Build | Draw | No emitter | Emitter |
| --- | --- | --- | ---: | ---: |
| Terran T1 | `-O2` unsanitized | off | 376,600 | 80,707 |
| Terran T1 | `-O2` unsanitized | on | 114,376 | 52,128 |
| Terran T1 | ASan + UBSan `-O1` | off | 149,132 | 23,112 |
| Terran T1 | ASan + UBSan `-O1` | on | 34,672 | 15,200 |
| Zerg Z10 skirmish | `-O2` unsanitized | off | 249,705 | 18,649 |
| Zerg Z10 skirmish | `-O2` unsanitized | on | 82,842 | 15,508 |
| Zerg Z10 skirmish | ASan + UBSan `-O1` | off | 173,617 | 13,377 |
| Zerg Z10 skirmish | ASan + UBSan `-O1` | on | 31,880 | 10,052 |

The emitter costs more than the simulation: every step it rebuilds each live
row to compare it, scans every fog cell, and formats text. The skirmish trace
is larger (about 1.8 KB per step against 250 bytes for T1) because buildings
there change every step. With the per-update route checks on, as in the gates,
every build runs at about 1,000 updates per second.

## Self-gates

| Gate | Check |
| --- | --- |
| same trace | two fresh processes give the same trace SHA-256 and world-hash chain |
| trace on/off | a build with the emitter and hooks and one without give the same world hash and per-step chain |
| route purity | `route_purity_hash()` is unchanged around every injector call, scenario query, emitter call and between updates (the runner aborts otherwise) |
| render on/off | drawing after every update or not gives the same world hash and chain |

Two guards keep a pass meaningful: every run must reach the requested step
count still in play (`full_length`), and the traced builds must report the
emitter compiled in and the plain ones without it (`builds_differ`). A trace
counts only when its footer, step ids, event totals, command records and body
hash all agree with the run's report, and a binary only when the input hashes
recorded at its build still match the files and its own SHA-256.

The world hash covers units and buildings by named field, minerals, fog,
banks, supply, the clock, the outcome, campaign phase and the route state, so
it can be compared across processes. Route state is hashed region by region
like `route_purity_hash()`, except that every 8-byte-aligned word pointing into
route state is hashed as the region and offset it points to: route statics
hold such pointers (the field table, the spare point plane, facility
frontiers, and the pointer FIFO the linear flood keeps in `bfs_queue`), and
their values change with each process load. `SC64_PARITY_REGION_HASHES=1`
makes the runner print one raw hash per route region, to locate a region that
differs between two runs. It leaves out presentation state: facing, walk bits, the
death pool and the audio snapshot.

Sounds are not covered by the render gate: with no draw no sound is ever
requested. Protoss warp-start sounds are requested inside
`game_audio_foundation()` and are not recorded yet.
