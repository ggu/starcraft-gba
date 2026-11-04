# P8 and P9 on the current integration

P8 and P9 were qualified before #134 shared bounded ranged routing. This audit
requalifies both explicit candidates on the current integration. Neither is
promoted; canonical Protoss stays P7.

## P8 needed a controller repair

The old P8 tape no longer wins. The controller lost at frame 28,201 when Fenix
died. It bisects to #134 (first bad `63665045`, last good `7181c093`), and it
wins at frame 88,769 with `SC64_RANGE_WORK` off.

I did not find an engine defect behind this. The two builds run identically to
about frame 18,000 and then diverge during the raids, and the old route had
little margin in either build. Half its Probes were idle while over 15,000
minerals remained:

- **Stacked Probes.** Idle Probes stack on a spent patch, and the route could only
  select one Probe at a time.
- **Cargo.** A Probe can pick up minerals between being chosen as a builder and
  placing its building. Once idle, it refuses the mineral order. Draft #111 changes
  this area, so the engine is untouched. The route instead taps the Assimilator,
  which drops the cargo, then presses STOP.
- **Spending.** Gas banked unused while a large mineral bank waited on Gateways,
  which only followed Probe count.
- **Front unit.** The army waited on the unit nearest the Cell in a straight line,
  often across a cliff. It now follows the unit with the shortest walk.

P9's controller needed no change.

## Results

Both tapes were run at commit `04cca900`, on base `d60e75eb` plus #253's controller
changes. Inputs were cloned from the 2026-10-31 pinned set into a private `.local`,
with the codec-2 audio bank. Host runs use ASan and UBSan. ARM runs use mGBA 0.10.5,
one isolated process per run, log mask 7, no debugger. Replays write no game state.

| Check | P8 | P9 |
| --- | --- | --- |
| Host controller win | Frame 106,881 (109,513 calls) | Frame 168,265 (170,259 calls) |
| ARM full replay, observations matched | 110 of 110, same win | 171 of 171, same win |
| ARM prefix, observations matched | 32 of 32 | 33 of 33 |
| Pause-menu save point | Frame 31,035, structure under construction | Frame 31,423, structure under construction |
| Fresh-process resume on ARM, observations matched | 79 of 79, same win | 139 of 139, same win |
| Final state digest | `1084059693` | `435603684` |
| ARM SRAM against host | Record identical | Record identical |
| Carried startup/save matrix | 234 configurations pass | 234 configurations pass |
| `make verify-protoss-p8` / `verify-protoss-p9` | Pass | Pass |
| `__bss_end` / `__iwram_end` | `0x02038d54` / `0x03004a28` | `0x02039134` / `0x03004a58` |

The SRAM files differ only in the unwritten bytes after the record (0x00 on host,
0xFF in mGBA), starting at byte 16,324. Both products stay below the unchanged
guards of `0x0203e000` and `0x03005000`.

### ARM timing

Whole tape, including menu and briefing frames, at 16,777,216 cycles per second:

| | P8 | P9 |
| --- | ---: | ---: |
| Frames | 109,513 | 170,259 |
| Mean cycles per frame | 1,762,057 | 1,840,651 |
| Equivalent frames per second | 9.5 | 9.1 |
| Mean update cycles | 1,039,860 | 1,187,233 |
| Peak frame cycles | 26,666,990 | 32,303,107 |

Both are far below the 30-fps target. Peak frames were not broken down.

## Reproduce

With private inputs prepared and no shared input regenerated:

```sh
make test-sc64-protoss-p8 sc64-p8-arm-replay SC64_AUDIO_DIR=.local/sc64/audio-codec2/generated
make test-sc64-protoss-p9 sc64-p9-arm-replay SC64_AUDIO_DIR=.local/sc64/audio-codec2/generated
```

Then run each ROM with `tools/sc64_p8_arm_replay.py run` and compare, as in the
[P8 audit](../2026-10-29-p8-campaign/README.md). For P9 pass `--mission 28 --prefix P9`.
The tool writes `p8-lines.txt` for both missions.

## Not claimed

- **Fragile routes.** The P8 win is one tape. Nearby build-order settings still lose
  when raids reach Fenix, so this shows the mission can be won, not that this route
  is reliable. The P9 tape was run once at this commit.
- **One save point each.** Other save states are not covered.
- **Authored AI.** The AI backends, trigger cadence and transmission durations are
  authored, not native. Native combat equivalence is not claimed.
- **Replay ROMs.** ARM results are from the replay probes, which share the candidate
  flags but use their own main loop. The production ROMs were linked and verified,
  not replayed.
- **Hardware.** Nothing was run on a physical GBA.
