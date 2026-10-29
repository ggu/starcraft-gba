# Z7 current-source ARM win and SRAM continuation (2026-10-29)

This audit closes two of the three gates that kept the separate `zerg-z7`
candidate out of the canonical Zerg profile: a full ARM win under the exact
candidate feature vector, and a fresh-process SRAM continuation. Both use the
current source rather than a frozen runtime. The canonical `zerg` alias and
the six-entry Z6 product are unchanged. This is emulator evidence, not a
30-fps or physical-GBA claim.

## Source and inputs

- Base: `codex/source-release` at `9ff7f7c` (PR #123 merged).
- `starcraft/game.c` SHA-256 `98f9a968125ce75f39d1e4ec1d903396f589d9941a500cbf47c3e51994c55c22`.
- `common/gba.c` SHA-256 `22502125db8bb0dbc6b17c048e80421203d6e20f37df26bc666ab90faa1321b6`.
- Prepared private inputs were reused read-only. No importer or extractor ran;
  the only generated header is the asset-free contact mask table under `build/`.

## Host controller on current source

`make test-sc64-z7-canonical-controller` with its trace redirected into the
worktree reproduced PR #123's ordinary-input result byte for byte. The win
comes at host tick 821,565 / frame 808,493, through 70,561 held-key
transitions, with trace SHA-256
`2d28b63d74ac4438630561b714444ba77a12a9e79e9b4062133c928c447b8198`. The
route, contact and P7 merges after PR #123's frozen runtime (`a8764ef`) leave
this trajectory unchanged.

## Input-only replay probe

`tests/sc64_z7_arm_replay.c` replaces `common/main.c` and compiles the rest
with `$(ZERG_Z7_CANONICAL_FLAGS)`. That is the `zerg-z7` release vector
without `GBA_NO_DEBUG_INPUTS`, which only affects `common/main.c`. It boots
normally and plays a key tape generated from the controller trace by
`tools/sc64_z7_arm_replay.py`. The tape starts at the title screen and
includes the ordinary campaign-menu route. Each frame runs the shipped
`game_update()`/`audio_update()`/`game_render()` loop. Nothing writes game
state. At each of the trace's 909 checkpoint ticks the probe reports the
controller's trace fields: frame, screen, win/loss, model outcome and
started mask, all eight trigger cursors and timers, switch, AI script and
plan state, fault, own and enemy unit/building counts, workers, minerals and
gas. The driver compares every field with the host trace row at the same
tick. On ARM, records go through the mGBA debug registers. One isolated mGBA
process runs per replay, with its own HOME, config and save directory, log
mask 7, no debugger and no GDB port.

The probe's ARM links end IWRAM at `0x03004e08`, the same as the release
link, so hot-code placement is unchanged. BSS ends at `0x0203d818`. The
release ROM links at BSS `0x0203d730` / IWRAM `0x03004e08`, within the
`0x0203e000` / `0x03005000` guards.

Make targets: `sc64-z7-arm-replay` (full and prefix ROMs),
`sc64-z7-arm-resume Z7_ARM_CUT_TICK=N`, and host builds
`build/test-sc64-z7-replay-{full,prefix,resume-N}`.
`Z7_ARM_TRACE` selects the controller trace.

## Save-cut rule

The first host fresh-process resume, from a cut at tick 400,350, diverged at
the first later checkpoint: one Drone and 75 minerals missing, then drifting
gas. Per-tick world hashes were identical across the load. The difference
was the field UI's remembered command row (`menu_last`, `menu_cursor` 10 in
the uninterrupted run, 0 after load). By design, the save omits that
transient UI memory. The blind tape's next menu gesture therefore picked a
different command. This is a property of replaying a recorded key tape, not
lost simulation state. A player sees the menu and adapts.

The prefix therefore saves at the first tick at or after
`Z7_ARM_SAVE_MIN_TICK` that meets all of these:
- nothing is held at that tick or the next;
- the field is in normal mode with no open menu or selected building;
- every unsaved UI value the next inputs can read (remembered command row,
  menu cursor, help, tap and box gestures) equals its post-load default.

With the default minimum of 400,000, that is tick 403,482 / frame 392,580.
The save itself is the ordinary pause-menu path: START, release, B, release.

## Results

| Run | Checks matched | Result |
| --- | --- | --- |
| Host full replay with rendering | 909 / 909 | strict win, tick 821,565 |
| Host prefix | 669 / 669 | saved at tick 403,482 |
| Host fresh-process resume | 240 / 240 | strict win |
| ARM full replay | 909 / 909 | strict win, tick 821,565 / frame 808,493 |
| ARM prefix | 669 / 669 | pause-menu save at tick 403,482 |
| ARM fresh-process resume | 240 / 240 | strict win, tick 821,565 / frame 808,493 |

