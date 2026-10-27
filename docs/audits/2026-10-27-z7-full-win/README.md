# Z7 full-render ARM winning replay

An isolated instrumented ARM build completed the ordinary-input Z7 winning
tape in mGBA. All 365 checkpoints matched the host reference's dual actor and
game-state digests. The final source checks passed at host tick 209,485,
game frame 205,764, with RESULT/won, no runtime fault, a surviving human force,
zero source Force 1 buildings, and all eight action cursors and wait clocks
clear. The replay did not use GDB or writes to world state.

This is a development-profile completion result. It does not promote Z7,
qualify a fresh-process ARM save continuation, establish native AI
equivalence, or meet the 30-fps objective. Canonical Zerg remains Z1–Z6 and
qualified staged progress remains 22/30.

## Build and comparison scope

Runtime source was based on `a40bb806a18844ff5ec1b29afd4a742456f6ae57`
(PR #62), retaining the full canonical presentation features, packed Zerg
bank, and the existing optional Z7 rule/performance features. The ARM
preprocessor vector was checked against public `Z7_STEERING_FLAGS` plus
`SC64_REACHABLE_FORMATION` and `SC64_Z7_PRUNE_STARTED`. This build does not
enable `SC64_ROUTE_DECAY_FREQ`.

The public ordinary-input policy is `tests/sc64_z7_campaign_run.c`. Its
20,325 input transitions and 365 state observations form the frozen tape.
An independently compiled indexed host reference and the full-render host
candidate matched all 365 digests under ASan/UBSan before ARM replay. Digest
fields include named actor/gameplay state; padding and derived presentation,
pathfinding, and audio caches are excluded. They are checkpoint digests,
not byte-for-byte snapshots of every emulated memory region.

The private harness supplies only held/pressed buttons through
`game_update`, followed by `audio_update` and `game_render`, including the
normal presentation call. Read-only observations validate the resulting
state. The terminal success predicate directly evaluates the source force
count and pending programs, rather than relying on the menu's displayed win
alone. Generated input headers, ROMs, ELF files, saves and logs stay ignored.

Frozen evidence identities:

| Item | SHA-256 |
| --- | --- |
| Winning input/observation trace | `252891e51ce82c44442acd098f58c63ad7616c79bd895c22c1bebc26e279ec97` |
| Fixed ARM ROM | `3d9982e2a7c64a34cc5dbd18a7ec5154de54c25e516478657291e8663a17ade4` |
| ARM ELF | `82b9e5f39254ab7aa4bb15dfdfa431778beacedd6dd8f599b5e53515ef580e86` |
| ARM harness source | `5cf28fc6401f20e3ddeca3b56c717a4c44634cd7138ae20e70ff3f3fa7a8f6e5` |
| Expected digest header | `e133bbfd9d44b4265db18627ca4d7069160d1f4d58927166e2b729830a1ef844` |
| Input tape header | `a55a2a3c2082ac1084cb5f9e288ae4eb8eb4e769c8f68e7e4ff09b632fc77b5b` |

The fixed ROM is 32 MiB, title `SC Z7 WIN36`, code `SZ7W`. The listed replay
artifacts and tape/digest hashes were checked again after the run completed;
source and generated inputs are pinned by their separate manifests. Its
source state and input tape must not be substituted with a later candidate
when interpreting this result.

## Performance result

The harness measured 205,764 simulation-advancing PLAY updates. Timed update
work excludes the subsequent checkpoint hashing/logging; the draw interval
includes audio, rendering and presentation. GBA timer deltas reported:

| Timed interval | Mean cycles | Maximum cycles |
| --- | ---: | ---: |
| Game update | 5,317,343 | 1,723,919,131 |
| Audio/render/present | 559,682 | 2,071,131 |
| Combined | 5,877,025 | 1,724,760,571 |

At 16,777,216 cycles/second the combined mean corresponds to about 2.85
rendered updates/second. This instrumented emulator result is far below the
30-fps target and is not a physical-GBA benchmark. The extreme update
outliers need investigation; a successful campaign replay is insufficient
performance qualification.
