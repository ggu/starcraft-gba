# P6 save validator cost

Winning P6 froze the game for about 83 seconds of GBA time, measured in an ARM replay under
the P10 flags. The [ten-mission audit](../2026-11-03-protoss-p1-p10-evidence/README.md)
found it.

## Cause

Winning a mission writes a save, and writing a save validates the candidate.
`race_p6_save_candidate_valid` decoded unit slots again for each source-map row and for each
pair of source slot and unit type. That one call decoded 122,722 units on the winning frame.
The second loop doesn't depend on how far the mission has run, so saving or loading P6 at
any point was expensive. Only the winning frame was timed.

## Fix

Only `starcraft/sc64_p6_save.h` changes in the game, and only the validator:

- Each slot's identity fields are decoded once into a table on the stack, and the row loop
  reads that table.
- The per-pair counts are collected during the existing pass over the slots.
- A pair with no units skips its count check, because zero can't exceed the limit.

The validator's stack frame is now 2,860 bytes in the P10 build. It adds no static memory.

## Results

Run on commit `acd4c20b` plus this change. Inputs were cloned from the 2026-10-31 pinned set
into a private `.local`, with the codec-2 audio bank. ARM runs use mGBA 0.10.5, one isolated
process per run, log mask 7, no debugger.

| P6 ARM replay under the P10 flags | Before | After |
| --- | ---: | ---: |
| Peak frame cycles | 1,385,378,775 | 41,291,345 |
| Peak frame, GBA time | About 83 s | About 2.5 s |
| Mean cycles per frame | 1,426,281 | 1,413,014 |
| Equivalent frames per second | 11.8 | 11.9 |
| Observations matched against host | 102 of 102 | 102 of 102 |
| Final state digest | `2454784593` | `2454784593` |

**The validator gives the same answers.** A scratch host build ran the old and new versions
side by side during the P6 tape. At 17 points it saved the real world, then applied 300 random
changes to the candidate's unit rows: bit flips, copied slots and cleared slots. Across 5,117
candidates they agreed every time, with 3,895 accepted and 1,222 rejected.

| Other checks | Result |
| --- | --- |
| Other nine missions, ARM replay | All observations matched, same final digests and the same peak frame cycles |
| P8 and P9 fresh-process save on ARM | Prefix 32 of 32 each; resume 69 of 69 and 124 of 124, same win |
| `test-sc64-p6-save`, `test-sc64-p6-battery-save` (ASan, UBSan) | Pass |
| `make protoss protoss-p8 protoss-p9 protoss-p10` | Link and verify; `__bss_end` and `__iwram_end` below the guards |

`tests/sc64_p6_save.c` gains a check that a candidate with too many copies of one source
actor is rejected while it still fits the unit limit. That check fails with the new count
removed.

## Not claimed

- **Remaining peak.** 2.5 seconds is still a visible stall. That peak wasn't broken down, and
  it may not be the winning frame.
- **Other P6 paths.** Pause-menu saves and loads in P6 run this validator, but only the winning
  frame was timed.
- **Other profiles.** Only the P10-flag replays were timed. Other products that include P6
  compile the same validator, and their ROMs were linked and verified, not replayed.
- **Canonical P6 fixture.** `make test-sc64-p6-canonical` didn't run because its private
  inputs aren't prepared here.
- **Hardware.** Nothing was run on a physical GBA.
