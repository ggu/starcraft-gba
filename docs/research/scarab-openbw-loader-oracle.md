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
