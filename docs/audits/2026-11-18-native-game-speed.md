# N64 custom-game speed and mineral income

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
measure banked minerals at four samples. Source simulation ticks, speed
endpoints and a general worker mining rate still need separate validation.

| Pinned input or log | SHA-256 |
| --- | --- |
| Owned USA ROM | `e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020` |
| ares visual observer | `eb8d30c00dee5176188cdc91367c219c9c3aa73d0079cd6a0273bd3ba8490aa5` |
| Starting slider commands | `b4d0d536739596bef37515d0137c4ee79e7447c518ae0d13810542c9c7309585` |
| Higher slider commands | `34a853ebfba37f2d531d576d7a845a83dc394967f773822fb0831a5a646c6cff` |

Both isolated manifests passed. Captures and game data stay local.
