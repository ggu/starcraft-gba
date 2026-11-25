# N64 parity capture spikes S1–S3

These are the three go/no-go spikes for the N64 capture side of the
parity harness. A new read-only ares frontend,
[`experiments/ares-headless/capture.cpp`](../../experiments/ares-headless/capture.cpp),
was built at ares `0aafd85789215e84e1e43415c07d4c88461b7899`
([build and tape reference](../../experiments/ares-headless/CAPTURE.md)).
Every run used the owned USA ROM and the JIT unless stated, held the
shared native lock, and ran alone. ROM copies, saves, tapes, RAM hashes
by region, tap records and frames stay in an ignored private folder.

| Spike | Verdict | One-line evidence |
| --- | --- | --- |
| S1 long-tape determinism | PASS | Two fresh processes (B, B2) give identical full 8 MiB hashes at all 152 checkpoints to step 151,815 in live T1 play |
| S2 clock and speed | PASS | Unpaused clock matches a Marine timer (360 of 360 ticks) and `floor(unpaused/16)` over 58,213 ticks; selector 4 → 6 follows the slider |
| S3 per-tick tap under JIT | PASS with one rule change | Records taken one step after the tick change are stable and equal under JIT and interpreter; the change-step read is torn 1.2–1.3% |

The host was heavily loaded by other work during most runs (load
average 400–950). Wall-clock rates below are therefore unreliable, and
throughput is given as main-thread CPU time.

## Runs

The campaign route is a Wasteland (T1) game resumed from a cartridge
save made by an earlier session (FlashRAM SHA-256
`76cf0ba841a568e2bd9d1c34f43b2c2c1cf214a8a699a5ffdb7db7b5017184c7`).
The tape loads it through the ordinary menu, opens the Barracks menu
and, at step 7,535, trains one Marine (9 of 10). A fresh-boot T1 tape
reaches the mission through Start, Down and A and dismisses the
opening transmissions with Start. Both routes use ordinary controller
input only.

| Run | Binary | Steps | Purpose |
| --- | --- | --- | --- |
| A `s1-observed` | `b0a3f7e3…` | 151,815 | Tap every tick with raw rows, JIT-vs-interpreter branch every 50 ticks from step 7,845, hash every 1,000 steps |
| B, B2 `s1-plain` | `b0a3f7e3…` | 151,815 | Same pad inputs as A, no tap, no branches |
| C, D, E | `b0a3f7e3…` | 9,100 | A's tape, B's tape, and A's tape without branches, hashed every 25 steps |
| `fresh1` | `481fa21e…` | 30,000 | Fresh boot to T1, tap, branch every 20 ticks (older step-aligned branch rule) |
| `settings1` | `fd61c90e…` | 9,585 | Pause menu and Game Speed slider |
| `bench2` | `4c768806…` | 27,715 | Interleaved throughput rounds, 119 branches and round trips |
| `bench3-step` | `17ce80ff…` | 19,215 | Tap on every step to find when rows settle |

Full binary hashes: `b0a3f7e3cc89671e137384e315a52babae958384ebcd954cba1534cf89648159`,
`481fa21ee201ba1b87fc801664ea5295c594d17f684df946532e9b70fe89e352`,
`fd61c90e52d6b00f844c3ef35f651520383d9af0276048bbb454925318df469d`,
`4c7688069d981bf44d98b601558c67815cd5a4404de35267a7c6a48394e681c8`,
`17ce80ff410a0124805ed2669b18ddf91f7e6b831a64a9d597514fa65b849966`.
The binaries differ only by frontend revisions in this branch: the
branch comparison rule (481fa21e → b0a3f7e3), CPU-time replies
(4c768806) and output-error checks (17ce80ff). After review the
tracked source adds stricter open, frame and close checks, refuses
diagnostic file names that already exist, cancels a pending branch
when the tap or verify settings change, and never counts empty
records as a match. It builds
`075b49483a4203b37fd9d586f134d51f1886887ec24e02c2dd7471923a7329a8`.
A 1,100-step smoke run with it reproduced the step-1,000 hash of runs
B and B2. No measurement below used it. The
four generated resource files hash as in earlier audits (`ea7fe256…`,
`bc76b4e8…`, `e37dc7f2…`, `4b560c7f…`). MoltenVK
`432e8880…6f99` and ROM `e7d21880…b020` are unchanged.

## S1: rebuild and long-tape determinism

The frontend rebuilt from a clean clone in a durable private folder.
It has no cumulative step, time, snapshot, frame, input-event or
output-size limit; one command runs at most 10,000,000 steps, and the
wrapper's default timeout is six hours.

Run A stayed in live Wasteland play to step 151,815: the unit
table was present throughout, 43–47 rows were live, and the tick
counter advanced from 19,450 to 77,670 with no tick skipped between
reads.

