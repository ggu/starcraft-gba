# Two-flyer foreground picking

When two friendly same-layer flyers overlap, the shared painter orders their bodies by its depth key. The previous unit-only picker chose the nearer anchor, which could select the body visibly behind the other. This change uses the painter key only when exactly two eligible units and no building are in range, both are flyers on the same layer, both body masks cover the visible cursor, and their current poses are supported. Other cases retain the previous proximity result. This follows the existing GBA painter; native sprite pool-ID lifetime parity remains open.

The Terran framebuffer fixture renders the Wraith/Mutalisk pair in both slot orders and verifies both backbuffers. At world pixels (94,43) and (92,51), respectively, the old picker selected slot 0 while the visible painter selected slot 1; 76 opaque, distinguishing pixel cases exposed the mismatch. The updated fixture checks ordinary selection plus transparent-pixel, fog, unsupported-attack-pose, hostile-owner, third-contender and offscreen fallbacks. Existing foreground, context and Z10 worker gather input checks pass under ASan/UBSan.

On the exact #694 Z10 3,600-update three-CPU Blood Bath pair, 31
video and state cuts, 16 fresh continuation cuts, both pages, palette,
route counters and SRAM match. Mean cycles are 840,021 and 840,110.
The p95 values are 1,132,275 and 1,132,358. The local paired receipt
SHA-256 is `050407d095d86ebe01b6f85a5359dde7c203c490da676607e49d48cd037b61a1`.

The final rebase onto the packed sprite release `43c5d98` reproduces
all four previously qualified composition ROM hashes. On this exact
Z10 pair the same replay matches its world, both pages, 16 fresh
continuation cuts, route counters and SRAM. Mean cycles are 832,982
and 833,082. The p95 values are 1,123,557 and 1,124,208. The busy tape
tests unrelated gameplay and frame cost. The staged opaque click
test exercises the changed selection. The packed comparison receipt
SHA-256 is `8c01a6dd74246ef6111a490172207f70e3097a77c7fc397dad02cde781f5d980`.

## Moving same-type flyers in the double-tap window

The ordinary Z7 tape matches through call 16,375 on the pre-fix
`43c5d98` cartridge and the current `c05ac442` Zerg cartridge.
Two friendly Mutalisks overlap at the cursor. The first A edge at
16,376 picks slot 100 on the old cartridge and foreground slot 104
on the current one. At the second A edge (16,378), the current
picker returns slot 100. The double-tap code requires the same
slot, so the current route enters attack mode with a single
Mutalisk. The old route enters with both. The later gas order
becomes movement because the Extractor has been destroyed.

For the replay harness, retain calls 16,375–16,382. Record the
cursor, opaque body masks, painter keys, both framebuffer pages,
picked slot and selected raw types at each A edge. Verify that a
single click selects the foreground unit. After a second click
within the existing time and cursor bounds, verify that overlapping
friendly units of the same type can still be selected as a group.
Cover different types, a second click outside either bound, fogged
or hostile actors, and retirement and reuse of the remembered slot.
Compare native overlap and repeat-selection inputs separately.

The shared input fix compares the second picked actor's selection
type after checking that the remembered actor remains live, owned,
selected and eligible. It preserves the original timing and
distance bounds and the foreground result of the first click.
With the fix, call 16,376 still selects slot 104 and call 16,378
groups slots 100 and 104. Both remain selected entering attack mode
at 16,382. The asset-free sanitizer fixtures exercise matching and
differing types, expiration, dead and reused actors, a changed type
and a change in ownership.

On the combined Wasteland and input release, the full ordinary
Zerg mission 7 tape wins at frame 228,128. A third process loads
the victory from HOME and exports the same journal. All 254
world observations, ten two-page display cuts and six route
cache cuts match the earlier corrected build. The existing
raster difference at call 154,198 remains when compared with
the older normal-combat cartridge.

