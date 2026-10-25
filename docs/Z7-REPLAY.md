# Z7 replay validation tooling

`tools/sc64_zp_controller_replay.py --profile zerg-z7` prepares the shared
replay tool for a private seventh-entry Zerg controller trace. It checks
the normal HOME/menu route to global mission 18, compact actor-bank layouts,
and all fields of the bounded Z7 persistent runtime using the paired ELF's
DWARF offsets. It changes neither the game nor canonical campaign menus.

A full replay requires a winning trace. Qualification requires the source
victory trigger to finish without defeat or adapter faults, zero enemy
buildings, and at least one surviving player unit or building. Surviving
enemy units are allowed by that source objective. The observed ending actor
census uses source owners 3 and 4 and verifies their player/enemy bindings;
it does not infer ownership from the collapsed gameplay side alone.

For debugging a failed or limited attempt, explicitly supply both
`--attempt-prefix` and `--ticks N`. The endpoint must precede the terminal
event, and this mode forbids `--save-at-end`. It reports a prefix with
`verified_win=false`; it cannot qualify as a completed campaign. Queue-page
capacity is checked for the replayed prefix while the complete input trace
still undergoes structural validation. Recorded prefix observations do not
establish full-state parity with the host controller.

The runner requires a matching privately prepared scheduler ROM, its ELF,
and a controller trace; none is distributed in this repository. For example,
replace the private paths and endpoint below with a paired local experiment:

```sh
python3 tools/sc64_zp_controller_replay.py --profile zerg-z7 \
  --trace .local/experiment/attempt.json \
  --rom .local/experiment/probe.gba --elf .local/experiment/probe.elf \
  --attempt-prefix --ticks 1000 --output .local/experiment/replay
```

Optional `--expect-rom-sha256`, `--expect-elf-sha256`, and
`--expect-trace-sha256` arguments bind a run to its frozen inputs. The runner
uses mGBA's debugger and the existing replay scheduler; do not run it while
another task owns that debugger endpoint. Private reports, captures and saves
remain in ignored output paths.

Asset-free parser and decoder checks run with:

```sh
python3 -m unittest discover -s tests -p 'test_sc64_*controller_replay.py'
```

These checks use synthetic fixtures and establish tooling behavior. They do
not establish a Z7 mission win, an ARM replay result, fresh-process SRAM
continuation, native AI timing or GBA performance.
