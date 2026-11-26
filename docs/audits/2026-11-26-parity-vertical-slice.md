# N64 parity vertical slice (spike S4)

Spike S4 of the parity harness runs two scenarios from N64 capture to a
GBA verdict with no hand edits: five Marines ordered to one point in T1
(Wasteland), and the passive human's starting workers on Blood Bath. The
pieces are described in [`PARITY-HARNESS.md`](../PARITY-HARNESS.md). Every
capture, tape, frame and golden file stays in the private fixture home; the
numbers below are derived measurements.

| Criterion | Result |
| --- | --- |
| Both scenarios judged end to end, no hand edits | Yes: `./dev parity` builds the GBA runners, runs both, prints the scorecard |
| Two fresh-process N64 captures per scenario agree | Yes: identical per-tick hash chains (T1 941 ticks, Blood Bath 2,629 ticks) |
| Income fails with a measured GBA/N64 ratio | Yes: GBA/N64 = 1.75 over 2,000 source frames |
| Move check reports the group-spread difference | Yes: 23.0 px on the GBA against 29.4 px on the N64 |
| Real tolerances written down from the data | Below |

## Captures

All captures used the pinned frontend `3d082dd6…` (the #898 frontend plus
the `tap tick rec` writer), the JIT, the owned USA ROM and a fresh boot with
no cartridge save. Each held the shared native lock. The host was heavily
loaded by other sessions (load average 30–950), so cost is given in
main-thread CPU time.

| Run pair | Frontend steps | Mission ticks (unpaused) | CPU per capture | Chain |
| --- | ---: | ---: | ---: | --- |
| T1 move | 8,965 | 941 (801) | 67–81 s | identical, `5ab797b4…` |
| Blood Bath income | 14,450 | 2,629 (2,629) | 97–121 s | identical, `e03527e0…` |

Both members of a pair read the same RNG words at the first in-mission tick
(T1 `00016fd5…`, Blood Bath `000164c0…`), never a value the harness set.
No capture had a cache-alias conflict, a bad table link, a skipped tick or
a missing follow-up read.

## T1: five Marines to one point

The tape boots to Wasteland, closes the two opening transmissions (140
paused ticks, as in S2), drags a box over the five Marines with A held and
the stick moving, moves the cursor to open ground, and presses C-Right
(Move) then A. The decoder finds the five movers by position change; the
GBA joins them by type, owner and halved position, which matched exactly:
each N64 Marine stood at twice a GBA Marine's position.

| Check | N64 | Band | GBA | Verdict |
| --- | ---: | --- | ---: | --- |
| Cruise speed (px per source frame) | 3.905 | 3.514–4.295 | 3.733 | PASS |
| First arrival (frames after the group starts) | 80 | 78–82 | 65 | gap |
| Last arrival | 81 | 79–83 | 100 | gap |
| Group spread at rest (RMS px about the centroid) | 29.44 | 25.44–33.44 | 23.01 | gap |
| Command to first movement (report only) | 8 | 6–10 | 1 | report |

The N64 keeps the group's formation: every Marine moved by the same
vector (258, 132), walked the same 314-pixel path at 4 px per tick, and all
five stopped within one tick of each other. The GBA's group order sends the
five to a line of slots 16 px apart around the target (`issue_order`), so
the front Marines arrive 15 frames early, the last one 19 frames late after
detouring around the others, and the group ends in a flat 64 × 6 px line
instead of the original 37 × 72 px cluster. The remaining-distance curves
first differ by more than 8 px at source frame 44 after the start.

## Blood Bath: passive human worker income

The tape opens Scenario, picks Blood Bath (map 12 of the list), human Zerg,
first computer Terran, and starts. Nothing else is pressed. On the
cartridge, as on the GBA, the starting workers begin mining by themselves,
so the window opens at the first in-mission frame and no command is given.

