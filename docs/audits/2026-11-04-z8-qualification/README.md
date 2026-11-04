# Z8 "Eye for an Eye" qualification

Z8 now has an ordinary-input win, an input-only ARM replay of its key tape, and a
pause-menu SRAM save with a fresh-process continuation. The candidate stays
`zerg-z8`; the default Zerg ROM is unchanged.

## Town AI moved toward the N64

The authored town AI sent far more than the N64 does. With an idle human, its two
towns reached 84 combat units by about 11,000 updates and sent 6 to 16 units every
2,400 to 3,600 updates. This change moves both toward the native values. It does
not match them.

**Waves** now use the scripts' attack commands, read as native steps. Five engine
updates are four 42 ms native steps.

| Town | First wave (updates) | Gap (updates) | Units per wave |
| --- | ---: | ---: | ---: |
| `Zer8` (slot 2) | 8,125 | 10,000 | 5 |
| `Ze8T` (slot 1) | 39,300 | 17,900 | 1 |

**Garrison caps** stop production at the counts the N64 settled on with an idle
human. Units alive and queued both count.

| Town | Zealot | Dragoon | Scout | Reaver | Total |
| --- | ---: | ---: | ---: | ---: | ---: |
| `Ze8T` | 0 | 7 | 2 | 4 | 13 |
| `Zer8` | 11 | 12 | 4 | 4 | 31 |

The N64 counts were read from its unit table in private save states. They were
the same at native steps 6,062 and 9,184.

Where this still differs from the N64:

- **Short idle observation.** The counts come from one idle run of about 9,184
  native steps. Nothing was observed with a human attacking or later in a mission.
- **Rebuilding unknown.** Whether the N64 rebuilds to these counts after losses is
  not established. Ours does, while the town's wallet lasts.
- **Later attack groups not modeled.** A wave here is one size on a fixed gap. The
  scripts' attack groups are averaged, and no native wave was watched arriving.
- **Units we don't train.** Each N64 town also held 2 Shuttles and 2 Observers,
  which these towns don't train.
- **Two mission loads.** The native unit table moves between loads, and the
  comparison spans two separate loads of the mission.

Trigger-created raids are unchanged. Save generation 46 and the state layout are
unchanged. `sc64_z8_ai_state_valid` now allows each town's own longest timer.

## Results

Tested at commit `a96a5eef`, on base `88c17f15`. Inputs were cloned into this
worktree's private `.local`. Host runs use ASan and UBSan. ARM runs use mGBA 0.10.5,
one isolated process per run, log mask 7, no debugger. Replays write no game state.

| Check | Result |
| --- | --- |
| Host controller win | Frame 39,617 (40,335 calls), both Nexuses dead, Kerrigan alive, 0 faults |
| Key tape | 4,601 transitions, stop at call 40,335, resume at call 17,374 |
| Host full, prefix and resume | Pass, same win |
| ARM full, against host | 41 of 41 observations, same win, 139.3 s wall |
| ARM prefix, against host | 18 of 18 observations, 56.8 s wall |
| Pause-menu save point | Frame 16,943, one structure under construction, `menu_last` 0 |
| Fresh-process resume on ARM, against host | 25 of 25 observations, same win, 85.7 s wall |
| Fresh-process resume on ARM, against ARM full | 24 of 24 observations, same win |
| Final state digest | `2101272045` |
| ARM SRAM against host | Record identical |
| `make verify-zerg-z8` | Pass |
| `make test-zerg-z8 test-zerg-z9 test-zerg-z10` | Pass |
| `make zerg-z9 zerg-z10`, which share `sc64_z8_ai.h` | Pass, including their memory guards |
| `python3 tools/check_source_tree.py` | Pass |
| `./dev doctor`, `./dev build`, `./dev test` in a clean checkout | Pass |

The SRAM files differ only in the unwritten bytes after the record: 0x00 on host,
0xFF in mGBA, from byte 16,328.

| ROM | SHA-256 | `__bss_end` | `__iwram_end` |
| --- | --- | --- | --- |
| Playable `zerg-z8` | `e9ba2af4…c1` | `0x0203dc48` | `0x03004bf0` |
| Replay, full | `51575009…9d` | `0x0203dd18` | `0x03004bf0` |
| Replay, prefix | `f2fce1ae…75` | | |
| Replay, resume | `231c4504…fa` | | |

Both measured products stay below the unchanged guards of `0x0203e000` and
`0x03005000`.

### ARM timing

Whole tape, including menu and briefing frames, at 16,777,216 cycles per second:

| | |
| --- | ---: |
| Frames | 40,335 |
| Mean cycles per frame | 1,503,295 |
| Equivalent frames per second | 11.2 |
| Mean update cycles | 790,387 |
| Peak frame cycles | 33,136,092 (about 1.98 s) |

This is far below the 30-fps target. The peak frame was not broken down.

## Controller

`tests/sc64_z8_campaign_run.c` is ordinary input through `game_update()`. Live
state only chooses inputs. `tests/sc64_zerg_late_controller.h` holds helpers meant
for Z8 to Z10.

The route: Hydralisks from up to eight northeastern Hatcheries, squads at each of
the three raid guards, a charge through the east town's gate, then the east army
and the southern squad on to the west Nexus. Killing a town's Nexus stops its
raids, and both ends the mission.

### What made the difference

A win recorded before the save fix was lost as soon as the fix changed the inputs.
Hundreds of swept settings then also lost. These were controller faults. No mission
or engine change was made after the caps:

- **Save state.** The pause-menu save kept `menu_last` at 3, so the resumed tape
  diverged at about tick 19,000. It now clears that with any unit that selects alone.
- **Production starved.** Far trips, such as visiting a corner squad every 1,000
  frames, left 18 larvae and about 4,000 minerals unspent. Far visits are now rarer
  and more Hatcheries come sooner. The army went from 43 at frame 30,086 to 84 at frame 29,064.
- **Orders reached a screen of units.** One order moved about 20 of 70 Hydralisks.
  The controller now repeats it around units that don't have it yet.
- **Rally recalled the assault.** The rally job no longer runs during one.
- **Stopping before the gate.** Idle Hydralisks died to Reavers on a leg just short
  of them. The army now attack-moves through.
- **Wrong raid guard.** One raid spawned at frame 34,045, ahead of its 34,170 table
  entry, and the cursor left for the wrong corner. The controller now picks the row
  with a 600-frame margin and stays at the guard while a raid moves.

## Reproduce

With private inputs prepared and no shared input regenerated:

```sh
make test-sc64-zerg-z8 sc64-z8-arm-replay
```

Then run each ROM with `tools/sc64_p8_arm_replay.py --mission 19 --prefix PX run`,
seeding the resume with `--sav-in` from the prefix's save, and compare with
`--from-tick 17374`, as in the [P8 audit](../2026-10-29-p8-campaign/README.md).

## Not claimed

- **N64 match.** Waves and garrisons moved toward observed N64 counts. They don't
  match them, for the reasons listed above. The AI backend and trigger cadence are
  authored, and native combat equivalence is not claimed.
- **A reliable route.** The win is one tape. In the last sweep, 7 of 96 settings of
  assault size, save timing and Kerrigan's position won, and several of those are the
  same run. This shows the mission can be won, not that this route is reliable or
  how hard a person would find it.
- **One save point.** Other save states are not covered.
- **Replay ROMs.** ARM results are from the replay probes, which share the candidate
  flags but use their own main loop. The production ROM was linked and verified, not
  replayed.
- **Hardware.** Nothing was run on a physical GBA.
- **Promotion.** The default Zerg ROM stays Z7. Promotion waits on Z9 and Z10.
