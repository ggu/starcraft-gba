# Dense Zerg 7: measure same-goal retention before spending RAM

The read-only mGBA observer from [#492](https://github.com/ggu/starcraft-gba/pull/492)
replayed calls 47,758–48,237 on the exact November 13 KODAK Z10 ROM.
All 131 checkpoints matched. The 480 updates produced 448 point fills
across 308 updates, to 11 distinct goals, with no topology resets. All
fills came from the ordinary miss at `0x0802c8a0` in the pinned ELF.
None had a resident field with the same ordered walkable seeds, so the
blocked-goal alias would not help this workload.

The stock 14-slot bank kept identity physical mappings and had no donor
planes. Of 448 misses, 319 replaced a fresh complete point field. An
offline sequence retaining one, two or three evicted point fields
would have matched 51, 92 or 132 subsequent misses (11.4%, 20.5% or
29.5% of all point fills). It tracks only this verified point-miss
callsite, excludes victims of facility and range work, and preserves
logical eviction. It does not run the GBA copy or a candidate build.

A retained 128×128 packed plane costs 6,144 bytes. First test one plane
on the same tape and measure the extra copies, paired game state and
all production link reserves; compare it with the wide FIFO already
profiled on this window. More than one plane would exceed historical
P10 EWRAM headroom. The replay result is an opportunity estimate;
neither a speedup nor default activation is established.

| Artifact | SHA-256 |
| --- | --- |
| Exact KODAK Z10 ROM | `e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907` |
| Stock ELF | `74c4b2394c4aee619bf2ea0549c31de7987827a915c32c5c50e7a4fec58ad6c1` |
| Authenticated HOME tape | `e1c7693d633f1ecd486036e797baea555dcaddf1e0a409d4da397f3b2f3fbe17` |
| Register observer | `b0b9abdf79604b7ebca7f877c3b1e8774d0a4c19e0ffe2996fbd054ecc0ae1f6` |
| Stock seed receipt | `96c975a51fc0c4e54a234abe1195850120227c479ea46b94c0d83bfd1380f3b7` |
| Retention receipt | `8f0a39d5b70702e6ce4572911199186100dfe8bb4d0eac57a40a69ada902b162` |

## One-plane ARM result: do not enable

A Z10 probe preserves the stock logical insertion and adds one private
field for point misses on 128×128 maps with no stock donor planes.
On a miss it can retain a fresh point victim; on a match it copies or
swaps the packed bytes. It invalidates on route resets and geometry
changes. The asset-free sanitizer fixture compared the retained result
with an independent BFS across all 16,384 cells, checked the logical
miss counters, and verified that topology invalidation forces a fill.

The paired ARM build uses the same KODAK source vector as the wide FIFO
probe and adds only the retention flag. All 131 checkpoints, the route
build and hit counters on all 480 calls, and raw world, grid and logical
field bytes at both endpoints matched the wide FIFO. Of 480 calls, 477
advanced the simulation. Mean sampled active draw/update cost grew
**0.55%**; orders grew **0.89%**. The 95th-percentile call fell from
4.704M to 4.567M cycles, but the worst call grew from 5.970M to 6.096M.
The dense speed falls from 5.54 to 5.51 emulated updates/s. Z10 passes
its guards but uses another 6,152 bytes of EWRAM; IWRAM is unchanged.
Skip the copy-based scheme in production. The wide FIFO and narrower
floods that can prove the same route answers remain better candidates.

| Artifact | SHA-256 |
| --- | --- |
| Retention candidate ROM | `5fd918f89087f1780202cc03d59cb17d0388b4f58f50ccc53608e3fcce867a10` |
| Matching ELF | `5e63aa55632f7bfd2f8674d75f5e22b1fad3f4dbe2f96cf2edda3c8316714b68` |
| Paired timing receipt | `982716b63b52b23526f15fb1d750323a00fccadcef30491474301512bb943f2f` |
| Shared endpoint state receipt | `12665bdcd588a704e58296b07cdc5aad4d87cc5c4679007881b52f77740a0cd4` |

## One-plane pointer retention on the current FIFO

The default-off `SC64_ROUTE_POINT_POINTER_PROBE` rotates one spare
backing pointer into a safe logical slot on a 128×128 point miss.
The victim's bytes become the spare without a copy. It preserves the
logical slot, key, age, counts and eviction order, and excludes the two
slots backing the fixed range scratch planes. A reset or grid rebuild
invalidates the retained key. On asset-free ASan/UBSan tests, the recovered
bytes match an independent BFS, the logical build still increments, and
both range scratch pointers remain fixed across an actual plane claim.
A 386,646-query differential also matches the logical residency receipt.

The paired Z10 build adds only this probe to the production pointer FIFO.
All 131 checkpoints, all 480 frame and route counter samples, and raw
world, walkability and valid route-field bytes at both endpoints match.
Of the 480 calls, 477 advanced the simulation.

| Guest cycle counter | Pointer FIFO | Pointer retention | Change |
| --- | ---: | ---: | ---: |
| Whole loop, active mean | 2,941,972 | 2,858,242 | −2.85% |
| Movement orders, active mean | 1,784,487 | 1,700,683 | −4.70% |
| Whole loop, all calls p95 | 4,420,534 | 4,053,982 | −8.29% |
| Whole loop, all calls maximum | 5,583,925 | 5,361,778 | −3.98% |

All four 32 MiB ARM builds verify and pass the unchanged linker guards.
The tightest EWRAM margin is 1,816 bytes on Protoss P10. IWRAM does not
change on any of the four products. Keep the probe off pending fresh
process SRAM continuation and physical input parity on the other wide
maps. The measured improvement is in guest cycles for one Z7 interval.

| Product | EWRAM end | IWRAM end |
| --- | ---: | ---: |
| Terran | `0x0203b468` | `0x030048a8` |
| Zerg Z10 | `0x0203a310` | `0x03004d50` |
| Protoss P10 | `0x0203d8e8` | `0x03004c10` |
| Showcase | `0x0203b424` | `0x03004e60` |

- Candidate Z10 ROM: `1152e11de4b6bf2426558f8fb33af015215563b2ce1f89ddfec6224d25aff869`.
- Paired dense replay receipt: `e92822c5dc324e492065127a63b471cd88d6478add9dea18850055d31b5bb074`.
- Matching raw endpoints: `12665bdcd588a704e58296b07cdc5aad4d87cc5c4679007881b52f77740a0cd4`.