| Check | N64 | Band | GBA | Verdict |
| --- | ---: | --- | ---: | --- |
| Minerals gathered per source frame, frames 0–2,000 | 0.176 | 0.160–0.192 | 0.308 | gap, ratio GBA/N64 1.75 |
| First delivery (frame) | 154 | 152–156 | 59 | gap |
| Minerals per delivery | 8 | 8 | 8 | PASS |

Over 2,000 frames the N64 human gathered 352 minerals in 44 deliveries; the
GBA gathered 616 in 74. Over the first 800 frames the ratio is 2.07
(248 against 120), because the first GBA delivery comes 95 frames sooner.
The minerals-gathered array (`0x800B1D44 + 0x90`) rose exactly as the
spendable bank did while nothing was spent, by whole 8-mineral trips; that
cross-check is the evidence that promoted both arrays to `proven` in
`sc64-usa-v1.json`. The gathered array starts at 50: it counts the starting
bank, so the check uses its rise.

## Tolerances adopted

| Quantity | Tolerance | Why |
| --- | --- | --- |
| Interval (arrival, first delivery, latency) | ±2 source frames | The N64 record is exact per tick (follow-step rule, zero torn records in these captures), and all five N64 Marines stopped within 1 tick; the GBA step-to-frame rounding adds up to 1 frame |
| Cruise speed | ±10% of the N64 per-unit range | 8-frame chords on the GBA's 2-px grid can be off by up to 2√2 px per chord (0.35 px/frame, 9% of 4 px/frame); the N64 units agreed to 0.001 px/frame |
| Group spread | ±4 N64 px | Two GBA pixels per axis |
| Income rate | ±(4 workers × 8 minerals) over the window | Each worker's trip phase can shift one delivery across either end |
| Trip yield | exact | Credits are whole trips on both engines |

The N64 bands carry no seed spread: both captures of a pair replay one
seed. Seed envelopes need captures that move the launch press (design §4)
and are left to WP9 and WP11.

## Design assumptions the slice tested

- **Clock.** Held. One unpaused tick is one source frame; the pause halfword
  removed exactly the 140 ticks of T1's opening boxes. Selector 4 in both
  scenarios. The command time needs care on both sides: the N64 state
  before a press is the last follow-up read taken at or before the tape's
  mark, and the GBA pre-command step must own its source frame
  (5 updates per 4 frames put two steps in one frame every fifth step).
- **Identity join on type, owner and halved position.** Held for T1's
  Marines (exact) and owner index (1 on both). In Blood Bath the N64 owner
  numbers and start corners are shuffled per launch: the human was owner 2
  at the top-left base, while the GBA human is owner 0 at the bottom-left.
  The join there is by race (the only Drone owner), and positions are not
  joined at all.
- **Timebase conversion.** Held (steps × 4/5), with the boundary rule above.
- **Group orders.** Broke: the N64 preserves formation; the GBA forms a
  line. This is the known group-spread gap, now measured.
- **Scenario equality.** The N64 Blood Bath lobby starts four players
  (human Zerg; computers Terran, Protoss, Terran) where the GBA scenario is a
  1v1. No other player's unit came within 1,396 px of the human's workers
  in the window, so the income window is undisturbed, but the start corner
  and its mineral layout differ.
- **Command input.** On the N64, A on open ground with units selected does
  nothing; the selection wheel's C-Right (Move) then A issues the move. The
  cursor moves 2.4 screen px per step at stick 16,000, linearly, and the
  screen shows the map at about half scale. The move target is inferred as
  the movers' final centroid, because cursor RAM is not located (S6).

## What the next scenarios need

- Cursor, camera and selection RAM (S6), so tapes are compiled rather than
  authored from frames and the move target is read, not inferred.
- Seed sweeps (moving the launch press) to turn single-seed bands into
  envelopes, and a lobby with the other computer rows closed for a true 1v1.
- The order byte (`+0x4D`) mapped to order names, to time a command from RAM
  instead of the tape step.
- A GBA Blood Bath launch that can choose the human's start corner, so the
  mineral layout matches the N64 capture.
- WP9 curves at 1–8 workers from the same gathered array, and one capture
  per movement class for WP8.
