# Human automatic target acquisition and fog

A private T9 ordinary-input diagnostic exposed player Wraiths damaging enemy
buildings whose centers were not currently visible. The controller had issued
Move orders rather than explicit attacks. Passive source-owner/type hit records
correlated the damage with human Wraith attacks; the ordinary idle acquisition
query did not filter the human visibility bitmap.

The authored fix adds current-visibility eligibility when choosing a *new*
human automatic target, before the existing weapon/detection/immunity checks.
It covers the spatial index, owner lists and fallback scan, defense-building
candidate paths, and P6 wall-trap acquisition. Non-human owners retain their
existing AI visibility model. Explicit remembered-target handling and released
attack resolution are separate paths and are not globally changed by the
acquisition predicate. Unit, building and save layouts do not grow.

## Reference and limits

The behavioral reference is pinned OpenBW revision
`4b046d5f65302b10cb0a745f0fecd37ec85b20a8`,
[`bwgame.h` lines 1868–1893](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L1868-L1893):
default target acquisition checks visibility independently of attack eligibility.
The local inspected header SHA-256 is
`5ee316ca2c008c9252f7fde7444ca5dc7f35440549be2576dc37fa228fb31ae6`.
No reference implementation code is copied.

This GBA fix uses the engine's existing human center-cell visibility bitmap;
it does not establish complete PC/N64 vision, target priority or combat parity,
and does not add a separate vision model for every AI. A visibility-correct
campaign replay may differ from earlier traces that relied on the defect.
Those earlier traces cannot qualify a new campaign win.

## Qualification status

Runtime checkpoint `2abc3d5` is qualified on default branch base
`4b3717302a4eb7b17de969873ef53a34fea5870d`. Follow-up `f308f95` only initializes
historical all-visible fixture baselines explicitly. Local results:

- Default build and tests pass: 649 Python checks, 20 optional skips, eight
  profile/native checks and the asset-free C suites. Source-boundary and
  whitespace checks pass.
- Exact tracked-source sanitizer fixtures pass for unit target queries, plain
  list/array queries, late-Terran defense owner lists, P3 and Z/P defense paths,
  and P6 wall traps. Regressions check hidden-nearer/visible-farther candidates,
  all-hidden, reveal/loss, human/AI differences, and a real one-update idle
  attack. The direct hit-resolver check verifies released damage is not gated
  by the new acquisition predicate; it is not a projectile lifecycle replay.
- All four normal ARM products link with unchanged RAM/stack guards and no BSS
  growth. Remaining IWRAM/BSS bytes: Terran 584/28,532; Z6 168/28,424;
  P7 1,168/23,880; Showcase 48/200. Four normal 32 MiB ROM headers, game codes
  and SRAM signatures verify.
- The current-source host matrix passes 72 normal-menu skirmish/save/load
  continuations: both maps, all three human races, one to three AIs, and focused
  mixed-three-AI compositions in each campaign product.
- Three normal-menu Blood Bath/mixed-three-AI actual ARM replays match fresh
  current-runtime host world and displayed Mode4 RGB555 checkpoints. Their
  37,859 counted frames sample an aggregate 28.834 fps, with the slowest P7
  window at 25.367 fps. These are fixed-camera unattended defeats. Individual
  full-frame work samples reach about 300 ms; this is not steady 30 fps.
  Changed combat eligibility can change workloads, so these figures do not
  isolate the CPU cost of the visibility predicate.
- Three actual in-place and fresh-process SRAM continuations match their fresh
  current-source host records at frame 9001 through frame 9901. Six owned SRAM
  emulator processes and the three rendering/timing emulator processes all
  terminate and are waited on. Transient-cache markers in these harnesses
  describe the VFX duration cache only.

Private evidence stays under `.local/build/idle-auto-visibility/`,
`idle-auto-visibility-products/`, `idle-auto-visibility-rgb/`, and
`idle-auto-visibility-fresh-sram/`. The two defense fixture baseline changes
occurred after the pixel probes compiled; compatibility records confirm that
all compiled runtime inputs are unchanged. Initial optional-fixture failures
and the private host capture helper's 64-bit pointer-cast compile failure are
preserved; the exact tracked fixtures and corrected host-only capture guard
were then rerun successfully.

No mission is enabled by this change. Delivered catalog remains T10/Z6/P7
(23 missions), with map selection and all human races versus one to three AIs.
All 30 original missions remain the campaign goal. Full campaign completion,
steady 30 fps and hardware qualification are not established.

No assets, generated game data, ROMs, saves, captures, private traces or native
decompilation sources are tracked. No Actions are dispatched.
