# Current shared skirmish stage profile (2026-10-30)

This is a bounded debuggerless mGBA measurement of the shared source at
`e150ff645524c458da6e7e8fc845f2117c39c630`, not a full campaign, physical
hardware, or steady-30-fps qualification. The normal menu and retained
ordinary-input tape start a Zerg human custom game against three AIs; no
actors, resources, or other game state are injected. The source game remains
in the play screen at the end of the run.

The probe replaces `common/main.c`, compiles the same game translation unit
and canonical Zerg flags and inputs, drives the tape, and samples existing
stage latches. It measures calls 8021–9020 inclusive: 1,000 input-loop calls.
For context, call 8000 reports game frame 7929 and call 9000 reports frame
8929. This is a new instrumented workload with a 20-call boundary offset from
previously published busy-Zerg samples; it is not a controlled comparison with
their reported fps.

| Measurement | Mean guest cycles per call | Maximum guest cycles in a call |
| --- | ---: | ---: |
| Map | 277,288 | 563,593 |
| Background, within map | 123,146 | 359,818 |
| Unit and effect drawing, within map | 102,318 | 196,182 |
| Building overlay, within map | 9,664 | 21,867 |
| Orders | 166,614 | 1,179,125 |
| HUD | 72,064 | 604,053 |
| Sight | 30,518 | 69,280 |

The sampled 1,000 calls took 698,580,726 guest cycles including update, audio,
render and presentation wait, equivalent to roughly 24.02 calls/s at the GBA
clock rate for this bounded probe. Map includes background, building and
unit/effect stages; nested stages are not additive. The stage latches describe
the latest completed draw and may not align one-to-one with update entries.
The samples make map rendering and orders useful targets for further profiling,
including the orders spike; they do not identify a specific optimization or
establish a general frame-rate bound.

The stage-reading probe and a reference probe use the same game object and
input tape. Their 90 sampled checkpoints match for selected state and fog,
displayed palette-resolved RGB555 FNV-1a 32-bit digests, frame, screen and
win/loss. These are sampled 32-bit digests, not exhaustive state or pixel
proof. Hashing and logging occur outside the timed loop, and the extra latch
reads can shift presentation phase; timer results remain measurements of the
instrumented probe. The reference probe's total includes 1,001 calls due to its
original sample boundary, so its total must not be compared directly with the
1,000-call profile total. The earlier visibility-span T5 measurement is a
separate workload and does not establish a busy-Zerg fps gain.

Source and evidence identities:

- `starcraft/game.c` SHA-256:
  `7f6ae28239da7d54a84d36bac658ecd14b91732faa72bad8b3af5f5e5b6680f9`.
- Derived input tape header SHA-256:
  `0d79706afb77e58c958a6dd622a56843f611f4400bfa014bdbeb7db751f6ef2b`.
- Reference probe ROM SHA-256:
  `bd4f57b5751ee4074f15086b1c3bd7bc51ece63114a671bdb26a209f611336c5`.
- Stage-reading probe ROM SHA-256:
  `dd8887eaf164886f6ac0b6479e59cdd227422c03ce2427945285cc163ef01d0d`.
- Private local evidence, from working directory
  `/Users/gabrieluribe/hacks/starcraft-gba/.local/worktrees/static-key-stream-20261030`:
  `.local/static-key-evidence/baseline/reference-run/probe.log`,
  `.local/static-key-evidence/baseline/profile-run/probe.log`, and
  `.local/static-key-evidence/baseline/probe/`. The retained controller input
  is under the shared-engine worktree at
  `.local/build/skirmish-catalog-controller-v1/zerg/matrix/m0-h1-n3.json`.
  These paths are local evidence, not tracked assets or a portable reproduction
  package.

An examined streaming rewrite of the fallback static-object key is not a
canonical performance improvement: canonical profile flags select
`SC64_STATIC_KEY_CULL`, whose separate live helper already streams and culls
its comparisons. The fallback rewrite was not merged as a performance change
and no speedup is claimed for it. The near-30-fps objective remains open.
