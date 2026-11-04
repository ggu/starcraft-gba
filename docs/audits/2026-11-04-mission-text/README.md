# In-mission text boxes

Campaign missions now stop for a modal text box with a portrait, a speaker name and
wrapped text, and A advances. This closes item 1 of the N64 parity audit. Transmission and
DisplayTextMessage rows open it, as on the N64.

This page holds no ROM-derived text, art, frames or traces. Those stay in ignored `.local/`.

## Design

**Shared engine.** `SC64_MISSION_TEXT` is part of the race engine's core capability set in
`SC64_SHARED_ENGINE_FLAGS`, and `game.c` refuses a production race build without it. The
model is `starcraft/sc64_mission_text.h`, the panel is `starcraft/sc64_mission_text_draw.h`,
and `game_update()` asks one gate before any field input or simulation work.

**The world stops.** While a row is unread, no simulation tick runs: combat, AI, production,
the mission clock and trigger waits all hold. The pause menu can't open. Death effects
already in flight finish, as they do under the pause menu. Only A advances, and A is ignored
for 12 frames on each new page, so mashing A in a fight can't dismiss a line nobody saw. The
audio observer already skips a frozen game clock, so A makes no unit speak.

**Fixed storage.** The queue holds 16 rows in a ring. A full queue drops the newest row and
counts it. All of the box's RAM is about 60 bytes of IWRAM, because Z9 and Z10 have only 8 bytes
of working RAM left under their unchanged guard. The largest single trigger holds 11 rows, and the port runs one
trigger's rows at most in a tick, so an overflow needs several large triggers to fire
together. No mission has been seen to drop a row.

**A key held across a box.** Keys already down when a box opens are not new presses when it
closes. A drag held through a box continues, and neither repeats nor loses its press.

### Saves

A box is never in a save, and the save format and generations are unchanged. The queue drains
before the next simulation tick and before the pause menu or a save can run. An outcome
reached on the tick that posted its row waits for that row, and only then saves and shows the
result. Loading clears the queue.

The hand-written missions decide some rows from world counts, using latches that aren't saved.
The first tick after a load re-reads the world and latches what is already true, without a box.
So a line already read doesn't repeat. The cost is the opposite case: a load can show again a
tutorial line whose condition has gone false and later returns.

### Controllers and replays

Controllers and replays are keyed to simulation ticks, so they shouldn't all need new key
tapes. `sc64_mission_text_interactive` decides whether a posted row opens a box:

| Run | Box | Why |
| --- | --- | --- |
| The product's `common/main.c` | Opens | This is play. |
| Host tests and ARM probes | Row is counted only | The world evolves as it would across a dismissed box, because no tick runs under a box. |
| `common/main.c` under a debugger schedule | Row is counted only | Those tapes were recorded without A presses for boxes. |
| `-DSC64_MISSION_TEXT_QUALIFY` | Opens | A host layer reads every row with real A presses through `game_update()` between controller calls. Requalification uses this. |

The product's physical-input main loop has no schedule, so it always opens boxes.

### The 4-second holds are gone

Z8, Z9, Z10, P8 and P10 ran a silent 4-second wait for each Transmission, a stand-in for the
box. Those rows carry a zero time in the ROM, and on the N64 consecutive rows chain with A
presses, so the wait now ends on the next update. These five missions' timelines moved, and
their controllers are requalified below.

Z4, P4, P5, P6 and P7 keep their authored 3-second presentation holds, which their saves
validate. After A, those missions keep playing for those 3 seconds before the trigger's next
action, so their consecutive rows are spaced where the N64 chains them. Removing that needs
its own save and controller work.

The authored status lines stay, including T1's and P2's. They are now hidden while a box is up.

## How rows reach the box

