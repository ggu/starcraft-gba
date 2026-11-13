# Blood Bath: mixed AI lobby and opening economy

The owned StarCraft 64 ROM reached **Melee → Blood Bath** through ordinary
controller input on the pinned ares visual observer. The map card reads
64×64 and four starting slots. The lobby shows human Terran, then computer
Zerg, Terran and Protoss. This checks the same map offered by the GBA port;
its other map, Outpost, is an authored map and was not compared here.

An initial map navigation run captured Meltdown, Blood Bath and Spread Out
on consecutive Right presses. From the established starting map row, 11
Right presses select Blood Bath. A second clean run captured that lobby,
pressed Start, then held neutral throughout both gameplay captures.

| N64 capture | Observation |
| --- | --- |
| Lobby, observer step 3,244 | Blood Bath, Melee, four slots; human Terran, computer Zerg/Terran/Protoss |
| Field, step 3,754 | Terran Command Center, 50 minerals, 4/10 supply |
| Field, step 4,254 | Terran Command Center and workers, 66 minerals, 4/10 supply |

The normal Z10 GBA ROM launched the same map and player choices through
physical buttons. At game frame 1 it reported 50 minerals, zero gas and
four workers with 4/10 supply. It had one base for each of the four owners,
four workers each, and the Zerg AI's starting Overlord. The three AI wallets
each contained 50 minerals. With neutral input throughout, the human's
mineral readings at frames 61, 121, 181 and 241 were 50, 82, 90 and 114;
supply stayed 4/10. The runner used named read-only state. It did not write
guest memory. GBA ROM SHA-256:
`bb286c9c4fe3c70c12b0dec7abb298ba24576f2213dd5dbf8fcb3b692dc70412`.

Both games start Terran at the same displayed minerals and supply and accrue
minerals without a player mining order. N64 observer steps and GBA game
frames are different measurements, so this does not establish matching
mining rates. The N64 captures do not show the three AI bases, their internal
wallets, or a match outcome. In particular, the GBA AI balances are not
evidence for the N64 balances.

The visual-only observer used CPU/RSP recompilers, no RDRAM capture or guest
writes, and the existing disk, output and wall limits. Both runs returned
zero, verified their command ledgers and three requested frames, and retained
unchanged inputs. The second plan SHA-256 is
`1b348844c37c9672d66c67a9e069048c973fa1d73e4f81fb2fa2291fe08ed23c`;
its three raw frame SHA-256 values are:

- `6258ce9e5df4470cab221f2a37417469e457a4f8e8ba43ec0787edee16628c77`
- `f714c0433d55a26d0da9df671344a292f1a222c1876a7b733e7de67143b5316a`
- `ac57b7c8d8612acbd4701e52e7311468a19c051a26892ecadd098c6716c3f58b`

## Named Zerg and Protoss openings

Two more clean N64 runs selected the named human races while leaving the
computer slots Zerg, Terran and Protoss. One Right on the human race row
selects Protoss from Terran; three select Zerg, passing Random. Both runs
showed the chosen race in the lobby before Start and held neutral after it.

| Human race | Lobby step | First gameplay capture | Later gameplay capture |
| --- | ---: | --- | --- |
| Terran | 3,244 | 3,754: Command Center, 50 minerals, 4/10 | 4,254: 66 minerals, 4/10 |
| Zerg | 3,424 | 3,934: Hatchery and Overlord, 50 minerals, 4/9 | 4,434: 66 minerals, 4/9 |
| Protoss | 3,334 | 3,844: Nexus, 50 minerals, 4/9 | 4,344: 66 minerals, 4/9 |

The normal GBA Z10 cartridge after #400 started all three lineups through
physical buttons with the same map and named AI races. At frame 1 every
human race had 50 minerals and zero gas. Terran started at 4/10 supply;
Zerg and Protoss started at 4/9. Each run sampled the same mineral sequence
(50, 50, 82, 90, 114) at game frames 1, 61, 121, 181 and 241. Supply stayed
at the opening values. These are matched opening HUD values and evidence of
automatic mining on both systems. The N64 pictures do not verify gas or
AI wallets, matched rates, AI difficulty or long-match behavior.

The GBA ROM SHA-256 is
`d7c219637dbebe8223001a35beb717678cc008ec370f2062ace095196b7aff67`.
Both new N64 observer manifests passed with unchanged inputs and verified
their three captures. Plan SHA-256 values:

- Zerg: `b3fa3ac28c0c6c89fc53702c958f2172a54201c6b6339b1f2c0707dfbd698e53`
- Protoss: `2b02ca34275770801fd85418e9e1ffad4b3723c4477946f9650406b47dc1bb61`

