# Combined defense optimization replay

This composes the [Pylon lookup](https://github.com/ggu/starcraft-gba/pull/396)
with [spatial target selection](https://github.com/ggu/starcraft-gba/pull/398)
in one optional Z10 cartridge. The two probes interact within the same defense
pass, so compare the combination against stock rather than adding the
separate speedups. The release defaults remain unchanged.

| Normal Z10 replay window | Mean loop cycles, stock → combined | Mean simulation cycles, stock → combined | p95 loop cycles, stock → combined |
| --- | ---: | ---: | ---: |
| Calls 19,000–19,999 | 1,568,109 → 1,493,832 (−4.74%) | 791,327 → 716,393 (−9.47%) | 1,965,123 → 1,892,766 |
| Calls 35,000–35,999 | 1,995,868 → 1,846,261 (−7.50%) | 992,461 → 843,032 (−15.06%) | 2,375,134 → 2,229,001 |

The physical-input runner uses the same 32 KiB SRAM image and selected
inputs as the [stock baseline](2026-11-12-z10-battle-cycles.md), with no
guest writes. The 34 campaign checkpoints match the stock cartridge's
named state and raw unit, building and mineral arrays. The combined focused
ASan/UBSan fixture compares 800 generated worlds and covers Pylon state
transitions, cooldown retention, cell-boundary ties, exact range and
out-of-domain fallback.

The Z10 verifier passes. EWRAM remains `0x0203c9b8`; IWRAM ends at
`0x03004be0`, up 256 bytes from stock. Combined ROM SHA-256:
`9eab3d5ddc9e7893b82ed902a37059728c50338ce93ff0543cff79c9b180914b`.

With the same prepared inputs, use a clean build tree:

```sh
make test-sc64-z10-defense-combined
make CANONICAL_DIRECT_FIRING_FLAGS='-DSC64_DIRECT_FIRING_POINTS -DSC64_DEFENSE_SPATIAL_PROBE -DSC64_DEFENSE_PYLON_ROSTER' verify-zerg-z10
```

Both windows still exceed the two-frame budget of 561,792 cycles. Do not
infer a hardware frame rate from these CPU cycle measurements. Promotion
requires the dynamic stack check, complete ordinary-input campaign and
custom comparisons, and links and behavior checks for the other products.
The spatial lookup currently replaces the Zerg/Protoss campaign owner-list
pass; the scalar, P3 and late Terran target policies remain separate.