| Missions | How a row is found |
| --- | --- |
| T4 to T10, Z4 to Z6, Z8 to Z10, P4 to P10 | The trigger interpreter passes the row's opcode, string ID and speaker unit. Same string, other speaker is another row. |
| T2 | The existing `(trigger, action)` message callback. |
| T3, Z3, P3 | Their model events, by trigger and action. |
| T1, Z1, Z2, P1, P2 | Written by hand, so rows follow the source conditions from world counts. Source waits between a condition and its row are not modelled. |
| Z7 | No enabled rows in the ROM. |

Four rows that the ROM marks disabled are left out, including one that is a developer comment.
That leaves 325 of the 329 rows.

**Not wired.** These depend on conditions the hand-written missions don't model:

- Z2 trigger 5, action 0: its Chrysalis danger line needs a countdown the port doesn't run.
- P1 trigger 8, actions 2 and 4: its power warning needs per-region building counts.
- P1 trigger 12, action 1: it needs a source switch the port doesn't keep.

## Speakers and art

The rows name 26 speaker units. Against the 23 briefing portraits:

| Speakers | Count | Result |
| --- | --- | --- |
| Named characters and the Advisor | 19 IDs, 14 portraits | Each maps onto a briefing portrait. Raynor has three IDs and Duke three, and Fenix's two IDs keep their two portraits. Duke speaks through his Norad II, and Tassadar through the Gantrithor. |
| Marine, SCV, Civilian, Firebat, Drone | 5 IDs | **Need art.** See below. |
| Dropship, High Templar | 2 IDs | Only in disabled rows, so never shown. |

DisplayTextMessage rows name no unit. The N64 shows them as the Advisor, so they use that portrait.

The N64 draws a real face for an SCV row, and that face isn't among the 23 portraits. It wasn't
found in the other image directories looked at, so where it lives is **not known**. The port draws
the HUD's button icon for those five speakers in the portrait frame, with the right name. No extra
art was added.

Briefing portraits index the briefing palette, which isn't live in a mission. A 224-byte ROM table
maps them to the nearest world colours, computed at import from the world palette all race products
share. A test checks it against the live palettes. Colours are close, not exact.

## Differences from the N64

- The panel covers the top of the map and leaves the HUD visible. The N64 keeps only its minimap.
- Text wraps to 186 pixels by five rows. A long row takes up to two pages, where the N64 shows one.
- The panel is opaque, and the N64's is see-through.
- The A prompt is drawn as text rather than the N64's button art.

## Results

Everything here is from the commit in the PR, using inputs prepared on this machine. All of it is
emulator or host evidence. Nothing was run on a physical GBA.

### Memory and ROM

Measured against the base, `88c17f15`. The audit estimated about 30 KB of ROM.

| Product | ROM growth | Working RAM | IWRAM |
| --- | --- | --- | --- |
| Terran | 25,652 bytes | unchanged | +56 bytes |
| Zerg Z7 | 25,372 bytes | unchanged | +56 bytes |
| Protoss P7 | 25,740 bytes | unchanged | +56 bytes |
| Protoss P10 candidate | 25,476 bytes | unchanged | +56 bytes |
| Zerg Z9 candidate | 25,548 bytes | unchanged | +56 bytes |

The ROM and RAM guards are unchanged. Z9 and Z10 stay at the base's 8 bytes under the working-RAM
guard, which is why the state is in IWRAM. Zerg Z7 has the least IWRAM room, with 72 bytes left.

### Boxes on the ARM builds

`tools/sc64_mission_text_arm_probe.py` ran each probe ROM in mGBA with no debugger. Each ROM carries
its product's exact flags and enters missions directly, so this shows the box, not that a mission
can be reached.

For each box, the probe checks that the world digest holds, that START, B and the D-pad stay out,
that A advances, and that the world then resumes. All checks passed. Frames were dumped from each box,
and T1, T10, Z1, Z6, P2 and P10 were read by eye.

| ROM | Missions showing a box with no input | Checks |
| --- | --- | --- |
| Terran | **T1**, T2, T3, T6, T7, T9, **T10** | 56 passed |
| Zerg Z7 | **Z1**, Z4, **Z6** | 29 passed |
| Protoss P7 | **P2**, **P7** | 20 passed |
| Protoss P10 candidate | P8, **P10** | 20 passed |
| Zerg Z10 candidate | Z8 | 10 passed |

