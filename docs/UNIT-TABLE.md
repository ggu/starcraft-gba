# Large unit tables

The release race ROMs hold 192 (Terran) or 240 (Zerg Z10, Protoss P10) unit
slots, and unit slot numbers are stored in bytes (255 = none) in several
tables, so 254 is a hard ceiling. This document collects the measurements for
lifting it without slowing the release ROMs.

All cycle counts are mGBA guest cycles (emulator, not hardware) from the
headless replay runner and the instruction-level profiler kept with the
private measurement inputs. They do not establish physical-GBA performance;
see [Validation](VALIDATION.md).

## Baseline (revision 95a435c7)

Busiest 1,000 consecutive play updates of a three-bot custom game on map 1
(human Zerg; bots Protoss, Terran, Zerg), replayed from a HOST controller tape
as physical buttons. Two runs of each tape are byte-identical. `loop` is the
whole main-loop iteration (update, audio, drawing and the wait for the next
video frame); `upd` is `game_update` alone. Means per update:

| ROM | window (updates) | units / buildings at start | loop | upd | upd p95 |
|---|---|---|---:|---:|---:|
| Terran | 10,803–11,802 | 135 / 51 | 1,105,069 | 633,894 | 873,883 |
| Zerg Z10 | 10,793–11,792 | 142 / 54 | 1,304,159 | 766,867 | 1,063,846 |
| Protoss P10 | 10,825–11,824 | 146 / 52 | 1,435,656 | 902,904 | 1,189,026 |

Crowd scene (`SC64_CROWD_PROBE=200`, [`sc64_crowd_probe.h`](../starcraft/sc64_crowd_probe.h);
`make build/StarCraft-Crowd-200-t240.gba`): 200 Zerglings attack-move into
eight Marines guarding a Command Center on the Zerg Z10 engine, 1,500 updates
from boot. Mean `upd` 1,642,703 and `loop` 2,043,660 cycles over all updates;
2,003,945 / 2,475,664 over the march and fight (updates 2–500).