## Individual computer slot options

On the third computer row, ordinary Right presses from Computer Protoss
produced Computer Random at step 3,329, Closed at 3,384 and Computer Zerg
at 3,439. The first two computer rows stayed Zerg and Terran. The N64
uses individual slot choices in addition to named race choices.

A second input plan closed the first and third computer rows and kept
only Computer Terran on the middle row. The lobby at step 3,694 showed
exactly that arrangement. Start reached the field: the readable HUD at
steps 4,204 and 4,704 showed 50 then 75 minerals and unchanged 4/10
supply. Both field captures have corruption across their lower halves,
so they do not establish the computer's spawn, camera position, or
pathing. The GBA port currently exposes a count of one to three AIs and
named races for those slots; it has no individual Random or Closed option.
Compare actual starting positions before assuming that closing an N64
slot is equivalent to reducing the GBA count.

Both visual-only observer manifests passed with three frames and
unchanged input provenance. Plan SHA-256 values:

- Computer option cycle: `a34a47e416565bd795a373003fded0664d5ff9f7e170b5bb60ecf16c5ed6176b`
- Middle computer slot only: `15bb40d6e5ab35965bb23fd0a895560d3f8d0855f018dc083bea0ff3d9bf463b`

The ROM, input tape, screenshots and runner outputs remain private. See the
[earlier Challenger observation](2026-11-07-n64-visual-only-jit/README.md)
for the same observer's provenance and limits.

## Sparse seat spawn positions

Read-only, cache-aware ares samples from the owned USA ROM verified the
actual first buildings in four single-opponent Blood Bath Melee launches.
The human played Terran. The setup screens verified the active row
before ordinary Start input. Each opening had a full-health town hall and
four race-specific workers per owner; the linked actor list also matched all
60 source resource positions. The camera origin identifies the human base.

| Active computer row | Human, native pixels | Computer, native pixels |
| --- | --- | --- |
| Middle only | (128, 240), northwest | (288, 1808), southwest |
| Middle only, Start delayed 137 callbacks | (128, 240), northwest | (288, 1808), southwest |
| First only, same launch timing as original | (128, 240), northwest | (288, 1808), southwest |
| Third only (Protoss CPU) | (128, 240), northwest | (288, 1808), southwest |