The other missions' first rows need play, such as reaching a location or killing a unit. Those are
covered by the controller runs below, except Z9, Z10 and P9, as noted there.

### Controllers against the real box

Each controller ran twice, unsanitized: with rows counted, and with `-DSC64_MISSION_TEXT_QUALIFY`,
where every row is read with A presses through `game_update()`. Each pair ended with the same
result, tick and frame.

| Mission | Result | Rows seen | Mission | Result | Rows seen | Mission | Result | Rows seen |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| T1 | Won | 6 of 8 | Z1 | Won | 7 of 12 | P1 | Won | 6 of 10 |
| T2 | Won | 12 of 14 | Z2 | Won | 1 of 4 | P2 | Won | 5 of 7 |
| T3 | Won | 5 of 9 | Z3 | Won | 3 of 4 | P3 | Won | 2 of 2 |
| T4 | Won | 11 of 32 | Z4 | Won | 14 of 16 | P4 | Won | 12 of 14 |
| T5 | Won | 13 of 16 | Z5 | Won | 19 of 36 | P5 | Won | 3 of 4 |
| T6 | Won | 12 of 15 | Z6 | Won | 8 of 9 | P6 | Won | 13 of 20 |
| T7 | Won | 3 of 6 | Z7 | Won | No rows | P7 | Won | 7 of 10 |
| T8 | **No win** | 1 of 9 | | | | P8 | Won | 10 of 12 |
| T9 | Won | 12 of 16 | | | | P9 | Won | 0 of 2 |
| T10 | Won | 2 of 3 | | | | P10 | Won | 9 of 12 |

That is 196 of the 325 rows seen through the real box. Most of the rest are defeat lines and
branches a winning route doesn't take.

- **T8's controller doesn't win, and that isn't from this change.** At base `88c17f15`, with the same
  inputs, it ends the same way: no win at tick 200,000, frame 195,891.
- **Z8, Z9 and Z10 have no winning controller**, so their rows are untested in play, apart from the
  two Z8 boxes above. Their host tests pass.
- **P9's two rows are on defeat paths**, so its win shows none.

### P8 and P10 on their new timelines

These two lost their 4-second holds, so they were replayed on ARM from new tapes. Both still win.
These replays are input-only, so they count rows rather than opening boxes.

| Replay | Compared with the host | Mismatches |
| --- | --- | --- |
| P10 | 51 checkpoints, won at frame 49,841 | 0 |
| P8 | 99 checkpoints, won at frame 96,185 | 0 |
| P8 fresh-process save continuation, on ARM and host | 68 checkpoints from the save | 0 |

### Test suites

| Check | Result |
| --- | --- |
| `./dev build` | Passes |
| `./dev test`, unprepared checkout | Passes: 958 tests, 26 skipped for missing inputs |
| `make test` with prepared inputs | 6 failures, the same 6 as at base: `test_sc64_photon_cannon_renderer` (5) and `test_sc64_unit_word_sprites` (1) |
| `make test-sc64-terran-full-campaign` | Under sanitizers, passes up to T8's controller, as above. T9 and T10 were then run by hand and win |
| `make test-zerg` | Passes under sanitizers |
| `make test-protoss` | Every target it could build passes under sanitizers. `.local/sc64/protoss-p6/generated` isn't prepared here, so `test-sc64-protoss-canonical-p6` didn't build, as at base |
| `make unified races showcase test-unified test-races` | Pass |
| `make zerg-z8 zerg-z9 zerg-z10`, with their host tests | Pass |
| `python3 tools/check_source_tree.py` | Passes |

## Follow-ups

- Find the N64's face art for generic speakers, or decide the HUD icon is enough.
- Remove the 3-second holds in Z4 and P4 to P7, which needs save-validator and controller work.
- Wire the three remaining rows in Z2 and P1.
- Decide whether the authored status lines should go, now that boxes carry the real text.
