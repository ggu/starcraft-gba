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

Probe ROM SHA-256:
`02fbc1319442a9ce82dc29e28f1435a3857a15ad1b99211e291991df973f3eee`.

With prepared inputs in a clean build tree:

```sh
python3 -m unittest discover -s tests -p test_sc64_z7_sight_spans.py
make CANONICAL_DIRECT_FIRING_FLAGS='-DSC64_DIRECT_FIRING_POINTS -DSC64_Z7_VISIBILITY_PACKED_ROWS' verify-zerg-z10
```

The current opt-in path is the existing late-Zerg source-sight transition.
Before activation, measure the paired custom workload, qualify full campaign
and legacy paths, and compare the other race profiles' shared-span transition
before generalizing. Do not add the separate renderer or defense savings to
these results without measuring the combined build.
