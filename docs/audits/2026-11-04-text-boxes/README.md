# In-mission text boxes

Item 1 of the [N64 parity audit](https://github.com/ggu/starcraft-gba/pull/261). How they work
is in [In-mission text boxes](../../TEXT-BOXES.md). This page holds the evidence. It contains no
ROM-derived text, art, frames, saves or traces.

## Status after merging the base

This branch later merged `11fd1dd5`, which brought in worker cargo, the late Zerg art and a T8
controller repair. Run at merge commit `ea771cbd`:

| Check | Result |
| --- | --- |
| `check_source_tree.py`, `./dev build` | Pass |
| `make -k test` | The same six failures as below, and nothing new |
| `test-sc64-text-box`, `test-sc64-text-box-game`, `test-sc64-zerg-late-art` | Pass |
| Eight product ELFs, with boxes compiled in and out | `__bss_end` and `__iwram_end` match in all eight |
| `test-sc64-terran-full-campaign` | Passes, and T8 now wins |
| `test-zerg`, `test-zerg-z8`, `test-zerg-z9`, `test-zerg-z10` | Pass |
| `test-sc64-protoss-px`, with modal boxes | All ten win |
| T1, T8, Z1, P2, P10, with boxes compiled in and out | Same results; traces identical, apart from P2's flag bits |

The merge changed some controller routes, so some of the win frames below are out of date. P10
now wins at frame 145,833, for example. `make protoss` also now builds P10, so the Protoss P1 to P7
row in the memory table has no new measurement. The ARM runs and the N64 checks were not repeated.
Z9 and Z10 needed the private Z8 sprite bank regenerated with `--include-zerg-late-art`, which the
merged base requires.

Everything below this section was run at `d70f6622`, before that merge, unless it says otherwise.

## What the N64 does

Observed on the N64 ROM in ares, using a private copy of the emulator and states. No debugger
was used. Native addresses were read from a private save state with a small script.

| Question | Finding | How we know |
| --- | --- | --- |
| Does the mission clock stop? | Yes. Elapsed seconds held still across about 2.5 seconds with a box up, while the native step counter kept moving. Clock restarted after A. | Save states in T1 |
| Which key dismisses? | A, and also Start. Start doesn't pause while a box is up. | T1 |
| Is there a hold after a box? | No. Three chained Transmissions in Z1 opened one after another on each A, inside one elapsed second. | Z1, and `0x80080DA8` arms an indefinite wait that the box's close releases |
| Who is the Advisor? | DisplayText rows use unit 229 plus the player's race for their head and show the Advisor's name. Terran head 24 and Protoss head 10 were read from live states; the Zerg entry comes from the table alone. | T1, P2, ROM |
| Is the box drawn over a running HUD? | The top-right resource display is hidden while a box is up. | T1 frames |

The first boxes in T1, Z1 and P2 opened at the same trigger points and in the same order on
the N64 and on this port's ARM probes, with the same head IDs for the speakers compared.
Other missions were not compared on the N64: their rows come from the same source program.

## Each mission still wins, with the same result

Boxes take no simulation step, so a mission must play out identically with them. To check that,
each controller ran twice from the same source, with boxes compiled in and compiled out. Both
used unsanitized `-O2` host builds, and boxes compiled in were acknowledged at once. Traces hold
every recorded input and checkpoint.

All 29 runs agree, across T1 to T10, Z1 to Z6, Z8 to Z10 and P1 to P10. T8 is the one run that
doesn't win, with or without boxes. Z8, Z9 and Z10 are game fixtures rather than controller wins.

| Evidence | Runs |
| --- | ---: |
| Trace byte-identical | 21 |
| Trace identical apart from the once-only bits in `objective_flags` (P1, P2) | 2 |
| No trace, identical output (T3, Z8, Z9, Z10, P5) | 5 |
| Win record identical (P4) | 1 |

Those P1 and P2 traces record `objective_flags` at each checkpoint, so they show the new high
bits. Masking them leaves the traces equal. Z7 has no text rows, so it wasn't in this comparison.
Its controller still wins in the suite below.

Under the P10 flags, all ten Protoss tapes then replayed with real modal boxes on host, and
their win frames and tape call counts match the
[earlier evidence](../2026-11-03-protoss-p1-p10-evidence/README.md) exactly:

| Mission | Win frame | Tape calls |
| --- | ---: | ---: |
| P1 | 24,986 | 25,167 |
| P2 | 34,026 | 34,375 |
| P3 | 48,961 | 49,747 |
| P4 | 136,750 | 137,973 |
| P5 | 5,056 | 5,115 |
| P6 | 99,906 | 101,314 |
| P7 | 48,923 | 50,152 |
| P8 | 96,889 | 99,351 |
| P9 | 150,969 | 154,594 |
| P10 | 72,441 | 73,829 |

## Which tapes changed

None. Tape files keep their keys. What changed is the players: they now dismiss a box between
tape ticks, so a box's frames never consume a tape tick.

- `tests/sc64_p8_arm_replay.c`, `sc64_p9_arm_replay.c`, `sc64_p10_arm_replay.c`,
  `sc64_protoss_arm_replay.c` and `sc64_t9_arm_replay.c` use `tests/sc64_text_box_replay.h`.
- `common/main.c`'s debug scheduler does the same for GDB-driven replay tools. Those tools
  weren't run, since this work avoided GDB. See [What didn't run](#what-didnt-run).
- `sc64_z7_arm_replay.c` needed nothing, as Z7 has no text rows.

## Row coverage

Rows that opened during these controller runs, out of the rows in the table:

| Mission | Rows | Opened | | Mission | Rows | Opened | | Mission | Rows | Opened |
| --- | ---: | ---: | --- | --- | ---: | ---: | --- | --- | ---: | ---: |
| T1 | 8 | 6 | | Z1 | 12 | 7 | | P1 | 10 | 7 |
| T2 | 14 | 12 | | Z2 | 4 | 2 | | P2 | 7 | 5 |
| T3 | 9 | 6 | | Z3 | 4 | 3 | | P3 | 2 | 2 |
| T4 | 32 | 11 | | Z4 | 16 | 14 | | P4 | 14 | 12 |
| T5 | 16 | 13 | | Z5 | 36 | 19 | | P5 | 4 | 3 |
| T6 | 15 | 12 | | Z6 | 9 | 8 | | P6 | 20 | 13 |
| T7 | 6 | 4 | | Z7 | 0 | 0 | | P7 | 10 | 7 |
| T8 | 9 | 1 | | Z8 | 12 | 2 | | P8 | 12 | 10 |
| T9 | 16 | 12 | | Z9 | 6 | 6 | | P9 | 2 | 0 |
| T10 | 3 | 2 | | Z10 | 5 | 5 | | P10 | 12 | 9 |

That is 213 of 325 rows. A controller that wins doesn't take every path, so a row left closed
isn't by itself a fault: defeat lines, for one, only open when a mission is lost. Not every
closed row was checked by hand. In particular:

- **T8 opened one,** in a controller run that didn't win before the merge. See above.
- **Z8, Z9 and Z10 are fixtures,** not controller wins. Z8's controller is still blocked.
- **T4 opened 11 of 32,** in a run limited to 30,000 ticks.
- **Hand-written missions** rest on conditions written for this work, so their closed rows have
  had the least testing. T1's and P1's defeat lines, for example, were never seen in a run.

## Memory guards

`__bss_end` and `__iwram_end` are unchanged from the base in all nine products. ROM measurements
are the `.text` plus `.data` of each ELF.

| Product | `__bss_end` | `__iwram_end` | ROM growth, bytes |
| --- | --- | --- | ---: |
| Terran | `0x02037150` | `0x03004B40` | 40,900 |
| Zerg Z1 to Z7 | `0x0203D780` | `0x03004F80` | 40,588 |
| Z8 | `0x0203DC40` | `0x03004BF0` | 40,636 |
| Z9 | `0x0203DFF8` | `0x03004BF0` | 40,724 |
| Z10 | `0x0203DFF8` | `0x03004BF0` | 40,620 |
| Protoss P1 to P7 | `0x0203838C` | `0x03004A28` | 39,644 |
| P8 | `0x02038D54` | `0x03004A28` | 40,204 |
| P9 | `0x02039134` | `0x03004A58` | 40,084 |
| P10 | `0x020394F4` | `0x03004A58` | 40,540 |

Z9 and Z10 still end at `0x0203DFF8`, leaving their 8 spare bytes.

## ARM runs

mGBA 0.10.5, one isolated process per run, log mask 7, no debugger. Nothing writes game state.

**Tape replays.** These replayed existing tapes under the P10 flags with real modal boxes:

| Run | Observations matched | Win frame | Box frames inserted |
| --- | ---: | ---: | ---: |
| P2 | 35 of 35 | 34,026 | 12 |
| P8 | 100 of 100 | 96,889 | 24 |
| P10 | 74 of 74 | 72,441 | 28 |

Each page takes two frames, a press and a release, so 24 frames is 12 pages. Every observation
matched the host replay with the same final state.

P8's tape also holds a pause-menu save at tick 31,239. A prefix run wrote that save, and a fresh
mGBA process loaded it and continued to the same win. All observations from the save tick on
matched the host run, with the same final state. Boxes opened on either side of the save.

**Product probes.** `tests/sc64_text_box_arm_probe.c` launched a mission from the ordinary menu
and idled, acknowledging each box with A. The panel was on the displayed page each time, and
the mission's frame never moved while a box was up.

| Probe | Build | Boxes | Note |
| --- | --- | ---: | --- |
| T1 | Terran | 2 | Frames 30 and 240 |
| Z1 | Z10 flags | 3 | All at frame 8, one after another |
| P2 | P10 flags | 1 | Frame 8 |
| Z10 | Z10 flags | 0 | The idle human loses at frame 10,992 before any row runs |

Z10's rows did open in its host fixture, but no Z10 box was seen on ARM.

Boxes add frames to a run, so these aren't new timing results.

## What ran

Run at `d70f6622`. Later commits on this branch change only documentation. Inputs were cloned from
the 2026-10-31 pinned set into a private `.local`, with the codec-2 audio bank.

| Command | Result |
| --- | --- |
| `python3 tools/check_source_tree.py` | Pass |
| `./dev doctor` | Pass |
| `./dev build` | Pass |
| `make -k test` | Fails only on the six failures below, which the base shares |
| `make test-sc64-text-box test-sc64-text-box-game` | Pass |
| Nine product ROMs, including memory guards | Pass |
| `make test-sc64-protoss-px`, under ASan | All ten win |
| `make test-sc64-terran-full-campaign` | Stops at T8, as below. Everything before it passes. |
| T9 and T10 controllers, under ASan | Both win |
| `make test-zerg test-zerg-z8 test-zerg-z9 test-zerg-z10` | Pass |
| `make test-protoss` | Passes, except targets needing the `protoss-p6` bank, which isn't prepared here |

The six failures, in `test_sc64_photon_cannon_renderer` and `test_sc64_unit_word_sprites`, also
fail on an export of base `88c17f15` with these inputs. They only run when private inputs are
present, and a clean asset-free clone should skip them, though that wasn't tried here.

The `make test-protoss` result rests on `make -k`, where the only error reported is the missing
bank, so `test-sc64-p6-canonical` and its dependents did not run.

## What didn't run

- **Physical hardware.** All ARM results are from mGBA.
- **GDB-driven replay tools,** including `tools/sc64_zp_controller_replay.py` and
  `tools/sc64_t5_controller_replay.py`. This work avoided GDB. The scheduler change in
  `common/main.c` that they rely on has only been compiled.
- **ARM replays other than the ones listed.** Terran has only the T9 tape player, and Zerg only
  the Z7 one. Other missions have host evidence and the probe above.
- **A save made during a real play session across a box.** The fixture covers a pause-menu
  save and a fresh load around boxes, in T1.

## Known issues

- **T8 didn't win before the merge.** `test-sc64-t8-campaign-run` reached its 200,000-tick limit
  at `d70f6622`. This didn't come from boxes: the trace was byte-identical with boxes compiled out.
  It stopped the Terran suite at that step, so T9 and T10 were run separately then.
- **The holds after a Transmission remain,** as noted in [the design](../../TEXT-BOXES.md). They
  run after the box closes. Removing them would match the N64 and change trigger timing in
  Z3 to Z6, Z8 to Z10, P3 to P8 and P10, so it would need new controller runs for those missions.
