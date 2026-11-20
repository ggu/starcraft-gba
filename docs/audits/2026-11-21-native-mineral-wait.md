# Blood Bath mineral wait comparison

The USA N64 mineral order loads a 75-tick work timer at `0x80132f20`
and waits for zero at `0x80132f34`. The active-unit update decrements
the timer at `0x801514a0`. The separate order poll runs every nine
native updates in the steady path. A timer started by that poll
therefore reaches its first zero-check at 81 native updates. The
normal full-patch pickup is eight minerals, which order 90 credits to
the owner wallet at `0x80136da0`. The NW/SW Blood Bath sample at native
frame 2,550 has two SCVs in the mining phase with timers 59 and 8,
one returning with eight minerals, and a wallet of 506. The wallet
matches the last captured gameplay HUD.

The current GBA wait starts at 45 updates. The opt-in
`SC64_NATIVE_MINERAL_WAIT_PROBE` uses 102 updates for a new mineral
wait, rounding 81 from the nominal 5:4 update conversion. Existing
work in a saved timer resumes at that timer's remaining value. The
fixture runs two trips for all three worker types and checks the
previous partial timer, Stop, and collection without a depot.

Both GBA Blood Bath runs use the ordinary Terran Custom Game menu
with Zerg in the third opponent row and base IDs 0 and 3, the exact
native hall positions at half scale. The human has four passive
SCVs throughout. These are the conservative readings from GBA
samples bracketing the compared N64 HUD captures.

| Opening sample | First wallet | Last wallet | Mineral gain |
| --- | ---: | ---: | ---: |
| USA N64 visible HUD | 206 | 474 | 268 |
| Normal Terran GBA | 346–378 | 850–890 | 472–544 |
| GBA mineral-wait probe | 250–258 | 578–602 | 320–352 |

The ARM probe links within the normal Terran static RAM bounds
using 38 matching prepared input hashes. Its gameplay save at
frame 1 records four SCVs with 101 work updates remaining. Ordinary
HOME L in a fresh mGBA process agrees with uninterrupted play at
load and 120 updates later. The 32 KiB SRAM seed stays identical.
The sanitizer fixtures for worker trips and the pending gather
journal pass with `WORKER_CARGO_CPPFLAGS=-DSC64_NATIVE_MINERAL_WAIT_PROBE`.

The longer wait accounts for most of the measured opening gap.
Compare patch occupancy, worker travel and depot approach before
activating it. A normal balance change needs new campaign controller
checks and computer build-order timing on all three regular products.

| Retained local input or receipt | SHA-256 |
| --- | --- |
| Native timer, cargo and wallet evidence | `f23f74f54bbf205f89a70fb3698001c4453738c46f86b89413bbb357f37d7715` |
| Matched Blood Bath opening | `fc754de650296c8dc7f2d47f3213643de9307251332a0bb60e279b0969641940` |
| Opt-in Terran ROM | `881330f9d4aeaaf78d3dac201cc4f0524352976bdaa35a4ded8e0bc7ac631c93` |
| Opt-in wallet observations | `6972e5c184ad28faaf2464f5841ec617178b0583a37d6441e71a0db13345c0c4` |
| Fresh long-wait save continuation | `8d1246f64ca0d0447a6c41957f7eeb605e005cd9d2926ea24de02b33d9531716` |
