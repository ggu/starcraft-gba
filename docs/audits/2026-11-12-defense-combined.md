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

## Full Z10 win and skirmish checks

The exact timed ROM continued from the same ordinary SRAM save to the
victory screen at controller call 60,834, game frame 58,795, with zero
mission faults. Sixty checkpoints matched the qualified ARM witness's
mission, frame, screen, result, wallet and fault fields. The first 34 also
matched the stock normal cartridge's raw arrays. The runner used only
physical buttons, including nine ordinary text dismissals.

The pinned 64×64 Z10 custom battle window matched all frame/screen rows and
21 sampled actor-count rows, with mean loop cycles 1,368,916 → 1,369,303
(+0.03%). This is a scoped performance sample, not a full custom match.

The same matrix cartridges each launched six custom games by physical
buttons: one on each map for each AI count from one to three. Across the
24 launches, all three human races and independent AI race choices were
exercised, including duplicate AI races. Every selected owner, starting
roster and wallet matched the setup; all runs then advanced 120 frames.
This establishes selection, loading and early play on all four products.
Two full three-AI physical-input matches on the exact Z10 probe also
reproduce the corresponding host losses and final actor counts for each
owner: Terran on Outpost at frame 18,621 and Protoss on Blood Bath at frame
18,986. Those are selected complete outcomes; long-running behavior across
the wider profile matrix remains a separate gate.

The combined Protoss host build passes the carried P1–P10 startup/save
checks and all 234 custom startup/save lineups. The full Eye of the Storm
controller wins at frame 52,969 with zero faults; 22 fresh-process
continuation checkpoints match the uninterrupted host replay, including
victory. The Gantrithor continuation and outcome text fixtures also pass.

The normal P10 probe ROM then loaded that 32 KiB save through the title and
won with physical buttons at the same frame 52,969. All 23 observations
(load, periodic and victory) match the host witness's frame, result,
mission, fault count, wallet, Overmind and Zeratul HP and mission switches.
There were seven ordinary text dismissals, no guest writes, and the input
save was unchanged. The P10 ROM SHA-256 is
`7ccf696f06afac355ca98cecb37ad4b3bcb76f0f0c1f0004553f29da1d0f1b07`.

The Terran host full campaign suite also passes with the two probe flags:
the early regression group, late world/save/repair fixtures, and T7–T10
controller victories. T7–T10 each report zero mission faults. This
complements the signed-owner and wide-coordinate Pylon fixture; it is
not a T10 cartridge stack or long-skirmish measurement.

## Combined rendering and sight experiment

The defense probe was also built with [packed walking sprites](https://github.com/ggu/starcraft-gba/pull/400)
and the [packed sight rows](https://github.com/ggu/starcraft-gba/pull/401).
Against the original stock ROM, the same 1,000-call windows average
1,439,766 and 1,735,312 loop cycles (−8.18% and −13.05%). Mean simulation
cost falls by 14.77% and 20.62%. All 34 gameplay checkpoints and the 24
drawn pages, palettes and fog observations match. This was measured as one
build; the savings were not added from separate probes. Packed walking
is now on the release branch; sight and defense are still gated.

The fully composed experimental ROM SHA-256 is
`caaa8acabfaab7a10a8d7f201bf503c6a5aad18e5dbb4efa49a4c09c986ed718`.

With the same prepared inputs, use a clean build tree:

```sh
make test-sc64-z10-defense-combined
make CANONICAL_WALK_WORDS=0 SC64_DEFENSE_PROBE_FLAGS='-DSC64_DEFENSE_SPATIAL_PROBE -DSC64_DEFENSE_PYLON_ROSTER' verify-zerg-z10
```

An instrumented Z10 ROM, using the same combined runtime, painted unused
user stack at boot and repeated the same physical-input replay. All 34
checkpoints still match. The lowest changed canary word was `0x030072f4`:
2,828 bytes below the user stack top, with 8,948 bytes untouched above the
`0x03005000` guard. This bounds the observed stack writes on this replay,
including the SRAM load. It does not measure the IRQ stack or exercise the
malformed-owner fallback. Instrumented ROM SHA-256:
`8cadcba3aee619d12fd691c7d586baeb0ec66191a6ddd74334dc0164a4b8cc34`.

The shared Pylon lookup now retains each building layout's coordinate width.
The wider Terran host sanitizer fixture passes, including a coordinate
that would alias after narrowing, exact range boundaries, and signed invalid
owner fallback. All seven opt-in ROM verifiers pass with the original
linker guards. The Z10 ROM from this matrix is byte-identical to the timed
combined cartridge.

| Cartridge | EWRAM room below guard | IWRAM room below guard |
| --- | ---: | ---: |
| Terran T10 | 17,336 B | 2,232 B |
| Zerg Z10 | 5,704 B | 1,056 B |
| Protoss P10 | 7,992 B | 1,136 B |
| Showcase | 17,396 B | 544 B |
| Legacy Z7 | 1,564 B | 152 B |
| Legacy Z8 | 348 B | 1,056 B |
| Legacy Z9 | 5,704 B | 1,056 B |

Both windows still exceed the two-frame budget of 561,792 cycles. Do not
infer a hardware frame rate from these CPU cycle measurements. The source
probe can be retained with normal defaults off. Enabling it requires
long skirmishes and stack checks in the wider cartridge matrix.
The spatial lookup currently replaces the Zerg/Protoss campaign owner-list
pass; the scalar, P3 and late Terran target policies remain separate.
