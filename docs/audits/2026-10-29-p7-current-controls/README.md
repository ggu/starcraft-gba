# P7 current-controls replay repair and performance baseline

The optional P7 ordinary-input replay had two obsolete inputs: A at the
briefing launch and R for unit cycling. On current canonical P7 it exited 2
before reaching its first checkpoint. The tape now uses START to launch and
an L tap to cycle selection. The harness also requires an actual selected
unit in attack mode and a selected unit receiving Attack-Move; a tape that
only pans cannot pass those checks. Runtime game code is unchanged.

## Current canonical measurement

The exact tracked replay and tape were compiled against default-branch
`1f875e1de4a7d9429cfb028b78449472e5cbeff1` runtime with the captured normal
Protoss P7 product flags and existing immutable prepared inputs. Range work
and contact grouping were OFF. No imported Make recipe or asset generation
was run. The authored host replay passed ASan/UBSan/Werror. All 20 sampled
ARM state digests and the terminal state match the fresh host replay.

This is a bounded opening and one ordinary Attack-Move, not a campaign win.
It ends in PLAY at frame 2,099 with no fault, 14 own units and 21 own
buildings. The frame counter advances less than the 2,115 input ticks because
menu commands can return before the simulation update.

| Measured window | Frames | Mean update cycles | Mean draw/presentation cycles | Mean full cycles | Derived fps | Maximum full cycles |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Opening | 600 | 1,557,321 | 483,958 | 2,041,279 | 8.219 | 11,181,679 |
| Attack | 1,400 | 1,727,899 | 498,603 | 2,226,503 | 7.535 | 2,903,615 |

These use mGBA ARM execution at the GBA clock, including the render and
presentation calls. They are not physical-device measurements. A 30 fps
frame has about 559,241 cycles: this current campaign workload remains far
outside that budget, even though other measured skirmish workloads average
near 29 fps. Neither the skirmish result nor this bounded replay qualifies
whole-campaign performance.

The private probe links with BSS ending at `0x02037a9c` and IWRAM ending at
`0x03004b70`, retaining the unchanged linker guards. Stack paint reports
9,588 bytes above the `0x03005000` reserve boundary. Harness bounds and stack
measurements do not replace normal-product memory qualification.

An additional diagnostic build, with update/order timers enabled, matches
the same state checkpoints. It attributes about 1.01M / 1.17M cycles to the
orders parent stage (opening / attack), including 464K / 601K setup,
95K / 90K typed queries, and 286K / 332K movement. Mission work is about 331K
per update in both windows. These are inclusive instrumented stage means;
they are not additive optimization savings. Its full means rise to
2.090M / 2.266M cycles. This evidence prioritizes orders/setup and mission
work for further profiling without reducing simulation cadence or changing
gameplay rules.

## Evidence and reproduction

The exact tracked-source host/ARM compiler arguments, fresh host records,
owned emulator launch/exit record and raw output are ignored local artifacts
under `.local/build/p7-controls-current-v1/`. The first failed unmodified-tape
attempt, equivalent private repair and separate stage profile are retained
under the range-integration worktree's
`.local/build/p7-current-campaign-perf-20261029/`. All owned emulator processes
were terminated and waited on. No GDB, Actions or SD writes were used.

The existing optional `test-sc64-p7-normal-input-replay` and
`build/sc64-p7-normal-input-replay.elf` targets still use their documented
development flag set. The new measurements above specifically use normal
canonical product flags; do not assign them to a different target or flag
set without rerunning it. Another machine must first prepare its own game
inputs through the documented import process. No game inputs, ROMs, saves,
traces, captures or generated headers are included in this source change.
