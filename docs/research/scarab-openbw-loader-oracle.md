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
