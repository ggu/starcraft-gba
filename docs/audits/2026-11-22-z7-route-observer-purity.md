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

Local evidence under
`/Volumes/Storage/sc64-route-observer-purity-20261122/build/purity`:
`tape.json` SHA-256
`144cc01b4c0bd0682b0f030f7857b14461b3f2a8723b5130b8af04ce79a2d54d`;
`comparison.json` SHA-256
`3821f10f11ebb62f3938231f12080ff781ba9e0f8460690e01a0d59d19afaab0`.
The host limit intentionally reports a non-win, so the controller exits 1.

The same corrected rendered controller subsequently completed the full
private early-gas policy under the 300,000-call cap: HOST won at call
270,585, frame 267,585, with strict terminal consistency and zero fault.
The exact full input tape SHA-256 is
`022d0f08d4020ab86b3ac69fcb8dfe430b7837c3817d37ae785e241a62cd3ad1`.
This is HOST evidence; a full physical-input ARM replay is still required.


## Focused regression recipe

With an already-prepared Z10 ARM argument JSON and its referenced local inputs,
run `python3 tools/test_route_purity.py PATH_TO_Z10_ARGS.json` from the checkout
root. It compiles and runs the prefix fixture with ASan/UBSan; the script does
not prepare or import private inputs. The local verification used
`/Volumes/Storage/sc64-shared-depth-overlap-20261122/build/depth-final/qualified/z10-on.json`.
The exact full physical-input replay on #711 is in progress; no terminal ARM
result is claimed here.