The four exact ARM links pass the existing RAM guards and match
their Wasteland-release openings and 41 camera cuts per product.
The three race ROMs pass all 54 ordinary Custom Game starts and
fresh save continuations. Wasteland wins again after a save,
restart and resume. The late Protoss 10 victory and busy Zerg
Blood Bath cuts match their controls in sampled state, both
pages, palette and SRAM.

| Local evidence | SHA-256 |
| --- | --- |
| Pre-fix click pair | `655230d7740fb389a42d06d4b0104910d5812384ae0ba5930eead7312f958469` |
| Current click pair | `3db26ef336bbc93564235bbec07bb052a6b5797163f823895058652641b7584b` |
| Pre-fix, current and corrected ordinary click comparison | `73310720e933762c7ddf22d4e1987ed5b11684defe02d6083e45790783b1b89d` |
| Exact combined ordinary click observation | `eef52a17019cee79a73eaff808fef8bba6d07d8709f8476849bbe78dbdcae99b` |
| Exact combined Zerg mission 7 replay | `549ffe1d66604004f2d1f4b7bd7bd6e96e2a820c8d273bc316e9ad9f73296a5b` |
| Exact combined Custom Game continuations | `f1d9231e5f96d377efc0073f0ac8a26e313a3be8f39f9af75de0594a15e24e4c` |

## Opaque flyers on different layers

The [bank-backed framebuffer fixture](../../tests/sc64_z3_frame_coverage.c)
places a layer-18 Wraith over a layer-16 Overlord. At one shared
opaque pixel, the nearer anchor belongs to the Overlord, while
the Wraith paints the visible pixel. The earlier picker chose the
Overlord because its painter override required equal layers.

The two-flyer path now compares the painter keys across layers.
It still requires two eligible flyers, a visible cursor and
supported opaque bodies. The prepared Terran ASan/UBSan check
matches 24 distinguishing pixels over both slot orders and checks
fog, hostile ownership, a third contender, an unsupported pose
and a transparent pixel. This case uses established layer keys.
Native sprite-ID ties need the separate native replay case.

The 18 ordinary Zerg 7 calls around the moving same-type
selection match on the cross-layer control and candidate.
The Command-cache composition keeps the same 9,489-call
Protoss victory, matching 68 world, video and fog cuts, the
entire route-counter history, SRAM and total guest cycles.
The busy Z10 Blood Bath pair matches all observations and
reported cycle statistics. The ordinary Wasteland pause,
restart and load route reaches victory at frame 10,120.

The three race cartridges pass 1,134 ordinary Custom Game
launches. The 378 per cartridge cover both maps, all human
races and every nonempty arrangement of closed or named race
in the three CPU seats. Each launch checks the resolved seats
and the actual town hall locations. A separate 54-case matrix
passes fresh HOME save continuations. The four links, ordinary
openings and 41 camera cuts per product pass.

| Local evidence | SHA-256 |
| --- | --- |
| Earlier picker mismatch | `a7fec1217fbe7ed970dab9fd23f0d3252119b3c36b3c53704a13c1ab305a2915` |
| Prepared cross-layer fixture log | `e62403f88944180dbc9f00c369e643fd745ca0b1ea8d6ab1e93dcd1eb7d0f0fe` |
| Ordinary moving-flyer control comparison | `dba1518218ce964f8a76af6849144bb7f28d7d2302fe73e2fee240f5bc5b437f` |
| Composed normal P10 victory | `c1ab64503684e336e849e6314e5775a92e53fa1fe5417818b7c1aa7beb8e01e8` |
| Composed busy Z10 Blood Bath | `db40d9a2b2d3d8013f4bda946c7a9b0e8879050f33c19e2af8a203d7811fe17d` |
| Composed normal Wasteland victory | `0d02e7c9ae5637c352a9184bc701cef77c21c3c82492002051327d197d1f2713` |
| Exhaustive named CPU launch matrix | `4c5fbb23eab788bd137f838457dc04c3a2b4279a4c4bc567229241ba63341952` |
| Ordinary Custom Game fresh HOME continuations | `e27612b0531b26a47153c3749ba2770f357a68217c0e46c786aae3d03d29d133` |
