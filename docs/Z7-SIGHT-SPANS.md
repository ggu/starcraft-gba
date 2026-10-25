# Z7 source-radius visibility spans

`SC64_Z7_SIGHT_SPANS` is a default-off optimization for the experimental Z7
profile. It requires `SC64_Z7_SOURCE_SIGHT` and `SC64_VISIBILITY_SPANS` and
checks that Z7 is active before replacing a source-radius disk operation.
Standard race ROMs and the existing controller/count probe remain unchanged.

The existing source-radius path removes and adds the entire pixel-centered
visibility circle when an actor moves. This option computes exact inclusive
row spans and updates only their symmetric difference for nearby movement.
Full additions/removals also use spans. The 968-byte ROM lookup is derived
from `floor(sqrt(radius*radius - dy*dy))` for radii 64 through 176 in steps of
16; it contains mathematical values, not imported game data. Unsupported
radii and non-Z7 missions keep the existing path. No radius, explored-cell
rule, source-count width, capacity, save field or RAM reserve changes.

With the private Z7 inputs already prepared:

```sh
make test-sc64-z7-sight-spans
make zerg-z7-sight-spans-probe
python3 tools/sc64_z7_trace_parity.py --variant sight-spans \
  .local/build/z7-controller/optimized-baseline.json \
  .local/build/z7-controller/optimized-early-gas.json
```

The two trace paths are examples from [the controller trial](Z7-CONTROLLER-TRIAL.md).
The parity tool compares count-only against count-plus-sight-spans, and stores
variant-specific private outputs to avoid overwriting other comparisons.
The separate ROM is `build/StarCraft-Zerg-Z7-Sight-Spans-Probe-GBA.gba`, title
`SC Z7 VIS36`, code `SCZX`. It does not promote Z7 into a standard race ROM.

The asset-free geometry oracle is also available without game inputs:

```sh
python3 -m unittest discover -s tests -p 'test_sc64_z7_sight_spans.py'
```

It builds scalar, legacy-span and new source-span variants under ASan/UBSan.
An independent full-grid circle calculation checks all 16-by-16 sub-tile
phases, axis/diagonal movement, tile crossings, edge clipping and teleports;
actor activation/death/ownership/loading/type changes; building construction,
morph and completion; and more than 255 overlapping sources plus removals.
It checks visible/explored grids, exact source counts and minimap aggregates,
as well as all 968 lookup values. Positive unsupported radii and inactive-Z7
fallbacks are exercised. Invalid compile-flag combinations are rejected.

## Integrated evidence, 2026-10-25

The base is default branch `ccdd8fe`. Both complete controller traces match
count-only reference state at every update (23,295 baseline and 15,587 early
gas updates). Hash coverage includes incremental visibility snapshots,
per-cell counts, minimap state, actors, source runtime and selected UI state.
Both traces still end in authentic losses, not campaign completion.

A separate private ARM/mGBA pair replays the first 12,000 ordinary baseline
inputs. It retains update/audio/render/display-deadline behavior, with
observer reporting outside the timed interval. All 25 frame/common-state
checkpoints match through frame 11,697, with mission 18 active, callbacks/map
bound and no runtime faults. Only the sight-span option differs between the
pair. Mean 500-update windows show:

| Measurement | Count-only reference | Sight spans | Reduction |
| --- | ---: | ---: | ---: |
| Update, audio and render cycles per update | 1,481,554 | 1,094,200 | 26.15% |
| `game_update` cycles per update | 958,699 | 585,874 | 38.89% |

The pair still exceeds the two-VBlank 561,792-cycle 30 fps budget on 11,814
and 11,815 of 12,000 updates respectively. This is a bounded instrumented
ARM result, not a standard-ROM, mission-wide or physical GBA frame rate.
It does not demonstrate a Z7 win or fresh-process ARM save continuation.

The non-observer probe has unchanged BSS at `0x0203de00`; IWRAM ends at
`0x03004dc8`, within the existing `0x0203e000` / `0x03005000` reserves. It adds
2,072 bytes of text/readonly data and no BSS relative to the count/controller
probe. With the option disabled, both the default original-art ROM and the
count/controller ROM are byte-identical to their base builds. Private
inputs, traces, benchmark ROMs and captures remain outside Git.

The three focused host source-sight/runtime/save fixtures pass, including
normal-menu save/load and transactional rejection with a 16,296-byte payload.
All 450 asset-free Python checks and default host fixtures pass. The complete
replay pair also includes both losing trials' construction, morph, combat,
production and removal states; this remains sampled deterministic evidence,
not a proof over every possible game input.