Runs B and B2 replayed the same 151,815-step tape in two fresh
processes with no observation other than the hashes. Their
cache-aware 8 MiB hashes are identical at all 152 checkpoints (every
1,000 steps and the end). The final hash at step 151,815 (tick 77,670)
is `0666c0205feacc89c7609fdb14f530b35a8b3eba6f3bc4c6b0a3e08e150476f0`
in both. Run D, a third fresh process on the first 9,100 steps of the
same tape, matches both at every shared checkpoint. The rebuilt final
binary reproduces their step-1,000 hash
(`6aebeb91…6222`). B took 30.0 minutes and B2 18.4 minutes; the
difference is host load. S1 passes for this tape.

**What diverged and why.** Run A differs from B from checkpoint 9,000
on: first in three 64 KiB blocks (`0x0D0000`, `0x100000`, `0x110000`),
later in up to 16. Blocks `0x0A0000` and `0x0E0000` differ at most
later checkpoints, and the blocks holding the bank arrays and the unit
table each differ once. The short
runs isolate the cause. C (A's tape) and E (the same without branches)
are identical until step 8,025, 18 steps after C's first branch at
8,007. E and D (no tap) end with byte-identical 8 MiB views at step
9,100. The in-place branch, which restores a serialized state in the
interpreter and then restores the JIT state, does not return the main
run to the same trajectory. The pinned core re-powers and recreates
the Vulkan renderer on every restore. The tap alone does not change
game state.

One transient difference also appeared. At step 4,475 both tapped runs
(C, E) differ from the untapped D in three blocks at
`0x190000–0x1B0000`, a framebuffer-sized area. Both equal D again at
4,500 (full 8 MiB hash); C stays equal until its first branch, and E
stays equal through 9,100. Full-RAM hashes taken at a step boundary
can therefore catch rendered output in a different state. Goldens
should hash game-state regions, or exclude the framebuffer, rather
than all 8 MiB.

## S2: clock and speed

**Text boxes.** On a fresh T1 boot the two opening transmissions
(Advisor and SCV, seen in frames) set the pause halfword `0x800F0C20` to 1 for 85
and 55 ticks. Through both boxes the tick counter `0x800B1E84` kept
advancing, elapsed seconds `0x800B9368` stayed fixed, and the unit
rows, the four bank arrays and the RNG words were unchanged on every
tick. Over the 7,221 ticks of that run,
`(counter − paused ticks) − 16 × seconds` stays within 0…15, so
`seconds = floor(unpaused / 16)` holds exactly from the mission start.

**Pause and a build timer.** In run A the Marine row's build timer
(`+0xA8`, status bit 31 clear) counted 360 → 0. Across its life the
unpaused clock advanced 360 ticks and the timer 360. During the pause
(Start, Options, Settings, Back, Resume) it was unchanged on all 355
paused ticks while the counter kept running. The floor relation holds
over all 58,213 ticks when seconds is read one step after each tick
change, with the pause halfword taken from the change step (range
3,546…3,561, a fitted constant offset for the resumed save; the
paused ticks before the save were not observed). Read on the
change step itself, it is off by one 53 times, because seconds is
updated later in the same tick (see S3).

**Speed selector.** The settings word `0x800D13E4` reads 4 in every
fresh T1 tick and on the resumed save. In the pause menu,
Options → Settings shows Game Speed, Quickeys Timer, Unit Info Timer,
Cursor Speed and MiniMap Fade sliders. Three D-pad Right presses moved
Game Speed to its maximum. The word changed from 4 to 6 when B left
the Settings screen, and the run continued at selector 6. Start does
not resume from the pause menu; Down three times and A (Resume) does.
A fresh campaign mission runs at selector 4 (56 ms per tick).

**Path to a clock.** Unpaused ticks = counter minus the ticks whose
change step shows a nonzero pause halfword; read seconds (and unit
rows) one step after each counter change. The pause halfword covers both text boxes and the pause menu.

Not done here: the design's extra S2 item, one lab captured at both
selectors with identical per-tick unit traces. Whether the selector
persists in flash is also unknown. The slider change was not
followed through a save.

## S3: per-tick tap under JIT

**Reads.** The tap reads the counter after every step. When it changes,
it reads all 900 rows through the cache-aware view, then reads them
again on the next step. In run A, 116,437 reads gave zero alias
conflicts (no line held by both cache indexes), zero link words
outside the table or off a row start, and zero asymmetric links. A
direct RDRAM copy equalled the core's own debugger read for all
8 MiB. In 55,074 table reads the cache-aware view differed from plain
RDRAM, so reading through the data cache matters.

**Torn records.** The read on the tick-change step is not reliable. In
run A, 772 of 58,215 change-step reads (1.33%) differed from the read
one step later without another tick change. The rate was 1.16% at
selector 6 in the step-mode run. Most cases follow one pattern. About
every 101 ticks (~254 steps) a long tick's unit pass does not finish
within the video field: the counter has already advanced, but later
rows still hold the previous tick's timers (offsets `0x54`, `0x85`,
`0x112`, sometimes the order byte `0x4D`). Partial cases change only
rows 14 or 30 onward, which is the list mid-pass. Elapsed seconds tears
the same way. In step mode, 3,782 records were taken one step after a
tick change. Rows changed again before the next tick in 4 of them
(0.11%): 3 on the second step (of 1,219 second-step reads) and 1 on
the third (of 1,217). All four were one field of the training Marine.
A tick never changed on two consecutive steps, even at selector 6.

