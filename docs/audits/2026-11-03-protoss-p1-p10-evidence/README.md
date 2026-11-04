# All ten Protoss missions under the P10 flags

A promoted ten-mission ROM would ship `PROTOSS_P10_FLAGS`, so that is the profile a
promotion has to answer for. This audit runs every Protoss mission under those flags:
an ordinary-input win on host, then an input-only replay of the same tape on ARM.
Nothing is promoted, and the default Protoss ROM is unchanged.

## P8 needed a route for these flags

P8's tape from the [requalification](../2026-11-03-p8-p9-requalification/README.md) loses
under these flags, which add `SC64_NEAREST_RESOURCE_DEPOT`. Where that flag is set, the
route now takes gas and a Cybernetics Core early, recalls strays, and sends the army at
raiders. Under P8's own flags the tape is byte-identical, so that audit's evidence still
holds.

No mission was made easier, and no engine code changed for this audit.

## Results

Run at the commit recorded in the pull request. Inputs were cloned from the 2026-10-31
pinned set into a private `.local`, with the codec-2 audio bank. ARM runs use mGBA 0.10.5,
one isolated process per run, log mask 7, no debugger. Replays write no game state.

Every ARM replay matched every host observation, with the same final state. Timing covers
the whole tape, including menu and briefing frames, at 16,777,216 cycles per second.

| Mission | Win frame | Tape calls | Observations matched | Mean cycles per frame | Equivalent fps | Mean update cycles | Peak frame cycles |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| P1 | 24,986 | 25,167 | 26 of 26 | 730,888 | 23.0 | 253,904 | 18,798,663 |
| P2 | 34,026 | 34,375 | 35 of 35 | 1,244,057 | 13.5 | 485,325 | 27,808,526 |
| P3 | 48,961 | 49,747 | 50 of 50 | 1,218,725 | 13.8 | 534,542 | 43,788,980 |
| P4 | 136,750 | 137,973 | 138 of 138 | 1,304,994 | 12.9 | 642,629 | 28,636,259 |
| P5 | 5,056 | 5,115 | 6 of 6 | 1,088,539 | 15.4 | 569,519 | 24,156,413 |
| P6 | 99,906 | 101,314 | 102 of 102 | 1,426,281 | 11.8 | 940,945 | **1,385,378,775** |
| P7 | 48,923 | 50,152 | 51 of 51 | 2,506,382 | 6.7 | 1,797,410 | 31,525,511 |
| P8 | 96,889 | 99,351 | 100 of 100 | 1,784,047 | 9.4 | 1,043,847 | 26,453,254 |
| P9 | 150,969 | 154,594 | 155 of 155 | 2,088,779 | 8.0 | 1,286,548 | 31,741,188 |
| P10 | 72,441 | 73,829 | 74 of 74 | 1,828,192 | 9.2 | 1,026,231 | 39,610,502 |

All ten are below the 30-fps target.

### Save continuation

P8 and P9 tapes hold a pause-menu save, so each also ran as a prefix and a fresh-process
resume from the SRAM file that prefix wrote.

| | P8 | P9 |
| --- | ---: | ---: |
| Save tick | 31,239 | 31,072 |
| Prefix observations matched on ARM | 32 of 32 | 32 of 32 |
| Resume observations matched on ARM | 69 of 69, same win | 124 of 124, same win |
| Host resume observations matched | 69 of 69, same win | 124 of 124, same win |

P10's save continuation is in [its own audit](../2026-11-03-p10-ordinary-input/README.md),
under these flags. P1 to P7 have no save continuation under these flags.

### Memory

`make protoss-p10` links and verifies: `__bss_end` `0x020394f4` and `__iwram_end`
`0x03004a58`, below the unchanged guards of `0x0203e000` and `0x03005000`.

## P6 freezes on its winning frame

P6's peak frame is 1,385,378,775 cycles, about 83 seconds of GBA time. That is the frame
the mission is won, and almost all of it is inside `game_update`.

Winning a mission writes a save, and writing a save validates the candidate.
`race_p6_save_candidate_valid` (`starcraft/sc64_p6_save.h`) walks the source map's unit
rows and, for each row, decodes unit slots until one matches. A row whose unit has died
scans all 240 slots. By the end of the mission most rows are like that, so this one call
decoded 122,722 units on host for this tape.

This is game code, not part of the replay probe, and other profiles that include P6
compile the same validator. Only the P10-flag replay was timed. Saving or loading late in
P6 also runs it, though neither was timed separately. The other nine missions don't run
this validator.

The [P6 save validator audit](../2026-11-04-p6-save-validator/README.md) fixes this. The
table above is from before that fix.

## Reproduce

With private inputs prepared and no shared input regenerated:

```sh
make test-sc64-protoss-px sc64-protoss-px-arm SC64_AUDIO_DIR=.local/sc64/audio-codec2/generated
```

That runs each controller, converts its trace with `tools/sc64_protoss_tape.py`, checks
the P8 and P9 host resumes and builds the replay ROMs. Then run and compare each ROM, for
example P6 (mission 25):

```sh
python3 tools/sc64_p8_arm_replay.py --mission 25 --prefix PX run --rom build/StarCraft-Protoss-PX-p6.gba --out build/emulator/px-p6
```

```sh
python3 tools/sc64_p8_arm_replay.py --mission 25 --prefix PX compare build/protoss-px/p6/host-full.txt build/emulator/px-p6/p8-lines.txt
```

Missions P1 and P2 are 2 and 3, P3 to P7 are 22 to 26, and P8 to P10 are 27 to 29. The
prefix and resume ROMs for P8 and P9 are `build/StarCraft-Protoss-PX-p8-Prefix.gba` and
`-Resume.gba`; pass the prefix's `.sav` to the resume run with `--sav-in`, and compare it
from the save tick with `--from-tick`.

## Not claimed

- **Single tapes.** Each mission's result is one route run once. It shows the mission can
  be won with ordinary input, not that the route is reliable.
- **Save coverage.** P1 to P7 have no save continuation under these flags, and P8 and P9
  have one save point each.
- **Replay ROMs.** ARM results are from a replay probe that shares the candidate flags but
  has its own main loop. The production ROM was linked and verified, not replayed.
- **Availability.** P10's availability tables aren't applied, and the Gantrithor has no
  attack. Draft #212 covers Carrier work.
- **Authored AI.** The AI backends, trigger cadence and transmission durations are
  authored, not native, so native combat equivalence is not claimed.
- **Hardware.** Nothing was run on a physical GBA, and mGBA timing is not a hardware
  measurement.
