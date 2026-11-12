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

Two more 1,000-update custom windows compared the normal Terran and Protoss
cartridges directly with their opt-in counterparts. The sampled states
matched for both pairs, but neither shows a useful mean speedup:

| Product | Mean loop cycles, stock → probe | p95, stock → probe |
| --- | ---: | ---: |
| Terran | 1,126,888 → 1,126,882 | 1,450,767 → 1,450,903 |
| Protoss | 1,497,523 → 1,497,864 | 1,896,164 → 1,885,863 |

These windows are different workloads from the Z10 campaign. Keep the
defense flags off by default; the observed campaign gain does not imply a
gain in skirmish.

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

## Current Z10 with packed sight

The current Z10 pair uses the same runtime source at `20469c2`, with the
packed-sight build inputs from `88eddef`, packed walking, and the ordinary
Z10 release vector. Only the two defense switches differ. The cartridge
continued from a 32 KiB SRAM image written by ordinary buttons with the
current supply rules. Its physical input tape reached the victory screen
at controller call 61,938, game frame 59,997, with nine text dismissals and
zero mission faults. All 61 sampled raw gameplay states and 24 sampled
drawn pages, palettes, camera positions and fog states matched the
packed-sight-only ROM.

| Normal Z10 replay window | Mean loop cycles, packed sight → combined | Mean update cycles, packed sight → combined | p95 loop cycles, packed sight → combined |
| --- | ---: | ---: | ---: |
| Calls 19,000–19,999 | 1,802,117 → 1,709,213 (−5.16%) | 920,970 → 828,140 (−10.08%) | 3,069,981 → 3,046,926 |
| Calls 35,000–35,999 | 1,791,044 → 1,660,412 (−7.29%) | 960,807 → 829,835 (−13.63%) | 2,809,013 → 2,817,431 |

Both normal ROMs passed the verifier and the original linker guards.
EWRAM ends at `0x02038ae8` in each, 21,784 bytes below its guard.
IWRAM changes from `0x03004ae0` to `0x03004be0`, leaving 1,056 bytes.
Mean loop cost falls in both windows, while the second window's p95 rises by
0.30%. Both windows remain above the two-refresh CPU budget.

Packed-sight baseline SHA-256:
`0f144b2789f695658e09dd721d8b7349c29aacbde7edce93f476ce29b9c47608`.
Combined SHA-256:
`5af463b0f416ce2997eaef0d776dc2319538541174376d95853fd30915c219a7`.
The common SRAM SHA-256 is
`740ffc1f24454f67bdf3ca954850016a47e672040fa33035bdef1412bedc1888`
and the common controller tape is
`bea076602f09b1c8496b8c3dfd5d49640be3a8352946763178f0ef40a8329310`.

The focused combined sanitizer test also passes with packed sight. It now
compares delayed projectile rows and queue counters as well as actor
state, and exercises a stale cell entry after a same-pass death, invalid
owner IDs, fog, and a full impact queue with two powered Cannons.

The instrumented build `fc84319e631a6f4bd6fd24e238c9d211466098424b7038c665e60fc00fbc086a`
matched the normal candidate at all 61 campaign checkpoints. Its lowest
written user stack word was `0x030072f4`: 2,828 bytes below the initial
stack pointer and 8,948 bytes untouched above the guard. This includes the
ordinary SRAM load. It does not measure the IRQ stack or execute the
malformed-owner fallback. The normal candidate's disassembly shows a
1,432-byte spatial dispatcher frame and a nested 912-byte scalar frame
on that fallback, which is separately exercised by the host sanitizer test.

Both normal ROMs also played the same complete three-AI custom matches by
physical buttons. The games matched at every 250-call observation through
defeat, including raw actors and each owner's wallet.

| Custom game on Z10 | Outcome frame | Matching observations | Mean loop cycles, packed sight → combined |
| --- | ---: | ---: | ---: |
| Outpost, Terran vs. three AIs | 18,621 | 76 | 1,333,704 → 1,337,614 (+0.29%) |
| Blood Bath, Protoss vs. three AIs | 18,986 | 77 | 1,205,218 → 1,209,206 (+0.33%) |

Both custom samples cover calls 10,860–11,859. These current results apply
to Z10 with the listed settings; they do not qualify other cartridge
layouts or combinations with additional render changes. Release flags are
unchanged.

```sh
make LATE_ZERG_PACKED_SIGHT=1 test-sc64-z10-defense-combined
make LATE_ZERG_PACKED_SIGHT=1 SC64_DEFENSE_PROBE_FLAGS='-DSC64_DEFENSE_SPATIAL_PROBE -DSC64_DEFENSE_PYLON_ROSTER' verify-zerg-z10
```

Use separate clean build outputs when changing the defense flags.
