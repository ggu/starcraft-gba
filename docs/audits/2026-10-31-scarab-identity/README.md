# Scarab identity prerequisite

`SC64_SCARAB_RUNTIME` appends a distinct raw 85 identity to the existing unit
IDs. Its authored rule has 20 HP, 10 shields, zero armor and supply, 15 minerals,
zero gas, and 105 source build ticks. Producer 255 deliberately leaves ordinary
building production unavailable. This gate supplies identity and field-codec
coverage only; it does not enable Reaver production, Scarab gameplay, rendering,
or world-save migration. In particular, source flags are facts below, not a
claim that the invincibility or movement lifecycle is implemented.

The verified USA ROM's normalized SHA-1 is
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`. Its BOLT `00C/001` units.dat
has SHA-256 `54356d7585c66cd463346996444fd208336eaacf6335d30eb2688aa73c9cb2df`.
The column order and widths follow OpenBW pinned revision
[`4b046d5f65302b10cb0a745f0fecd37ec85b20a8` data_loading.h](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/data_loading.h)
(SHA-256 `5bce24b2f441062f501cbb2fc837a5ac3637ab5dc00c783e276e08ad87a41629`)
and the `xy`/`rect` types in
[`data_types.h`](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/data_types.h)
(SHA-256 `9ce5aa1ed99ce4e75bc929ee30dfeabd6d83aa291173e95048f3a49642b2f1dd`).
Raw 85 has enabled shields 10, Q8 HP 5120, armor 0, ground weapon 82,
air weapon 130 (none), minerals 15, gas 0, and build time 105. Its flags
`0x78000000` include move, turn, invincible and mechanical. These are data
values, not evidence of native N64 runtime behavior. The existing stock
profile provides the ground/air weapon IDs and movement speed for consumers;
no new combat or admission path is activated here.

`python3 -m unittest tests.test_sc64_scarab_fields` checks actual compact,
ZP, derived and P7 typed unit rows with raw 85 and 20/10 canonical values under
four enum capability sets, including the shared roster. It compares every
previous enum value with the gate off, verifies the six-bit limit, and runs
field boundary cases under ASan/UBSan. These row checks are not a complete
world-save, gameplay, or normal-menu test.
