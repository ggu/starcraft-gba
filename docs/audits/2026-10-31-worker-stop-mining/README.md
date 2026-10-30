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