The GBA's middle-only start currently maps to the northeast (880, 120)
in half-scale pixels. The southwest native location corresponds to
(144, 904) on GBA. This disproves the current middle-seat placement for
the observed cases and shows that the sole opponent uses the same start in
all three rows. A fifth launch, with Human Terran and Zerg/Terran/Protoss CPUs,
put the human northeast (1760, 240) and the CPUs southeast (1760, 1808),
northwest (128, 240) and southwest (288, 1808) in lobby order. These three
read-only samples again matched all 60 map resources. The table above
records one set of draws; those locations are not fixed by row or count.
Preserve the resolved placement explicitly before changing the startup rule:
already-written GBA saves must keep their old AI fallback locations.
The setup exposes a separate [Game Type row](#single-player-game-type-row).

## Seeded Melee start draw

The player records and camera identify the human independently of race or
lobby row. In all ten qualified runs, three read-only samples agreed on the
opening halls and matched all 60 Blood Bath resource placements. Source
marker IDs and full native world coordinates are 0 southeast (1760, 1808),
1 southwest (288, 1808), 2 northwest (128, 240), and 3 northeast (1760, 240).

| Lobby after the human Terran row | Recovered seed | Human marker | CPU markers in visible row order |
| --- | --- | --- | --- |
| Closed / Terran / Closed | `0xf3ae` | 2 | – / 1 / – |
| Same, Start delayed 137 callbacks | `0xfc9c` | 2 | – / 1 / – |
| Terran / Closed / Closed | `0xf3ae` | 2 | 1 / – / – |
| Closed / Closed / Protoss | `0xf3ae` | 2 | – / – / 1 |
| Zerg / Closed / Protoss | `0xf3ae` | 0 | 2 / – / 1 |
| Zerg / Terran / Closed | `0xf3ae` | 0 | 2 / 1 / – |
| Zerg / Terran / Protoss | `0xf3ae` | 3 | 0 / 2 / 1 |
| Same, Start delayed 50 callbacks | `0xf6e0` | 2 | 1 / 0 / 3 |
| Closed / Terran / Closed, Start delayed 37 callbacks | `0xf618` | 1 | – / 3 / – |
| Human Random (resolved Zerg); Zerg / Terran / Protoss | `0xf3ae` | 2 | 3 / 0 / 1 |

The private USA-ROM instruction trace at `0x8003e458` copies the occupied
player rows in ascending order and assigns their starting locations in
reverse order. It draws an index into the ascending list of available
marker IDs and removes that marker before the next assignment. The
bounded draw at `0x8003ddf8` advances the 32-bit state by multiplying by
`0x015a4e35` and adding 1. It scales the resulting high 15 bits to the
number of remaining markers. The source-only selector accepts the
occupied rows, legal marker IDs and explicit seed. Its host sanitizer
fixture reproduces all ten observed assignments and checks every valid
eight-slot row and marker mask.

The native launch path uses the low word of `osGetTime() / 46875` to seed
this RNG on the observed mode path; a separate branch uses a fixed seed.
We recovered each launch seed independently by reversing the RNG using
the recorded state and draw count in all three opening snapshots. A
controller callback count is not the seed. In the Human Random capture,
the live player is Zerg and the observed markers match after one prior RNG
advance (pre-placement state `0xa39d7707`), consistent with a race draw
consuming the stream. The kernel vector supplies that explicit state; it
does not assume the GBA's existing Random chooser shares the stream.
A GBA launch can use its own
monotonic elapsed milliseconds, with an explicit seed for host
verification, without claiming the two machines sample identical timing.
Keep the placement resolved in SRAM so loading or restarting cannot
draw again. The source-only selector does not change the default GBA
launch policy, settle native Random race selection, or cover other game types.

The private observer manifest SHA-256 values for the two-CPU and
alternate-seed cases are:

- Zerg / Closed / Protoss: `8fe4701b16bf684e1ef68ee1094b7b5b024359620981e03f1244f5b614a8214c`
- Zerg / Terran / Closed: `0273c8548c2c217d351f30ae8a814c9caff747b75dca06fdb9d62972cffde399`
- Dense, 50 extra callbacks: `5cae966eb41394d8f0a363b16448390dd16bd88edbc5f18c20ff1f47dd132011`
- Middle only, 37 extra callbacks: `7f5b5bd28668f61028ff978ac4c0747377c56fa71586b22ea17c939612931f70`

- Human Random (resolved Zerg), dense: `b8cd9948e6a3dfe4e2a420aa9244e0bc9648684409681da760505525238eac63`

### Random races consume the launch stream first

The USA ROM launch path calls its race resolver at `0x80022b1c` before
entering start assignment at `0x80022b2c`. The resolver `0x8003e88c`
scans active player rows in ascending order. Only race code 6 (Random)
requests a draw in [0, 2] from the same native RNG; the outputs identify
Zerg, Terran and Protoss in that order. It then writes the resolved
races back in the same row order. A named race or a closed row does
not consume a draw in Melee. Placement uses the resulting state.

The first observed Human Random launch independently checks the sequence:
seed `0xf3ae` resolves the human to Zerg, leaves the RNG at
`0xa39d7707` before placement, then gives human/CPU marker IDs
`[2, 3, 0, 1]`. Three further owned-ROM runs showed the same reset seed
with CPU Random. Each case verified its lobby and three read-only
opening actor samples, matching all 60 resource positions and four
workers per hall.

| Human / CPU choices | Resolved human / CPU races | Human and CPU marker IDs by row | State before placement |
|---|---|---|---|
| Terran / Closed / Random / Closed | Terran / – / Zerg / – | 0 / – / 1 / – | `0xa39d7707` |
| Terran / Random / Random / Protoss | Terran / Zerg / Terran / Protoss | 1 / 2 / 3 / 0 | `0xb953c674` |
| Random / Random / Random / Protoss | Zerg / Terran / Zerg / Protoss | 2 / 0 / 1 / 3 | `0x1d976e05` |

The joint Human/CPU Random case checks that the human draws before
the ascending CPU rows. The first two draws resolve to different races, and
the human viewport identifies the Zerg hatchery at `(128,240)` as
the human base. The CPUs open at `(1760,1808)`, `(288,1808)` and
`(1760,240)`, in row order. All three RAM samples reverse the native
RNG counter/state to the same `0xf3ae` reset seed.

The native manifests are `3eaa2d7e49d36bc2dfc3e7d43ab4bf2d0fb5a098bea3c977f5418d9426988dcb`
(sparse), `bf3bafeca0d1835d7bb2ffaad160aa7c62047adb05faf6f115e9cc02f245ad75`
(dense), and `fae8647d253a2a7c353b8566b7d495b1dcfb19a0402112415c41e79335151615`
(joint Human/CPU Random), SHA-256. The bounded pure fixture uses these captures as
expected vectors and adds malformed and unobserved gap controls.

The opt-in GBA Blood Bath Melee preview now uses one elapsed-time seed
for human and CPU Random and starts. It persists the resolved human
and CPU races and the same v2 start-order field; a load or restart
draws nothing. Default builds and Outpost keep their existing
named human selector. The [matched Free for All Random launch](#paired-random-launches) below
checks the same selector in that mode.

## Single-player Game Type row

Two visual-only runs of the owned USA ROM in Ares with the Expansion Pak
enabled reached Single-Player → Custom using ordinary controller inputs.
With the Game Type row highlighted, two separate Right presses cycle
**Melee → Free for All → Melee**. Map and race rows stay fixed:

| Map | Starting slots | CPU rows shown | Melee, Free for All, Melee observer steps |
| --- | ---: | --- | --- |
| Challenger | 2 | Zerg | 2,499 / 2,554 / 2,609 |
| Blood Bath | 4 | Zerg / Terran / Protoss | 3,194 / 3,249 / 3,304 |

Both runs retain the human Terran row. Neither inspected lobby has a
separate team or difficulty row. This establishes the visible cycle on
these single-player maps; it does not cover split-screen, other maps
or internal AI policies.
The read-only observer captured three frames per run, no RAM, and
verified its inputs and emulator source remained unchanged. The command
stream SHA-256 hashes are
`dacf85bf61a2eb15f3bcc060e8007911be83bf66c4a2ec5901a6c51480b97717`
(Challenger) and
`8c523c722e77aaf0bac7de19d0c4979e9fefe5371656dce80f8c29f6a2291f71`
(Blood Bath).

### Paired Blood Bath launches

The owned USA ROM launched the same human Terran and named Zerg,
Terran and Protoss CPUs in both modes. In the Free for All run, the
mode change replaced part of the Melee tape's idle time, leaving
Start at the same observer step (3,695). Both lobbies were captured
immediately before Start. The three read-only opening RAM samples per
mode (steps 4,201, 4,210 and 4,215) have the same RNG states and draw
counts and independently recover seed `0xf3ae`. The decoded actor
records match: human and visible CPU row markers `3 / 0 / 2 / 1`, four
workers at each hall and all 60 map resource positions. The first
sampled game image also matches.

The native relation table at `0x800b92a4` has 12 rows of 12 bytes.
Across the paired runs all three samples agree: the only table changes
are the six directed CPU-to-CPU entries, from `2` in Melee to `0` in
Free for All. The human is native owner 3 and each active CPU is one
of owners 0–2. Human-to-CPU and CPU-to-human entries remain `0`, and
the four active self entries remain `1`.

The native initializer at `0x8003d97c` clears the table and conditionally
writes symmetric `2` entries for the computer player type. An enemy-unit
scan reads the current owner's row at `0x80060fb8`; it skips nonzero
entries and calls the other player's unit search at `0x80060fd0` only
for zero. Together the captured table and that reader establish that
the Melee CPUs open allied to one another, while Free for All admits
them as opponents. The pairing does not measure a later firefight,
different Random selections, victory conditions or other maps.

| Mode | Controller plan SHA-256 | Observer manifest SHA-256 |
| --- | --- | --- |
| Melee | `d7923a4a7abced9026ddc95b5b573ceb3ad63c838baebb391d437b3a9fb89840` | `7e245d3378b968b3144b6205d1353b4059792ab678f66db4270210b5e6569594` |
| Free for All | `b759ddf7e897e385ab756ee066e62be320cb9557d5a29317530c3bc75bbd0903` | `4538e843914374adc280be510da10d8368bb045ba5553e1cc9aaf3295ce95260` |

## Shared GBA mode preview

The Blood Bath preview exposes the Game Type choice and applies the
native CPU relation in the [shared hostility path](../../starcraft/game.c).
The campaign and Outpost rules are unchanged. Existing dense, sparse
and resolved-start saves still load as Free for All. A distinct `0x534e`
context marks Melee, so older ROMs refuse it rather than changing the
saved CPU alliances. Free for All keeps the `0x534d` resolved-start word.

All four preview HOST profiles pass the 234 named opening cases and
the extra sparse, Random, mode, AI target and save cases. All four ARM
previews pass their memory and ROM guards. Physical-button tests cover
one, two and three CPUs and the three human races across the four
products. The emulator reloads the actual save in a fresh process,
then matches the 60-update continuation without modifying its SRAM.

On the Terran ARM preview, both game types pass physical named and
Random launches, fresh load, continuation, and pause restart. Both
human and CPU races and their base positions follow the existing seeded
sequence. Read-only ARM register traces on the loaded games show CPU 1
treating CPU 3 as hostile only in Free for All, while both modes treat
the human as hostile. The previous normal Terran rejects the Melee save
at HOME without changing its SRAM; both old and new cartridges read the
older and current Free for All saves. These ARM tests do not establish allied vision. The native Random
pair below checks the observed race and start sequence in Free for All. The separate
[Blood Bath end-rule audit](2026-11-13-n64-blood-bath-end-rules.md)
decodes the live native triggers and traces the defeat cleanup.


### Paired Random launches

A short controller tape matched the earlier joint Random Melee opening
with Free for All on the same owned USA ROM. Both lobby frames showed
Human Random and CPU Random / Random / Protoss. The input tapes pressed
Start at the same observer step (3,695) and held neutral afterward.
The bounded cache-aware observer verified both lobbies and three RAM
samples per mode. Each sample independently reverses the native RNG
state and draw count to launch seed `0xf3ae`.

| Sample step | Draw count | RNG state in both modes |
| --- | ---: | --- |
| 4,201 | 572 | `0xd3699d52` |
| 4,210 | 575 | `0x3c6eb959` |
| 4,215 | 581 | `0xa63467d3` |

Every sample gives owner IDs 2 / 0 / 1 / 3 in human and visible CPU
row order. The resolved races are Zerg / Terran / Zerg / Protoss and
the start markers are 2 / 0 / 1 / 3. Both modes have the same four
workers per hall and the same 60 resource actors by type and position.
The game-type byte is 3 for Melee and 1 for Free for All, and the six
directed CPU relations are 2 and 0 respectively.

The existing selector predicts this opening: the three ascending
Random rows consume draws before placement and leave state
`0x1d976e05`; the named Protoss row consumes no race draw.
Placement then assigns the available marker IDs in reverse row order.
The [GBA launch dispatch](../../starcraft/game.c) already uses the
same opt-in Blood Bath selector for both modes and stores the mode
separately from the resolved races and positions. No selector change
is needed for this vector. This dense pair and the sparse pair below
use one recovered seed; they do not establish identical clocks
between the machines.

| Mode | Controller plan SHA-256 | Observer manifest SHA-256 |
| --- | --- | --- |
| Melee | `90884125d92b3353f5de35ac95037b4134b9617549d01a5f536615727b0dea4e` | `fae8647d253a2a7c353b8566b7d495b1dcfb19a0402112415c41e79335151615` |
| Free for All | `02da3f120d0918f4de0ec79320479e2f238528f425f61b347f63664dda261c85` | `5120e53b82fc15624119f10848e33b7313ba459da8dd7f69febc3aa0035a7792` |


The sparse repeat used a named Terran human and only the middle CPU
row set to Random. The Free for All lobby displayed four starting
slots for the map, even with the first and third CPU rows closed.
It exposed the game type, map and race rows, with no separate
difficulty or team row visible. Start launched with exactly one CPU.
This checks that the middle seat can be the sole CPU; it does not
establish whether the lobby permits zero CPUs.

The matching Melee and Free for All tapes both pressed Start at
step 3,695. All three RAM samples at steps 4,201, 4,210 and 4,215
agreed on draw count 370 and RNG state `0x6c523784`, independently
recovering `0xf3ae`. The active owners were human 0 and CPU 1.
The hall and four workers identify Terran at marker 0 and Zerg
at marker 1, respectively, with the same 60 resource placements.
The mode bytes were 3 and 1. A one-CPU launch has no CPU-to-CPU
relation to compare.

The same GBA selector, with occupied rows 0 and 2 and marker mask
15, resolves the middle Random CPU to Zerg, leaving state
`0xa39d7707` before placing the two halls at markers 0 and 1.
The named human and both closed rows consume no race draw in this
vector. Other sparse lineups and launch seeds remain unobserved.

| Sparse mode | Controller plan SHA-256 | Observer manifest SHA-256 |
| --- | --- | --- |
| Melee | `04cdf796673202a8b3a583351f7a1c6bb5ec65345b58792ce5e52832cc31951a` | `3eaa2d7e49d36bc2dfc3e7d43ab4bf2d0fb5a098bea3c977f5418d9426988dcb` |
| Free for All | `ebfa1ce61f80e98e9018cbce9552a14f4e91db3da0375fc0298584c90220088c` | `d0973068f916f9c7a86ab733a67af2f2617709d3c5dbd254dc071d902cbe35c8` |
