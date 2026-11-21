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
