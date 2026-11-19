# N64 custom-game speed, mineral income and cadence

Two fresh ares sessions opened the same Challenger Melee match with a human
Protoss and one Zerg computer. The first six captures are byte-identical,
including the opening and the selected Game Speed slider under Pause →
Options → Settings. The settings also show Quickkeys Timer, Unit Info Timer,
Cursor Speed and Minimap Fade.

The runs differ at four 10-step input holds. One leaves the Game Speed
slider at its starting position. The other applies D-pad Right four times
with identical releases and moves the slider right. Both return through
Back and Resume at the same observer steps, then remain neutral. Supply
stays at 4/9 in the captures below.

| Emulator run step | Starting slider minerals | Higher slider minerals |
| --- | ---: | ---: |
| 5,941 | 74 | 74 |
| 6,841 | 106 | 130 |
| 7,741 | 162 | 194 |
| 8,641 | 202 | 258 |

The starting slider gains 128 minerals and the higher setting gains 184
over the same 2,700 observer steps. This establishes a visible gameplay
effect for the speed control in one ordinary-input match. The readings
measure banked minerals at four samples. A general worker mining rate still
needs separate validation.

| Pinned input or log | SHA-256 |
| --- | --- |
| Owned USA ROM | `e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020` |
| ares visual observer | `eb8d30c00dee5176188cdc91367c219c9c3aa73d0079cd6a0273bd3ba8490aa5` |
| Starting slider commands | `b4d0d536739596bef37515d0137c4ee79e7447c518ae0d13810542c9c7309585` |
| Higher slider commands | `34a853ebfba37f2d531d576d7a845a83dc394967f773822fb0831a5a646c6cff` |

Both isolated manifests passed. Captures and game data stay local.

## Paired gameplay cadence

A separate pair used the same two controller plans and took read-only RAM
snapshots from the same emulator executable. A third run substituted four
ten-step analog-left holds for the default run's four neutral holds. At
observer step 5,370 all three runs were in Settings with the pause flag set.
The visible slider positions differed. The active selector was still 4,
the pacing-table value was 56, and the two gameplay countdowns were 66
and 12 in every run.

All three runs had resumed at the 5,940 and 6,001 snapshots. The nearby
field captures at the starting and higher settings showed 74 minerals
and 4/9 supply.

| Snapshot reading, steps 5,940 → 6,001 | Lower endpoint | Starting slider | Higher slider |
| --- | ---: | ---: | ---: |
| Active selector / pacing-table value at both steps | 0 / 167 | 4 / 56 | 6 / 42 |
| World countdown, modulo 101 | 62 → 55 | 53 → 35 | 49 → 25 |
| Trigger countdown, modulo 31 | 8 → 1 | 30 → 12 | 26 → 2 |
| Outer scheduler word | 677 → 684 | 755 → 773 | 794 → 818 |
| Advances supported by all three readings | 7 | 18 | 24 |

Both gameplay countdowns and the outer word agree on 7, 18 and 24 advances
at these three settings over the same 61 resumed observer steps. The outer
word can also advance while gameplay is paused, so it
serves as corroboration within the resumed interval. This establishes a
gameplay cadence difference for the sampled match and settings. The GBA's
session settings cover cursor movement and menu sound.

A separate visual endpoint check moved the slider marker from capture x=346
to x=400 after four Right presses. Six further ten-step Right holds, with
neutral input and a capture after each, left the marker at x=400. The command
log SHA-256 is `a8ac365d2f77dcac5a942304d82a87f42dcfd276d2811195d2f4b371293985a0`.

The same opening and Settings route reached the lower visual endpoint.
Four ten-step analog-left holds moved the marker through x=320, 292, 266
and 238 from the starting x=346. Six more holds left it at x=238. The
verified 22-capture endpoint receipt SHA-256 is
`fb711a25c8a59088af7d63976570db3a59ae5f684079eae28212b1d65846ed8a`.

| Pinned observer or receipt | SHA-256 |
| --- | --- |
| ares RAM observer | `54d100a9b56d342b07c9a0fdf8fbffedeab0134010359eaa57952d0be0b0b75c` |
| Starting slider capture manifest | `d213ac2c431b51657683736122667c50f7637015efa5dc7a76de5a331b93b688` |
| Higher slider capture manifest | `0e82bf5df5668783d4b37550fab8cbb89ad5ced16e09f62f591d8ee360a25063` |
| Verified snapshot readings and deltas | `dde67e8f8fca6105b1e0ab7b5047e3637628c46f502071e9f473a105cd781252` |
| Lower endpoint capture manifest | `097698e241ea4ec33dd32e04ce02816433074466a6d78e2fd676a6a6f34ac4f0` |
| Verified lower endpoint cadence | `3f0bf4dd7bbfd5901df003871a3964fe68e75e9fc3d36e864415b51017cdf219` |

## All seven Game Speed positions

Four more fresh ares sessions used the same Challenger match and opening.
The inputs changed only inside the same four slider intervals. Three,
two or one analog-left holds reached settings 1, 2 or 3. One D-pad Right
hold reached setting 5. All sessions, including the three earlier controls,
matched the paused scheduler readings before leaving Settings.

| Selector, slowest to fastest | Pacing-table byte | Advances in 61 resumed observer steps |
| ---: | ---: | ---: |
| 0 | 167 | 7 |
| 1 | 111 | 9 |
| 2 | 83 | 12 |
| 3 | 67 | 15 |
| 4 (starting position) | 56 | 18 |
| 5 | 48 | 22 |
| 6 | 42 | 24 |

Both gameplay countdowns and the outer scheduler word match at each
advance count. The active selector and pacing-table byte stay constant
across the sampled interval. The full local result SHA-256 is
`2613267d333e199eade4aa1f58c038d15ba8825fd06483e310e83d87bcd6630b`.
Per-run manifests for settings 1, 2, 3 and 5 are respectively
`3464a9098875b3a458712278f3f5f93b5920af93ff206d8004df5419cebc54be`,
`45d2580a2d849c10d774e553ac34aa3ff3f2d1fa9b27de17f8fee83147a52122`,
`102b80273b3266cd646e5c10354b7f1b39591cbdd04c86d8de043d21def0cecb` and
`b34553bf3e9553d01f93d8ed7891d5732c1f8e2949f1d0b8d54347a16a22c448`.
These are per-window simulation advances. A GBA implementation still
needs independent display pacing and an end-to-end timing check.
