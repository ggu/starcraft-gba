# Packed source-sight rows in late Zerg battles

Z10's [cycle profile](2026-11-12-z10-battle-cycles.md) identified repeated
sight-span arithmetic as a simulation cost. The probe builds the exact
integer circle rows once into ROM. Each byte encodes both offsets from
the actor's center tile or an empty row. The runtime still clips to the map
and applies the same old/new row differences in the same order. Negative
centers and unsupported geometry retain the previous path.

| Normal Z10 campaign window | Mean loop cycles, stock → probe | Mean simulation cycles, stock → probe | p95 loop, stock → probe |
| --- | ---: | ---: | ---: |
| Calls 19,000–19,999 | 1,568,109 → 1,525,574 (−2.71%) | 791,327 → 749,235 (−5.32%) | 1,965,123 → 1,924,902 |
| Calls 35,000–35,999 | 1,995,868 → 1,939,680 (−2.82%) | 992,461 → 936,700 (−5.62%) | 2,375,134 → 2,314,352 |

The paired normal physical-input ROMs match all 34 gameplay checkpoints.
Twenty-four sampled drawn pages, palettes, camera positions, visibility
and exploration also match. The 51,200 generated rows match an independent
squared-distance scan; the asset-free sanitizer test runs the new variant
alongside the five existing full-grid oracle variants. Z10 verifies with
unchanged IWRAM (`0x03004ae0`) and EWRAM (`0x0203c9b8`).

The prepared Z10 fixture also compares the packed decoder with the scalar
row path for all phases and supported radii on both 64×64 and 128×128 map
geometry, including edge clipping. The pinned 1,000-call 64×64 skirmish
window matches its frame/screen and sampled actor counts. Mean full-loop
cycles fall from 1,368,916 to 1,356,372 (−0.92%); p95 falls from
1,726,162 to 1,717,394.

Probe ROM SHA-256:
`02fbc1319442a9ce82dc29e28f1435a3857a15ad1b99211e291991df973f3eee`.

With prepared inputs in a clean build tree:

```sh
python3 -m unittest discover -s tests -p test_sc64_z7_sight_spans.py
make LATE_ZERG_PACKED_SIGHT=1 verify-zerg-z10
```

## Current split-cache cartridge

With the packed walking and split terrain caches already active, the same
two 1,000-call windows average 1,532,236 → 1,491,881 (−2.63%) and
1,914,170 → 1,859,653 (−2.85%) full-loop cycles. Both runs match all 34
sampled gameplay checkpoints and all 24 sampled drawn pages, palettes,
camera positions, visibility and exploration.

The prepared phase/clipping fixture passes. All four Z7–Z10 opt-in
cartridge verifiers pass their original guards. The exact enabled Z10
ROM is `31d54a25fa9b45fd23d8e3b70fa03949f8d9548bf09aa20ef017d92584fbd47c`.
Switching back to 0 in the same build tree rebuilds the normal Z10 ROM
and reproduces its stock SHA-256
`b1a0831adbf2d1b15cf8ae669047be43a64547246815f7a0470eda955252fc9f`.

The normal selector stays off until the enabled Z10 full campaign replay,
a legacy continuation and the current custom workload are qualified.
Other profiles use a different shared-span transition and need separate
evaluation. These measurements still exceed the 30 fps cycle budget.
