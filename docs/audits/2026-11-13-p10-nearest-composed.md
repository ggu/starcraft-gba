# Nearest depot selection on composed P10

The earlier [standalone nearest-first comparison](2026-11-13-p10-route-options.md)
passed the native linker and ROM guards, matched its sampled world and pixels,
and won the full ordinary P10 tape. It did not preserve derived cache identity:
the selector can skip farther depot queries and change which fields remain
resident. Its P10 default stayed off pending broader qualification, with no
observed native, gameplay, or performance failure in that run.

This bounded comparison adds only `SC64_DEPOT_FIRST_NEAREST_PROBE` to
the exact [composed P10](2026-11-13-composed-performance.md) runtime at
`153ae6b0`, with facility FIFO, Command census, and typed save reader.
Word packing is off on both. The candidate links and verifies as 32 MiB.
IWRAM ends at `0x03004c70`, BSS at `0x0203c0e8`; data and BSS sizes
match, and text grows 208 bytes.

The ordinary HOME prefix through call 32,645 matches 13 HOST checkpoints,
ten briefing pages, seven authored acknowledgments, and eight ordered
samples of both indexed pages, palette, fog, cursor, actor and mineral pools,
and campaign phase. Only the route metadata, active packed fields, and route
build/hit counts are allowed to differ. The actor digest includes worker
home slots and orders. This is a bounded composition check, not a composed
victory or a save qualification.

| Call | Whole update, control → nearest first | Facility fill, control → nearest first | Route builds in update |
| --- | ---: | ---: | ---: |
| 9,674 | 19,775,810 → 6,208,992 | 17,363,836 → 4,205,145 | 41 → 16 |
| 11,045 | 19,246,413 → 4,945,299 | 16,986,428 → 3,164,886 | 38 → 12 |
| 11,046 | 1,702,308 → 1,693,729 | No fill | 0 → 0 |
| 32,644 | 24,649,859 → 11,809,466 | 16,734,571 → 4,258,286 | 44 → 18 |

Explicit flip wait stays at 25 cycles in all four traces. The route build
deltas cover all route kinds. Both fewer builds and cheaper expansion
account for the difference; expansion falls from 5.89–7.18M to
0.068–0.128M cycles across the three spikes. The prefix maximum falls
37.75%, from 24,649,332 to 15,344,401 guest cycles, while its mean falls
0.77%, from 1,639,374 to 1,626,691. This improves the sampled stalls
without materially changing the sustained update rate.

Both composed builds loaded the same retained KODAK pause journal through
physical L at frame 36,761. The restored world fingerprint
`65a9decfb8bac76b4108a594efc52c05a8203c80e18402d5de5814579646e8fd`
matched the normal ROM, with 213 units and 81 buildings. At all five
sampled suffix checkpoints, world state, both indexed pages, palette and
fog matched, including the victory at frame 46,057. Both exported the
same final journal (`c69ff8edddd990e94f3f9418e5b2198cb47e649a555afb36aca5d6f54de58cbf`).
The paired suffix receipt is
`a5e84f91c6d0d6f9f7eff853018c2a0fb93b543625f3c6f55390b2822e8e6dc5`.
This qualifies the composed save suffix. A fresh full normal HOME replay
and a custom match with multiple active depots are separate gates for
promoting nearest-first on the default P10 cartridge.

- Control ROM: `6ac056503bf28b8ce7af09b3313b90b78f764cfd9a3fd805e75ae12d5bd08c97`.
- Candidate ROM: `cb8990e8b3a5621b3bbaa637e0a6f4c37ff8f5ac4ec5d79cf0fe15f6faba8f66`.
- Control / candidate prefix: `b5d94f99d11842314c6a03f9b7ad4c8d9be887b1f2206a9ecf9de42bb424a5fa` /
  `ddd11a8149c85f66d44cb7afac7c2040d30c4acaccc5fa09f4bc644b5a7f1df1`.
- Candidate ordered state/video: `01e9085c7438909e3faa9838c31f9b29960153a50517b9ee43ff9e1958bc5f13`.
- Profile comparison: `837bd723299cd9d488477c0521e6b6539f3b2cb9863b20d6fed354fde88b9924`.
- Cache-only difference receipt: `904a7944168de36f42e98781d6f70cc1d8f913c4187ac3947ee178216116ec5c`.
