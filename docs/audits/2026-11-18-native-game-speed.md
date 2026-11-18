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
snapshots from the same emulator executable. At completed observer step
5,370 both runs were in Settings with the pause flag set. The two visible
slider positions differed, but the active selector and pacing-table value
were still 4 and 56 in both runs. The world and trigger countdowns were
also identical at 66 and 12.

Both runs had resumed at the 5,940 and 6,001 snapshots. The nearby field
captures in both runs showed 74 minerals and 4/9 supply.

| Snapshot reading, steps 5,940 → 6,001 | Starting slider | Higher slider |
| --- | ---: | ---: |
| Active selector / pacing-table value at both steps | 4 / 56 | 6 / 42 |
| World countdown, modulo 101 | 53 → 35 | 49 → 25 |
| Trigger countdown, modulo 31 | 30 → 12 | 26 → 2 |
| Outer scheduler word | 755 → 773 | 794 → 818 |
| Advances supported by all three readings | 18 | 24 |

Both gameplay countdowns and the outer word agree on 18 advances at the
starting slider and 24 at the higher setting over the same 61 observer
steps. The outer word can also advance while gameplay is paused, so it
serves as corroboration within the resumed interval. This establishes a
gameplay cadence difference for the sampled match and settings. The lower
endpoint, the full selector-to-cadence mapping and physical refresh timing
remain open. The GBA's session settings cover cursor movement and menu sound.

A separate visual endpoint check moved the slider marker from capture x=346
to x=400 after four Right presses. Six further ten-step Right holds, with
neutral input and a capture after each, left the marker at x=400. The command
log SHA-256 is `a8ac365d2f77dcac5a942304d82a87f42dcfd276d2811195d2f4b371293985a0`.

| Pinned observer or receipt | SHA-256 |
| --- | --- |
| ares RAM observer | `54d100a9b56d342b07c9a0fdf8fbffedeab0134010359eaa57952d0be0b0b75c` |
| Starting slider capture manifest | `d213ac2c431b51657683736122667c50f7637015efa5dc7a76de5a331b93b688` |
| Higher slider capture manifest | `0e82bf5df5668783d4b37550fab8cbb89ad5ced16e09f62f591d8ee360a25063` |
| Verified snapshot readings and deltas | `dde67e8f8fca6105b1e0ab7b5047e3637628c46f502071e9f473a105cd781252` |
