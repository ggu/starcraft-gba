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
