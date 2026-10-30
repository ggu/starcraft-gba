# Z7 ARM win and SRAM continuation at runtime base 9ff7f7c (2026-10-29)

**Historical evidence.** Every result here was produced on the runtime of
`codex/source-release` at `9ff7f7c`. Later merges changed shared runtime code,
so these results do not qualify current main; current-main qualification
stays open (see [Current main](#current-main)).

On that base, this audit closes two of the three gates that kept the
separate `zerg-z7` candidate out of the canonical Zerg profile:
- a full ARM win under the candidate feature vector;
- a fresh-process SRAM continuation.

The ARM runs use a replay probe that replaces `common/main.c`. They are
separate from replaying the unchanged production ROM; see
[Probe versus production ROM](#probe-versus-production-rom). The canonical
`zerg` alias and the six-entry Z6 product are unchanged. This is emulator
evidence, not a 30-fps or physical-GBA claim.

## Source and inputs

- Base: `codex/source-release` at `9ff7f7c` (PR #123 merged).
- `starcraft/game.c` SHA-256 `98f9a968125ce75f39d1e4ec1d903396f589d9941a500cbf47c3e51994c55c22`.
- `common/gba.c` SHA-256 `22502125db8bb0dbc6b17c048e80421203d6e20f37df26bc666ab90faa1321b6`.
- Prepared private inputs were reused read-only. No importer or extractor ran;
  the only generated header is the asset-free contact mask table under `build/`.

## Host controller at 9ff7f7c

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

## Verdict gates

`tools/sc64_z7_arm_replay.py` qualifies a run only when every gate passes, and
lists each failure:
- **Grammar:** every record must match its exact form: the tag, its kind word,
  and its fields in the probe's order, with no extra tokens. Each CHECK opens a
  CHECK, CHECK2, CHECK3 bundle for one tick. An orphan, incomplete or repeated
  bundle fails. A tick may carry two kinds (a checkpoint plus the terminal or
  cut bundle) only when their values are identical.
- **Framing:** the log starts with one START record whose mode, tape length,
  check count and stop tick match the trace. A full or resume run ends with
  one DONE record whose cursor consumed every tape transition. A prefix ends
  with one successful SAVED record. A resume has one successful LOADED record,
  on the play screen, directly after START. No early end is allowed.
- **Coverage:** the run must match exactly the host observations its mode
  covers. That set must be non-empty, and nothing outside it may appear. A
  prefix cut must be inside the tape, and its SAVED frame must equal the cut
  observation's frame.
- **Timing:** exactly one final cycles record, last, at the expected tick.
  There must be one periodic record at each multiple of 50,000 ticks in range.
  Each record's frame count must equal its tick minus the mode's offset, and
  all counters must be coherent and cumulative. Host logs are checked with
  `--untimed`, which rejects any timing records.
- **SRAM:** the schema is pinned in the tool (tag `0x53435A54`, 16,296-byte
  `RaceSave`); a log cannot redefine it. A slot is unused only when fully
  erased. Any other slot is a live record that must have the pinned tag and
  size, a non-zero sequence and a valid checksum. Required records:
  - prefix: exactly one pause save;
  - full run: one victory autosave when the trace wins;
  - resume: the seeded pause save kept byte for byte in the same slot, plus a
    sequence-2 victory autosave.

  SRAM is checked before the verdict; a run without SRAM cannot qualify.

`tests/test_sc64_z7_arm_replay.py` has 13 tests covering these gates, with
negative cases including:
- a wrong tag in both the SAVED log and the SRAM header;
- oversized, torn, zero-sequence and partly overwritten slots;
- orphan, incomplete, repeated and conflicting bundles, extra tokens and reordered fields;
- gibberish or mismatched START and DONE payloads;
- missing, duplicate, misplaced and incoherent timing records;
- an out-of-range cut, a SAVED-only prefix and a resume without its seed.

The reviewer's four reproduced cases also fail when applied to the real ARM
logs and SRAM.

Re-evaluated with these gates, the retained ARM logs and SRAM still qualify:
- full: 909/909 observations, 16 periodic records plus the terminal one, one victory record;
- prefix: 669/669 observations at cut 403,482, one pause save;
- resume: 240/240 observations, with the seed kept and a sequence-2 victory record.

## Build provenance

`tools/sc64_build_provenance.py` deletes one ELF and rebuilds it through
`make`, never with `-B`, so no private asset generator reruns. It records:
- the compile and link command Make actually ran;
- that command's ordered `-D`/`-I` vector;
- a `-M` dependency pass for each translation unit, run with the same argv;
- SHA-256 hashes of every dependency, the `.incbin` audio blob and the
  outputs.

Each rebuilt ROM was byte-identical to its evidence ROM. All four records
were taken at commit `367df4f`, which adds only tooling and docs to
`9ff7f7c`, using arm-none-eabi-gcc 16.2.0. Each record has eight
translation units for the release ROM and seven for a probe (no
`common/main.c`).

| ROM | ROM SHA-256 | Argv SHA-256 | `-D`/`-I` vector SHA-256 (count) | Input manifest SHA-256 (files) |
| --- | --- | --- | --- | --- |
| Release `zerg-z7` | `33664d3138755ef9e78c54028c7551a6f1b4e249b7abc80a5c14273c447f76e8` | `a003fd64088ff006cb1032b053878ed022b9082a7a618c569cb64b9f07263fd0` | `ace159029f921d29ac345c00f2309a2f6cca184640c82b00c40d44861850ab90` (108) | `97538bfcacc4b20ea8fbed01b9ef68925079a184795b9f546da99307c3e57beb` (161) |
| Probe, full | `b44c6aeaaf84a7f2e511260225e956e48980b0c6605371c9e8da59f0dcd6ac66` | `b31ee9f204b115de10a2879128b777e603e944abfa2391b9ed22a252c77e8a0c` | `885ed85b771b445879b7aef7dfffa77f8cb91ca887a07ea17bbf267a3c8f15c9` (109) | `35b04cd88d0f06451d2196b9aa6751d83aeb7e647000ad7c08c94d402e1f3320` (166) |
| Probe, prefix | `de2ae3903ed0187dedcae791171bdf9017cf6a72a00ab8cba638e51c835639f8` | `67635c534007378d71f28262c9df8ca9c8be9fdf630130adb9dde8b7a4367c06` | `6c5aec5f0b480b829dfbbf1c633b60b548359a9d5da8323091e46d9701ea07aa` (110) | `35b04cd88d0f06451d2196b9aa6751d83aeb7e647000ad7c08c94d402e1f3320` (166) |
| Probe, resume | `c8a1ecaadd7c6467e3b1c23aef334460e2e2b3c407e34200110f913ca0508fee` | `c47f3129f14e08d96c96ba656b828a69277b5274a9f11eae34ebf29977f2d875` | `dec4a995b7929aa8066556ceafa316624d4ec7ba48937ef342f31b986e98d740` (110) | `35b04cd88d0f06451d2196b9aa6751d83aeb7e647000ad7c08c94d402e1f3320` (166) |

How to read the hashes:
- The argv hash covers the NUL-joined argv.
- The input manifest hash covers sorted `path<TAB>sha256` lines; each probe's
  manifest includes 28 private generated inputs.
- The audio blob is `bd0adfd0ce83c0bb054d40c8f6bc69d91a55ca0f846f7ada56d48d98d7f72427`.
- `starcraft/game.c` is `98f9a968125ce75f39d1e4ec1d903396f589d9941a500cbf47c3e51994c55c22`.

The full records name local paths and private input hashes. They stay in
the ignored `.local/build/z7-provenance-20261029/`.

## Probe versus production ROM

The probe ROMs differ from the release ROM by design:
- They link the probe's own main loop instead of `common/main.c`.
- They drop `GBA_NO_DEBUG_INPUTS`, which only affects `common/main.c`.
- They add the tape include path and a mode define.

The probes prove that the shipped update/audio/render code, built from
these flags and inputs, plays the recorded inputs to the win. They do not
replay the unchanged production ROM: its keypad, VBlank and main-loop timing
are untested by these runs. Its physical-input path is covered only by the
separate keypad edge fixture. A production-ROM replay would need inputs
delivered through the keypad register, without debugger writes, and remains
open.

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

## Current main

After `9ff7f7c`, PRs #149 (shared construction, Siege and save mechanics),
#152 (order markers), #153 (builder safety) and #148 (top-bar icons) landed.
Together they changed 15 runtime files. None of the results above has been
repeated on that integrated source. Re-qualifying needs:
- a current-main host controller run;
- new ARM full, prefix and resume runs;
- new provenance records.

`make test-zerg-z7` also fails there until the carried-mission and Scourge
fixture failures below are resolved. Current-main qualification therefore
remains open.

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