So the S3 rule becomes: record tick *t* from the read one step after
the counter changes to *t*. Under that rule 0.11% of records change
afterwards, below the 1% bar. The change-step read alone fails it.

**JIT versus interpreter.** A branch serializes the state at a tick change,
lets the JIT reach the next tick plus one step, then replays the same
pad inputs from the saved state with the interpreter and compares the
records for the same tick value. If the interpreter has not reached
that tick when the recorded inputs run out, it holds the last pad
state for up to 8 more steps.

| Run | Branches | Follow-step records equal | Change-step records equal | Step skew 0 |
| --- | --- | --- | --- | --- |
| A (mission length) | 1,143 | 1,143 | 1,138 | 1,141 |
| bench2 | 79 | 79 | 79 | 79 |

The five change-step mismatches in A have equal follow-step records,
which fits a torn change-step read in one mode only. In two branches the interpreter
crossed the boundary two VI steps later than the JIT. Records are
therefore matched by tick value, not step. The full 8 MiB view is
never equal across modes at the same tick: 10 to 8,420 bytes differ
(median about 270), with the first differing byte most often at
`0x800DDE18`, `0x800A23E3` or `0x800A5AC7`. Same-mode round trips (40) matched records and full
views exactly. Goldens bound to one CPU mode, as the design says, are
still required. In the sampled Wasteland branches the follow-step unit
records agreed across modes and whole-RAM hashes did not; other
scenarios were not branched. The older fresh-boot run, which matched by step rather
than by tick, had 312 of 314 equal. Its two misses were step skew.

**Throughput.** Main-thread CPU time over interleaved rounds of 400 JIT
steps or 200 interpreter steps in one process:

| Mode | Tap off | Tap on tick change | Tap every step |
| --- | --- | --- | --- |
| JIT | 136 steps/s | 133 steps/s | 137 steps/s |
| Interpreter | 55 steps/s | 55 steps/s | 55 steps/s |

The tap costs less than the round-to-round spread (about 10%).
Branching every 5 ticks drops the JIT to 38 steps/s. Wall-clock rates
ranged from 11 steps/s under the heaviest background load to 138 steps/s
(B2, 151,815 steps in 18.4 minutes). Run A took 57.5 minutes with the
tap and branches every 50 ticks.

## Consequences for the design

None of the design's fallbacks is needed: long tapes replay without
checkpointed segments, the tick counter works as the tick source, and
the JIT tap needs no interpreter-only windows. What changes:

- The tap can run under the JIT. Key records by the counter value and take
  them one step after it changes. Flag a record when the following
  step still changes it.
- Do not branch in place during a capture. JIT-versus-interpreter checks
  must run in a separate process from a state, or the capture must be
  treated as its own trajectory. The design's "tested once per binary"
  stance fits: run the check once, offline, not inside golden captures.
- Hash game-state regions rather than all 8 MiB, or exclude rendered
  output. A whole-RAM hash catches a framebuffer area that can differ
  between otherwise identical runs.
- Treat the campaign default as selector 4 and record the selector per
  tick. It changes mid-mission from the pause menu.

## Still unproven

- S1 covers one tape: a resumed Wasteland game, mostly idle after
one Marine and one pause. Combat, many units, a mission end and Blood
Bath were not replayed twice. Pairs at other lengths, on other
binaries or with observation enabled in both runs were not compared.
- The frontend does not flush cartridge saves to disk on exit. No run
  saved in game, and FlashRAM state across a restore was not checked.
- A cache line held only by the alternate index (virtual bit 12 flipped)
  is used and is not counted separately; no line was held by both.
- The per-type branch of the record format, liveness rules and offsets
  other than `+0x0C`, `+0x64`, `+0xA8` and `+0xD8` were not re-validated here.
- Throughput was measured on one loaded machine and one mission.
- Whether the off-tick change to one field of the training Marine
  (offset `0x8A`) is a UI-driven update or a long tick is not known.
