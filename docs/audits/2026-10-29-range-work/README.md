# Opt-in bounded ranged approach routing

`SC64_RANGE_WORK` is experimental and OFF in all normal products.
`CANONICAL_RANGE_WORK_FLAGS=-DSC64_RANGE_WORK` opts the Terran, Zerg, P7 and
Showcase builds into it. Default-off ROM bytes match main133 exactly.
Do not enable it for a release based only on the isolated checks below.

The existing bounded direct/corner firing helper still runs first. Obstructed
ranged ground approaches can request one shared incremental field, with four
FIFO successor demands. Moving-target positions coalesce; old same-target
fields can guide movement, while current weapon and collision checks remain
authoritative. The scheduler has a shared 96-work-unit quota per update.
A unit clears/copies at most 64 bytes, checks a seed, or expands one frontier
node; the quota is not a constant cycle cost or a whole-frame budget.

At the maximum 128x128 map, two existing packed route planes serve as worker
storage. Twelve ordinary fields remain; a retained completed snapshot occupies
one of those LRU slots. Tagged range snapshots are excluded from point-field
and component-label queries. Topology, geometry and load reset work before
reuse; abandoned partial snapshot copies release their slots. Idle updates do
not rebuild the collision grid just for this worker.

## Save boundary and copy phase

A live regression found that discarding a partial job only when loading delayed
unit movement relative to continuing the saved process. On persisted Barricade
terrain, a Tank moved one pixel sooner in the uninterrupted run at tick 41.
After a valid save is written, the opt-in path now resets its derived range
state too. Failed validation preserves active work. No Unit or save payload
grows, and default-off behavior is unchanged.

The new authored codec fixture drives real `game_update` calls and compares
500 consecutive updates with a loaded continuation at both default and
8-unit quotas. Its separate small-quota synthetic case checks partial-copy
reset; that case is not a gameplay continuation proof.

Snapshot copies use aligned word transfers when source and destination can be
aligned together, with byte prefix/remainder and mismatched-alignment fallback.
The sanitizer oracle covers all lengths 0–64, 16 alignment pairs and eight
buffer offsets (8,320 cases), including unchanged surrounding bytes.

## Local evidence and limits

On main133 plus the opt-in adapter, actual-engine sanitizer fixtures pass at
quotas 96/8, including the current direct-order hook and a correctly prepared P7
active-stride profile. The default build and tests pass at the initial adapter
checkpoint (649 Python checks, 20 optional skips, 8 profile/native checks and C
suites). Later changes are opt-in runtime paths and focused test/build targets.

Four normal ON/OFF ARM links retain the existing guards. Showcase ON leaves
24 IWRAM bytes and 16 BSS bytes; state adds 184 BSS-end bytes including 8 data
bytes. Four OFF ROMs remain byte-identical to current main133. The 72 standard
normal-menu skirmish/save/load cases pass with the adapter ON; these are short
economy continuations, not partial-job or full mission qualification.

An isolated actual ARM 128x128 wall test at quota 96 completes a field in 172
worker ticks. Its worst worker operation is about 136,783 cycles (8.15ms), and
individual replan requests reach 88,299 cycles (5.26ms). This does not establish
whole-frame performance or acceptable first-response latency. An otherwise
matching private copy-only variant reduces one completed-field copy from
109,841 to 44,154 cycles (59.8%, about 3.915 ms saved), with unchanged work units,
status and queue checks. The main worker peak is unchanged. There is no
whole-engine FPS claim from this microbenchmark.

Private evidence is under `.local/build/range-work-*` and
`.local/build/range-copy-word-v1/`. The invalid 64x64-Z7/LARGE_MAPS test combination
and an obsolete fixed-duration fixture assertion are retained as failures;
correct P7 active-stride coverage and quota-aware assertions then pass.

With private headers already prepared, the authored opt-in targets are:

```sh
make test-sc64-range-work-flags
make test-sc64-range-copy-alignment
make test-sc64-range-work-engine
make test-sc64-range-work-engine-default
make test-sc64-range-work-save-resume
```

The alignment oracle needs no game assets. The engine targets consume prepared
headers without declaring their import/generation recipes as prerequisites.
Missing headers fail compilation. Current-product fresh-process SRAM,
representative rendering, carried/full missions and worst-case whole-frame
qualification remain activation gates. Catalog remains T10/Z6/P7; all 30
original missions and near 30 fps remain the goal. No assets, ROMs, saves,
captures, private traces or native decompilation are tracked. No Actions,
imported Make recipes, asset generation, GDB or SD writes were used.
