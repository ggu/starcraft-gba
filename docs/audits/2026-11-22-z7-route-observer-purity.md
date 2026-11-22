# Z7 controller route-observer purity

The rendered HOST controller on `71809328` previously matched named ARM
observations through call 23,571 but disagreed at 23,879 by eight gas.
An independent per-update investigation found the ARM Drone 115 deposited
eight gas at call 23,714 while the HOST Drone had already diverged in its
route. Earlier, at call 17,629, all logged live-unit fields and the walkable
grid matched, but route diagnostics differed: HOST had 405 builds and
19,341 hits, ARM 404 and 19,342. At call 17,630 HOST Hydralisk 82 failed
its route and set retry 12 while ARM advanced along its waypoint. The
visible aggregate checkpoints did not expose that earlier difference.

The HOST route-observer transaction omitted `sc64_point_prefixes`, its
per-slot index and the small-component row hints. Current Z10 flags enable
these structures, and controller path queries can mutate them despite the
transaction restoring packed fields and passing its old purity hash. The
transaction now includes these regions. It also includes the guarded
facility-frontier bindings and diagnostic/verification scratch so their
bookkeeping follows the already-snapshotted queue and grid. The optional
`--render-input-frames` controller flag runs `game_render` between input
updates, matching the physical input loop's render cadence; it remains off
by default for other uses.

With the corrected transaction and rendered controller, a newly generated
ordinary-input HOST tape reaches its bounded 24,000-call limit. Replaying
that exact tape on the unmodified #708 Z10 ARM cartridge matches all 62
named observations through call 24,000, beyond both the 17,630 movement
split and 23,879 gas mismatch. This is checkpoint equivalence, not a
per-frame actor-array or full victory claim. A prepared Z10 ASan/UBSan
fixture verifies nested observer scopes restore all point-prefix storage,
index entries and both component hints. Source and ARM ROMs remain local.

The private bounded tape SHA-256 is
`144cc01b4c0bd0682b0f030f7857b14461b3f2a8723b5130b8af04ce79a2d54d`.
The comparison SHA-256 is
`3821f10f11ebb62f3938231f12080ff781ba9e0f8460690e01a0d59d19afaab0`.
The host limit intentionally reports a non-win, so the controller exits 1.

The same corrected rendered controller subsequently completed the full
private early-gas policy under the 300,000-call cap: HOST won at call
270,585, frame 267,585, with strict terminal consistency and zero fault.
The exact full input tape SHA-256 is
`022d0f08d4020ab86b3ac69fcb8dfe430b7837c3817d37ae785e241a62cd3ad1`.


## Normal ARM victory on the missile-trail release

A fresh process starts the normal `6c9b4482` Zerg ROM at HOME and
executes the complete corrected tape with physical button inputs.
All 206 named observations match through call 270,585. The result
screen reports victory at frame 267,585, with defeat clear and zero
script fault. The replay checks named campaign fields, actor counts and
economy. The world and video snapshots provide additional comparison
points for subsequent builds.

Across all 270,585 calls the replay averages 1,411,541 guest cycles
per call, including menu and presentation waits. The mean corresponds
to 11.89 emulated main-loop calls per second.

On the `4aff72cf` steering build, ordinary Start and B save the corrected
tape at call 31,892, frame 30,739. A fresh HOME process restores the save
and replays 20 matching named observations through call 39,349 and
frame 38,149. The save digest stays fixed during the continuation.
The full uninterrupted steering-build replay and a fresh continuation
through victory are separate checks. The pause receipt SHA-256 is
`010f7ad07b7e39565ea01c61e7fef835033a5018c29799e332dae13e022ff2a1`.
The continuation receipt SHA-256 is
`1ff8710d0e2dd334d885f7ca59f265cfdce420384a483a538a337e6fe370d4a5`.

The Zerg ROM SHA-256 is
`b7a6231297cf783011acbe70c8d2a2b6f243114b1c651dd00933a9d829ee5f54`.
The full ARM replay receipt SHA-256 is
`492fa7315db38bd07da59d0a3aab755560133dbea9f97e2ade33b8d240b950b8`.

## Full normal victory on the steering release

The exact `c2624205` cartridge repeats the full corrected ordinary
button tape and wins at frame 267,585. All 206 named checkpoints
match. The two release ROMs match the sampled world fields, route
counters, UI state, palette and both framebuffer page hashes.
The displayed-page phase differs at two cuts.

| Tape window | Mean guest cycles per call, missile trails → steering |
| --- | ---: |
| Entire 270,585-call tape | 1,411,541 → 1,385,425 |
| Calls 71,261–93,896 | 1,544,871 → 1,516,264 |
| Calls 152,303–170,438 | 1,674,097 → 1,641,111 |
| Calls 222,902–243,327 | 1,375,089 → 1,341,575 |

The full replay improves 1.85% across the combined detector and steering
changes. These are emulated main-loop calls, including presentation
waits, and dense combat remains above the 30 fps cycle budget.
The steering-build victory receipt SHA-256 is
`3651fd4fa509432c542cc4fbb7ea800f137fbd36f3c613ff6b1013afa04ff6ce`.

## Focused regression recipe

With an already-prepared Z10 ARM argument JSON and its referenced local inputs,
run `python3 tools/test_route_purity.py PATH_TO_Z10_ARGS.json` from the checkout
root. It compiles and runs the prefix fixture with ASan/UBSan using the
already-prepared inputs. The focused fixture passes with the normal #708
and #711 Z10 manifests.
