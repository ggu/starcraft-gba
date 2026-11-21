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

| Local evidence | SHA-256 |
| --- | --- |
| Pre-fix click pair | `655230d7740fb389a42d06d4b0104910d5812384ae0ba5930eead7312f958469` |
| Current click pair | `3db26ef336bbc93564235bbec07bb052a6b5797163f823895058652641b7584b` |
| Pre-fix, current and corrected ordinary click comparison | `73310720e933762c7ddf22d4e1987ed5b11684defe02d6083e45790783b1b89d` |
