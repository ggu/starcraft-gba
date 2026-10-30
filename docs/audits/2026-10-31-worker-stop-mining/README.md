# Idle workers and explicit mineral commands

The shared update no longer assigns every idle worker to the globally nearest mineral field every 30 frames. An empty stopped worker stays idle until another command. With selected workers, A on a currently visible, nonempty mineral field issues a mine order to each empty selected worker. Loaded workers retain their existing order and cargo; a loaded-only selection receives a refusal. A new mineral order clears the previous route timer and retry, so a stale MOVE path cannot delay the gather order. Existing gas, hostile attack, placement, and friendly-selection priorities remain. The default and race HUDs identify the mineral action as MINE.

The original-art outpost has five explicit opening mineral assignments; imported skirmish already assigns its starting workers. This does not alter existing producer-birth assignments. The current STOP command still clears a worker's carried resource, and the engine currently uses the worker's order to determine cargo type. A correct loaded-STOP representation requires separate work. This change does not implement source-style depletion fallback, owner exploration memory, rally behavior or an N64 parity claim.

On shared base `404913652f336160177db7cd6da6832d6ba7b997`, a host fixture using actual Terran, Zerg, Protoss and Showcase product flags seeds a worker and field, then uses native A and B/menu input: STOP stays idle for 120 updates and A on a visible field restores mining. It also checks hidden-field rejection, loaded-gas preservation on mineral click, mixed loaded/empty selections, stale route reset, attack-mode priority, friendly selection and MINE feedback. The original-art source-only ASan/UBSan fixture checks all five opening jobs and that a subsequently spawned idle worker remains idle; it also checks the default HUD action. These are focused host tests, not full mission, ARM timing, save-resume, or hardware qualification. All 104 prepared input hashes remained unchanged. Private inputs and logs are not included.

Full campaign controllers, custom-game matrices, actual ARM products and save continuations remain required before integration; removing the previous automatic job assignment can affect workers after construction or depletion and AI economy. Z7 completion remains unproven.

## Combined controller default qualification

Clean indexed source `2ffee1d` passes source boundary, doctor, default build
and the complete default test workflow: 782 Python tests (23 optional skips),
eight ancillary cases and C fixtures. The first current attempt passed Python
and ancillary checks but hit ENOSPC compiling a later depot fixture. Its log
is retained; a retry passes after only completed project-owned host test
executables and matching debug bundles were inventoried by SHA-256 and
retired. ROMs, ARM ELFs, inputs, saves, sources and test evidence remain.
Exact commands, exits and source/tree identities are in
`worker-default-qualification-20261031/.local/qualification-current`.

Production runtime is still `83875b4`; subsequent published changes adapt
T1/T2/T5/T8/T9 controllers and add trace tooling/documentation. Retained host
controllers pass T1--T5 and T8--T10; T6/T7 remain unqualified. This default
pass does not promote the change or replace the remaining gameplay checks
above. Later audit-only changes do not alter tested runtime or controllers.

## Integrated normal-product qualification

At `ce84be3`, the original full Terran host aggregate passes all ten mission
controllers, its mission save/repair checks, and 234 startup/save configurations.
The original four-product menu workflows pass 72 terminal ordinary-input
matches (14 wins, 58 losses, no incomplete cases) and 96 same-process host
save/reload continuations. Both maps, all human races, 1--3 opponents and
rotated opponent races are selected through normal menu inputs. These are
state-observing host policies, not blind-human or ARM replay claims.

The four original normal ARM Make products at that source pass header,
32-MiB size, ELF payload, padding, prepared audio and strict memory guards.
No Scarab or profile-cache experiment is enabled. BSS/IWRAM ends are:

| Product | BSS end | IWRAM end |
| --- | --- | --- |
| Terran T10 | `020370b8` | `03004c08` |
| Zerg Z6 | `02037124` | `03004cd8` |
| Protoss P7 | `020382e4` | `03004ab8` |
| Showcase | `0203df38` | `03004e38` |

All these workflows freeze the exact 104 existing prepared inputs with
Make `-o` prerequisites because fresh checkout timestamps otherwise request
regeneration. Dry runs contain no generators; original recipes and ordered
BC/CV5 feature vectors remain. The 104 inputs and 95 exporter/tool sources
match before and after. Post-build compiler dependency scans record all 314
consumed dependencies; only the recorded frozen inventories have independent
before/after hashes. Private receipts are under `terran-aggregate-ce84-20261031`,
`worker-skirmish-matrix-20261031`, and `normal-products-ce84-20261031` in
their ignored `.local` directories.

`make unified` passes at `ce84be3`. Its test first exposed a stale direct
call to the removed `economy` helper. Test-only child `1d215ff`, equivalent
to reviewed `d737c78`, runs the real active-play update from frame 29 to 30
and retains the idle-carrier assertion; the full original `make test-unified`
then passes with the same 104 inputs unchanged. Game code and the four normal
ROMs above are unaffected by that fixture correction.

These checks supersede the earlier integration hold for this limited worker
change. Source-style local depletion fallback, persistent cargo identity and
the other resource-order limitations remain separate work. Fresh-process
SRAM, normal-menu ARM input replays and hardware/performance are not newly
qualified by this host/build stage. Canonical campaigns remain T10/Z6/P7,
23 of 30 missions; this change does not add missions or establish 30 fps.