The ARM prefix SRAM holds one record: slot 0, sequence 1, tag `0x53435a54`,
16,296 bytes, checksum valid. Its payload SHA-256
`0c573c67e0b664e482494b1a463313a46d71aafed6e3848e0943127b01086d16` is
identical to the host prefix payload. A second mGBA process started with
that SRAM, pressed L on the title screen and loaded frame 392,580 in play.
It then continued the tape after the cut.
At the win, the resumed ROM wrote the normal victory autosave as sequence 2
in slot 1 (checksum valid, payload SHA-256
`693fd001d75b0869b192a94e7079106bd5c7afbef623ef182d36f3bf7ab14484`) and left
the slot-0 pause save unchanged. Both ARM terminal states report RESULT,
`SC64_Z7_WON`, started mask `0xfe`, no loss, no runtime fault, zero enemy
buildings and 89 / 8 / 12 own units, buildings and workers, matching the host.

## Timing and stack

Guest cycles come from the GBA timers and cover update, audio and render.

| Run | Frames | Mean cycles/frame | Frames/s at 16.78 MHz | Peak frame | Untouched stack |
| --- | --- | --- | --- | --- | --- |
| ARM prefix (ticks 1–403,482) | 403,482 | 2,039,154 | 8.23 | 44,905,361 | 10,116 bytes |
| ARM full (ticks 1–821,565) | 821,565 | 1,992,668 | 8.42 | 56,689,392 | 7,752 bytes |
| ARM resume (load + ticks 403,483–821,565) | 418,084 | 1,944,792 | 8.63 | 56,691,569 | 7,760 bytes |

Update work is about two thirds of the cycles (1.10 of 1.64 trillion on
the full run). The earlier documented 3.69 advancing frames/s measured the
older CONTACT tape on a frozen runtime; this tape and runtime differ, so the
two are not a like-for-like speedup. The current figure is close to the
7–8 updates/s recorded when P7 entered the canonical Protoss profile. The
untouched-stack figure is the painted region between `0x03005000` and 256
bytes below the startup stack pointer that was never written.

## Candidate host gate on this base

`make test-zerg-z7` does not pass on `9ff7f7c`. These failures come from
earlier merges, not from this change, and they also affect canonical Zerg:

| Check | Z7 flags | Canonical Z6 flags | Cause |
| --- | --- | --- | --- |
| Z7 profile, save, frame fixtures | pass | — | — |
| Z7 controller (`2d28b63d…`) | win | — | — |
| Z7 Scourge fixture | fails line 188 | — | PR #133: the new human-fog gate rejects the 8 px enemy; passes with that gate disabled |
| Carried Z1 | fails at tick 4,670 | same | PR #84: last good 81dafd5, first bad 83b264c |
| Carried Z2, Z3, Z4 | win | win | — |
| Carried Z5 | hangs at frame 4,620 | same | PR #84 (bisected the same way) |
| Carried Z6 | Force 2 not cleared | win | not bisected; not the fog gate |
| 18-match skirmish matrix | all terminal | — | — |

PR #84 switched every race ROM to the field controls. There, B reopens the
command menu on the last command, and an open menu holds the world clock.
The Z5 controller's attack-move helper still sends B, DOWN, A, expecting B
to open on Move. It returns with the menu open, and the controller's
frame-bounded loop then never advances. Walking the cursor with ordinary
DOWN presses (the P7 fix in PR #98) removes the hang, but the run then
reaches the 60,000-frame limit. Before #84, those presses also used game
frames, so the tuned timeline shifts. PR #84 adapted the other controllers
by writing `menu_cursor` directly after B. That is a UI state write, not
ordinary input, so it was not copied here. Z1 and Z5 need controller work
under the field controls; that is outside this audit.

## Limits

- The probe proves the shipped game loop under the exact candidate flags.
  It does not exercise the release `common/main.c` input path; the keypad edge
  fixture covers that separately.
- No physical GBA was used. The mean rate is far below 30 fps, and single
  frames reach tens of millions of cycles.
- The controller's inherited global placement and path predicates still read
  world state outside fog; see the observed-building ledger audit.
- Z7 remains outside the canonical `zerg` profile. Promoting it changes the
  default Zerg ROM and save identity (`SCZ6` generation 32 to `SCZ7`
  generation 36) and is a separate step.

Private traces, logs, ROMs and SRAM stay in the ignored worktree folders
`.local/traces/` and `.local/runs/`.
