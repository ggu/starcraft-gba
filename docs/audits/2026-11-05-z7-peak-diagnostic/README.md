# Z7 peak attribution probe

The qualified Z7 resume reported a 158,692,721-cycle maximum without its tape
tick or engine stage. That maximum may be a HOME load, UI transition, or active
gameplay; it must not be attributed to pathfinding from the aggregate alone.

`Z7_ARM_REPLAY_FLAGS=-DZ7_PEAK_DIAGNOSTIC` adds probe-only `Z7D PEAK` and
`Z7D STAGES` records on new maxima. They identify the tape tick, probe call,
input, screen/frame transition, update/render totals and existing coarse engine
stage counters. The default diagnostic keeps the normal runtime and reads its
existing order/sight counters. Adding `-DZ7_PEAK_DETAILED_STAGES` enables the
extra update subscopes; a `detailed=1` marker distinguishes those from zero
placeholders in the runtime-preserving variant. Context 2 is HOME load, 3 the
ordinary pause-menu save, and 1 the recorded tape.
Detailed stage counters are reset before each measured call; order
and sight values are reported only for advancing in-play frames. Diagnostic
writes affect counters and log output, not gameplay state or input selection.

Normal probe output remains unchanged when the flag is absent. The original
strict replay reader ignores the distinct `Z7D` prefix. The diagnostic reader
requires paired, complete records with end sentinels and bounded integers:

```sh
python3 tools/sc64_z7_peak_report.py .local/peak/mgba.log
```

Use the existing Z7 replay preparation, the identical controller tape and a
genuine SRAM file produced by its ordinary-input prefix. An isolated resume
probe can use `Z7_ARM_CUT_TICK=136058` for the recorded qualified cargo trace.
For an initial load-only observation, build with
`Z7_ARM_REPLAY_FLAGS='-DZ7_PEAK_DIAGNOSTIC -DZ7_PEAK_FRAME_LIMIT=1'` and stop the
owned isolated emulator when it emits `Z7D STOP`. The existing Python
`sc64_z7_arm_replay.run_mgba` helper accepts this explicit stop marker. That
bounded run deliberately emits no qualifying `Z7 DONE` and cannot be presented
as a mission completion. Never seed synthetic or modified SRAM into a guest.

When switching flags, rebuild only this worktree's own probe outputs: Make does
not track command-line flag changes. Preserve all ROM memory guards and verify
copied prepared inputs unchanged. Native compilation and execution use the
shared native flock and free-space guard; do not use hosted Actions.

These instrumented maxima locate a bottleneck. They are not an unbiased
production timing baseline, a hardware result or proof of 30 fps.

## Observed genuine resume

At probe source `9a445077`, the uninstrumented resume ROM first reproduced the
qualified normal probe byte-for-byte (`6e191f20f5b6acf0e6f2fe7f207ff93d633d91fb420c362bc78775f0b2861686`).
The phase-only probe then loaded the genuine ordinary-input prefix at cut
136058 and won at frame 173807. All 49 remaining scalar checkpoints passed,
with the prefix journal record preserved and a new valid sequence-2 victory
record written. This is the Z7 scalar replay schema, not exhaustive actor-state
equality. All 1,800 copied input hashes remained unchanged; normal and
diagnostic linker guards passed.

The maximum was 158,692,483 guest cycles (9.459 seconds), at tape tick 175455,
call 39397: frame 173806 to 173807, PLAY to RESULT, no held keys. This closely
reproduces the earlier unlocalized 158,692,721-cycle peak. The result transition
contains the campaign completion update and `save_game()` before publishing
the result screen; these measurements do not isolate the save routine alone.
The parser classifies this as a result transition even though its simulation
frame advanced. The initial HOME load separately cost 119,025,133 cycles
(7.094 seconds). Neither event should be described as a pathfinding spike or
ordinary combat frame. Busy gameplay performance remains a separate problem.

The diagnostic ROM hash is
`b5bd39c6f94984b11c072e9c6ad1f6b9fc9a5a4c145bc14a53baed0124560feb`;
the genuine seed hash is
`182407d91e782c4e879e65047223a70a0ec5f83bbec0131d4e205debddbd0d39`.
Detailed update-stage instrumentation was disabled. Logs and qualification
receipts remain in the private worktree's `.local/peak-qualification` directory.
