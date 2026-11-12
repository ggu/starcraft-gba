# Point route stalls in The Culling

The [qualified normal Z7 replay](2026-11-12-z7-normal-physical.md) was
repeated from HOME on normal Z10 and on the opt-in 96-column FIFO. Both
matched all 253 observations, won at frame 194,998, and averaged about
1,795,000 guest cycles per loop. Z7 is 128×128, so the earlier FIFO
does not service its point fields.

A read-only observer recorded the completed-update counter and the 64
largest loops in each run. The overall 58,707,335-cycle maximum on the
normal ROM is the final update, call 197,455, when the screen changes to
the victory result. This is separate from gameplay stalls. The largest
ordinary gameplay update is call 46,504, frame 45,262. At that call an
instruction and call-stack replay matches the measured cycle clock:
33,953,565 cycles fall under `sc64_route_fill` within movement orders,
out of a 37,547,713-cycle loop. It is a point-field problem, not a depot
lookup or drawing problem. The existing 96-column flag leaves this
update at 37,548,239 cycles.

## Use the same complete FIFO on wide maps

The wide probe reuses the same padded-byte expansion, seed order,
neighbor order and complete packed output as the 96-column version.
The 128-column layout reserves at most 6,144 queue entries instead of
8,192 to make room for its wider padded grid within the same scratch
area. Compaction keeps pending entries in order. Overflow falls back
without writing any destination field or live grid bytes. The 64×64
path, other widths, and route cache behavior stay unchanged.

`make test-sc64-route-linear` compares 386,646 host queries against the
existing packed flood. Both the normal and forced-overflow variants
match every cache observation and fill count. Open 96×128 and 128×128
fixtures compare every route cell and check the retained topology copy
before any fallback. Z10 ARM passes ROM and RAM guards: EWRAM is unchanged
at `0x02038b00`, IWRAM ends at `0x03004d68` (24 bytes above the earlier
linear probe and below `0x03005000`).

| Z7 call 46,504 | Normal | 96-column probe | Wide probe |
| --- | ---: | ---: | ---: |
| Loop cycles | 37,547,713 | 37,548,239 | 21,313,017 |
| Simulation cycles | 36,980,883 | 36,981,389 | 20,740,513 |
| Movement orders | 36,185,273 | 36,185,776 | 19,943,291 |

The ordinary replay matches the first 127 observations through that
call; the route build and hit counters also match. The gameplay spike
is 43.24% shorter. The complete wide run matches all 253 normal and
96-column observations and wins at the same frame 194,998 (call
197,455). Its mean drops from 1,794,976 to 1,679,547 guest cycles
(−6.43%, 9.35 to 9.99 emulated updates per second). Its worst ordinary
gameplay loop is 21,320,327 cycles at call 50,263, down 43.22% from
the worst normal gameplay loop. The separate result transition still
takes 57,817,607 cycles. On the 96×128 Z10 mission, the same wide cartridge
matches all 61 state checkpoints, 24 video/palette/fog samples and the
full win at frame 59,997. Its two measured window means are within 0.02%
of the earlier 96-column probe.

Terran, Protoss P10 and Showcase also link with unchanged EWRAM, with
IWRAM ends at `0x030048d0`, `0x03004c30` and `0x03004e80`. Their three
physical 64×64 save/fresh-load/continuation runs match the normal
controls, including resolved starts and identical SRAM bytes. Paired
128×128 campaign prefixes match nine world, route-cache, camera, fog and
video samples each. Terran T4 exercises seven wide point fills through
frame 1,476. Protoss P10 exercises eleven through frame 1,476, with seven
ordinary authored-message acknowledgments. These are prefixes, not
full Terran or Protoss mission wins.

Both complete Z10 custom matches with three independently selected AIs
also reproduce their normal control through defeat (76 and 77 state
checkpoints). The measured 64×64 custom window means are within 0.04%
of normal; the existing small-map FIFO remains the active path.

## Routine battle and menu cost

A separate normal-ROM instruction sample profiles 12 calls spaced 89
inputs apart from call 15,000 and matches 39 gameplay observations.
The eleven samples outside the command menu average 1,196,454 guest
cycles: rendering takes 700,872 and game update takes 462,315. Selected
large render costs include static map objects (124,926) and the generic
sprite blitter (111,752); these scopes can overlap and must not be added.
The one menu sample, call 15,623, spends 2,164,120 cycles drawing the
command list, including 1,070,811 in its row icons. The menu is a separate
workload worth an exact scaling/compositing check.

Wide probe ROM SHA-256:
`1abfd1e56dcc8c7a44522973ad37564136ccf1923b46e0b1486ac2ee718c2499`.
Full wide Z7 receipt SHA-256:
`4c136e7454ea30d6f59c5ab881f0a878d7688cbbb9eb7628b45cb8b52af7bf15`.

The option remains off by default pending hardware qualification.
The 561,792-cycle target
still requires a substantial reduction in routine simulation and draw
work. A separate selection-ring span probe matched all 35 gameplay and
24 video samples in the Z10 battle windows, but improved mean loop cost
by only 0.09–0.11%, so it stays out.
