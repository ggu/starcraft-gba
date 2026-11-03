# Carried P4 and P6 controllers

P4 and P6 no longer won on the current integration, under the default P7 flags or
any later candidate's. Both were controller defects. This change touches test
controllers and make targets, and no engine code.

## Findings

| Mission | Symptom | Cause |
| --- | --- | --- |
| P4 | Lost at frame 90,312 | No income for about 60,000 frames, because nothing sent idle Probes to mine. The escort then stood in the corridor to the exit, and idle units do not step aside. |
| P6 | 180,000-frame limit | It assumed the order list opens on Move, but it reopens on the last command, so it sat in build placement. Once fixed, group orders stopped one pixel short of the first gate. |

Neither was bisected: both faults are visible in the traces, and the first is the same
as P3's in [the carried controllers audit](../2026-11-03-protoss-carried-controllers/README.md).

## What changed

All orders are still ordinary key input through `game_update()`.

**P4 (`tests/sc64_p4_campaign_run.c`):**
- Idle Probes are sent to patches. A patch with a unit parked on it swallows the tap,
  so that patch is skipped for a while.
- Real menu input replaces a `menu_cursor` write, so the inputs now describe the actions.
- Up to four Gateways, more Pylons, the shortest queue first, and a shorter wait between
  build orders.
- When the escort falls and the bank can pay, the heroes go home while another wave
  clears more of the route. This repeats up to six times.
- The escort walks past the exit once the heroes start their final walk.

**P6 (`tests/sc64_p6_campaign_run.c`):**
- Real menu navigation replaces the blind taps.
- Gate waypoints prefer a point well inside the region, and one Zealot walks in alone
  when the group stops just short.

**Targets:** `make test-sc64-protoss-carried-missions` now includes P4 and P6.
`make test-sc64-p7-campaign-composition` uses the maintained P3 controller rather than
`tests/sc64_p3_expansion_run.c`, a separate route that still does not win.

## Results

Run at commit `TESTED_COMMIT`, on the #255 branch. Inputs were cloned from the 2026-10-31
pinned set into a private `.local`. The first row uses ASan and UBSan; the profile matrix
uses scratch builds without sanitizers.

| Check | Result |
| --- | --- |
| `make test-sc64-protoss-carried-missions` | Pass: P1, P2, P3, P4 (frame 136,750), P5, P6 (frame 99,906) |

| Flags | P4 | P6 |
| --- | --- | --- |
| Default P7 | Win, frame 136,750 | Win, frame 99,906 |
| P8 candidate | Win, frame 136,750 | Win, frame 99,906 |
| P9 candidate | Win, frame 136,750 | Win, frame 99,906 |
| P10 candidate | Win, frame 136,750 | Win, frame 99,906 |
| P7 composition (development only) | Win, frame 107,058 | **Lost, frame 53,613** |

Under the composition flags, the maintained P3 controller wins at frame 55,408.

## Still failing

- **`make test-sc64-p7-campaign-composition`** still fails, now at its P6 step. Those flags
  are a separate development profile, not a product, and I did not diagnose the loss.
- **`tests/sc64_p3_expansion_run.c`** still does not win. An economy repair restored its
  income, but its base falls while the army is away, so I dropped that repair.
- **Older `menu_cursor` writes** remain in other test controllers, including
  `tests/sc64_t6_campaign_run.c` and `tests/sc64_terran_assault_run.c`.

## Not claimed

- These are host results only: no ARM replay, save or timing for these routes yet.
- Each result is one run with no start-offset sweep, so it shows the mission can be won,
  not that the route is reliable.
- Nothing is promoted. Native AI behavior and physical-GBA performance are not addressed.
