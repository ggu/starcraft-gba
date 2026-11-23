# Facility flood placement on the mining-depth release

The default-off `CANONICAL_ROUTE_FACILITY_HOT64` experiment moves the
64×64 facility expansion kernel into Thumb IWRAM. It keeps the existing
bounded frontier, packed directions, visit order and cache behavior. The
64×64 point flood is split so its expansion loop remains in IWRAM while
its initialization and packing stay in ROM. No point-prefix state is
included. The option has its own build-configuration key.

This qualification rebases the placement onto `ba9ee409`, whose runtime
matches the mining-depth release. The flag-off Z10 ROM reproduces
`7e23b74eb9a8ebeb75c13aba3a1a7c4ce86d7c1fc891e37893f527532150ca2a`.
The enabled ROM is
`f3f95f004fff8ca689b74fea20e53c2bfbfe75d2ecca6d6ac6e6a6b01ab32b1c`.
All four flag-off builds reproduce their pinned release ROMs. All four
enabled builds pass the static link guards. The tightest is Showcase,
with IWRAM ending at `0x03004fe0`, 32 bytes below the guard. The runtime
stack watermark remains unverified. The asset-free facility FIFO
fixture passes with both flag settings under ASan and UBSan.

## Busy Blood Bath

The paired normal Z10 replay resumes the same ordinary pause journal and
advances 3,600 updates. All route build and hit counters, 31 sampled world
and framebuffer checkpoints, 16 fresh-continuation checks, and the pause
journal match. The input is the same three-CPU Blood Bath used by the
[current profile](2026-11-21-blood-bath-current-profile.md).

| Guest cycles | Control | Placement |
| --- | ---: | ---: |
| Mean | 923,198 | 921,310 |
| Nearest-rank p95 | 1,216,520 | 1,217,300 |
| Peak | 3,806,440 | 3,622,086 |

The peak falls 4.84%, the mean falls 0.20% and p95 rises 0.064%. In the
middle 1,200 updates, p95 rises from 1,337,079 to 1,349,247. The comparison
receipt SHA-256 is
`d2144ec6122c2390df80d678a89af6bd46189ff799253cf212dc76988f46ad88`.

## Outpost production tape

An ordinary input tape advances 18,743 controller calls through an active
Outpost match. All 95 sampled world, indexed-page, active-page and palette
checks match. The match remains in progress at the final sampled frame,
18,592. The same 800-update production window is measured in each build.

| Guest cycles | Control | Placement |
| --- | ---: | ---: |
| Mean | 1,243,696 | 1,239,157 |
| Nearest-rank p95 | 1,654,794 | 1,657,585 |
| Peak | 2,669,640 | 2,261,634 |

The window peak falls 15.28%, while mean falls 0.37% and p95 rises 0.17%.
The comparison receipt SHA-256 is
`5d350ba0a30a7ab0febd0aa91aed449699d9e87298fa3a8b6cb5778eeba40b6a`.

Product-specific ordinary Outpost tapes also compare the normal Terran and
P10 ROMs. Terran matches 90 sampled world, two-page and palette checks.
The displayed-page bit differs at call 13,820. Its terminal observation
is playing at frame 1,065. P10 matches 85 sampled world, two-page and
palette checks. The displayed-page bit differs at call 11,442. Both P10 runs end in defeat
at frame 15,799. The 800-call production windows have the following guest
cycle measurements:

| Cartridge | Mean, control → placement | p95, control → placement | Peak, control → placement |
| --- | ---: | ---: | ---: |
| Terran | 1,082,053 → 1,081,461 | 1,396,560 → 1,399,838 | 2,046,771 → 1,895,711 |
| P10 | 1,282,827 → 1,281,868 | 1,602,102 → 1,600,096 | 2,543,998 → 2,252,466 |

The sampled peaks fall 7.38% and 11.46%, respectively. The Terran and P10
comparison receipt SHA-256 values are
`d35cc14d48513b644bb4831d5f6246a1304662047789fbb15413fb4b6261937f` and
`d95b21e487a9ac13dfba44dfb26b0d8f841f7e720d207c21eb4176365a9d6476`.
At each differing observation the two indexed page hashes match, but
the displayed-page bit differs. Flip timing has not been traced there.

These measurements cover emulated guest cycles. Keep the option off
while qualifying the tight Showcase stack, further maps and late-game
loads, complete save/load paths across products, and physical hardware.

## With the Carrier frame roster

Repeat the 3,600-update Z10 Blood Bath comparison after enabling the
[Carrier roster](2026-11-21-carrier-frame-roster.md). The facility-off
ROM reproduces the qualified roster ROM exactly. The facility option
changes mean guest cycles from 880,942 to 879,058 (−0.21%), p95 from
1,171,673 to 1,175,411 (+0.32%) and peak from 3,760,529 to 3,569,137
(−5.09%). All 31 world and display samples, 16 fresh continuation
samples, route counters and exported pause bytes match. The comparison
receipt SHA-256 is
`c24d256cbfec20f1f342317b0629be1717cc961c6cc3bafe8f372c7e510165fe`.
This paired trial covers the Z10 busy match.

## Isolated placement on the later combined source

A local trial on `14e32cbb` moves only the current 64×64 facility
expansion function to IWRAM. It uses the pinned Z10 build vector
with Melee and the batched target index. The linker rejects the
trial at the existing 11 KiB stack reserve. The older preview also
split the dense point flood and placed its setup and packing in
ROM. That composition needs a fresh test against the current
dense route code. The isolated trial produced no new timing result.

The isolated link log SHA-256 is
`9373a3bbf329e4f98a4712272c13ab0fbd6e01df2bbdac3c1a0a4e62ce446851`.
