# P6 simulation profile baseline (2026-10-27)

## Scope and status

This note records stage timings for two diagnostic P6 route replays. The
current result is from the canonical packed-unit-word bank and exact feature
flags in the root final ARM command, with the two profiler defines added. The
earlier indexed-bank result is retained below as historical evidence. Neither
profile claims 30 fps or physical GBA performance, and neither profiling ROM
is the promoted product build.

The final uninstrumented product replay passed all 57 checkpoints at frame
49,599. Its ROM SHA-256 is
`c34c043b028c3bfc410297f42652a23cde3cd173bc46a736e8226ecb1e75c776`; its total
mean was 1,550,756 cycles (1,117,845 update + 432,911 draw). The diagnostic
profiles are instrumented builds with separate totals. The indexed-to-packed
comparison below does not show a meaningful simulation-stage improvement;
do not attribute one to the word bank.

## Replay and build

Both harnesses reused the same input-only P6 route tape and 57 state
checkpoints. The current packed-bank profiler was compiled with the exact
feature flags in
`.local/build/p6-promotion-20261027/root/final-arm/command.txt`, substituting
private source/output paths and adding only `SC64_DIAGNOSTIC_UPDATE` and
`SC64_DIAGNOSTIC_ORDERS`. It uses the generated `protoss-p6` sprite/word bank.
The indexed-bank numbers below came from the former
`.local/build/p6-promotion-20261027/profile/` build using the earlier canonical
ARM command.

The profiler sums existing per-update counters and separates early and late
means by replay tick (`tick <= REPLAY_END_TICK / 2` is early), not by a gameplay
event boundary.

At the repository root, the private command and runner were:

```sh
python3 - <<'PY'
import shlex, subprocess
from pathlib import Path
p = Path('.local/build/p6-promotion-20261027/profile-packed-final')
subprocess.run(shlex.split((p / 'arm-command.txt').read_text()), check=True)
PY
python3 .local/build/p6-promotion-20261027/profile-packed-final/run.py
```

The current runner launches one isolated no-GDB mGBA process and records
`arm.log` and `result.json` in its private directory. The indexed and packed
ELF/ROM/map files are local ignored artifacts, not part of this audit document.

## Current packed-unit-word run

The instrumented replay passed all 57 checkpoints and reached victory at frame
49,599. It measured 50,241 playing samples.

Mean cycles per update:

| Stage counter | Early | Late |
| --- | ---: | ---: |
| Orders | 310,118 | 282,043 |
| Mission | 468,990 | 404,816 |
| Production | 5,978 | 5,897 |
| Race state, including P6 trap processing | 159,125 | 29,517 |
| Defenses | 30,899 | 29,450 |
| Research | 122 | 122 |
| Update remainder | 1,786 | 1,777 |
| Exploration | 18,412 | 21,758 |

Order subcounters (nested within `orders`, so do not add them again):

| Order substage | Early | Late |
| --- | ---: | ---: |
| Setup/index construction | 116,279 | 104,478 |
| Query | 96,613 | 80,719 |
| Movement | 11,959 | 21,707 |

Mission and orders remain the largest update stages in both phases. In this
fog-small route, they are the first places to inspect for simulation work. The
race-state cost drops substantially later; a route-wide average hides this
variation. The order subcounters split only selected work inside `orders`.

The instrumented packed-bank run averaged 1,131,485 update cycles, 426,563
draw cycles, and 1,558,049 combined cycles. The prior instrumented indexed-bank
run averaged 1,131,485 update cycles, 426,558 draw cycles, and 1,558,044
combined cycles. Displayed stage-counter means were effectively unchanged;
selected nested query means varied by two cycles. This comparison shows no
meaningful simulation-stage improvement from the bank change and is not
evidence of a performance gain.

The final uninstrumented product replay averaged 1,117,845 update cycles plus
432,911 draw cycles, 1,550,756 total. The older 1,596,933-cycle input-only
probe used noncanonical feature flags and is historical only, not the
comparison baseline for the final product.

## Memory and limitations

The packed-bank profiling ELF linked with the existing linker guards enabled.
Its measured sections were:

- `.text`: 31,816,964 bytes; the padded GBA image is exactly 33,554,432 bytes
  (32 MiB).
- `.iwram`: 18,968 bytes; below the linker boundary at `0x03005000`.
- `.bss`: 250,452 bytes; below the EWRAM end guard at `0x0203e000`.
- The source save layout and guards are unchanged; this profile run is not a
  separate SRAM qualification.

These are mGBA ARM7TDMI counter timings from one replay, not hardware
measurements or a claim that the game meets 30 fps. Diagnostics add timing
overhead. The packed and indexed runs show no meaningful change in simulation
stage counters; the uninstrumented final product total is the appropriate
overall timing reference.

## Local artifacts

Current packed-bank artifacts are under
`.local/build/p6-promotion-20261027/profile-packed-final/`; historical
indexed-bank artifacts remain under `.local/build/p6-promotion-20261027/profile/`.
Each directory contains its `arm-command.txt`, `replay.c`, `tape.h`, ELF, GBA,
map, runner, `arm.log`, and `result.json`. The packed profile ROM SHA-256 is
`3ae7e759dcebe0a143739bd591872994fef08b30ed991c0e9aa58040097dcc6c`; its ELF
SHA-256 is `ada66080d80182f12e518b210d3c652f2838bc56f2c14be7f3f775fb737283d5`.
