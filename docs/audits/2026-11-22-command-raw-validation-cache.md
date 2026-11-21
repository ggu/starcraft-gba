# Command census raw validation

The direct Command census validates every live unit and building before filtering by owner or type. It previously recomputed the same immutable source extents for every actor on each query, although Command does not consume those extents. The cache stores the mapped type or failed validation by raw ID and actor kind for one census call. It is cleared before each call, so later trigger actions and fresh loads see the current pools. Owner and negative-coordinate checks still occur on every actor; out-of-range raw IDs use the original validator. Compile-time checks restrict this optimization to compact coordinates and mapped types that fit the table. The shared static scratch costs 456 EWRAM bytes; no state is saved.

`CANONICAL_COMMAND_VALIDATION=1` enables `SC64_COMMAND_RAW_VALIDATION_CACHE` in the existing canonical direct Command census vector. Set it to `0` to reproduce the original census. `CANONICAL_COMMAND_CENSUS=0` retains the generic provider and omits the cache in either setting. The configuration key distinguishes both direct-census variants. Terran and Showcase have no direct Command provider and retain their previous ROM bytes.

The existing actor-provider differential now also compares the original and cached validators over raw IDs −1 through 229, both actor kinds, owner boundaries and each combination of coordinates −1, 0, 1 and 32767. It passes with ASan/UBSan for prepared P10 and Z10 vectors, including the existing malformed rows, ownership, same-slot, Egg and provider-fallback cases. Its local log SHA-256 is `1fc821ba6c94f7eee294b71150db4dcd452c646aab04c0962cbefb2a680234af`.

On the prior collision composition, the pinned P10 physical tail matches 68 world, page, palette and fog observations, every frame and route-counter row, victory and SRAM. Active mean cycles decrease 1,605,713 to 1,575,970 and p95 2,249,019 to 2,149,883. The collision Z10 three-CPU 3,600-update replay matches the same state, pages, route counters and SRAM with effectively unchanged mean cost. Those bounded observations are not physical-device frame rates.

On Wasteland runtime `1c958d4`, all four prepared ARM links pass ROM and RAM guards. Terran and Showcase remain byte-identical; disabling this cache reproduces both P10 and Z10 control ROMs. P10 ends EWRAM at `0x0203b130` and IWRAM at `0x03004e10`; Z10 ends at `0x02039870` and `0x03004ef0`. With an instrumented stack canary, the P10 saved campaign tail reaches victory and retains at least 6,504 untouched bytes above the reserved IWRAM boundary; its 68 world and video observations and SRAM match the prior collision control. A fresh-loaded Z8 on the Z10 cartridge retains at least 7,216 untouched bytes over 1,200 neutral updates. Independent 120-update instruction observations confirm both the census and the cached validator execute in the P10 and Z8 windows. These are bounded canary observations, not a proof of all paths or stack depths.

| Local receipt | SHA-256 |
| --- | --- |
| Collision P10 comparison | `a44201ff74c396c27c853e05793b9ebf4d24d2f22a41b3f81c72242a2fa6f6a7` |
| Collision Z10 comparison | `a0bd8990f847501a2dbc8424702249b0559168cd11726416a3ccc7a5fd2e2c03` |
| Wasteland-base P10 canary tail | `80c6a1a574c34d6b54c8643817ac542315faac8a035bbf6b4cb1893dd02a3ec3` |
| Wasteland-base Z8 canary | `60e9ae7f86961ab6d0b8fe3930d1d556417346e5d156ab431fb5a8c7cd4fb489` |
