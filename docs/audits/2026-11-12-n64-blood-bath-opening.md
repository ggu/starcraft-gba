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
The Melee setup screens showed no difficulty selector; other modes and
any internal difficulty policy are unverified.

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

The observed Human Random launch independently checks the sequence:
seed `0xf3ae` resolves the human to Zerg, leaves the RNG at
`0xa39d7707` before placement, then gives human/CPU marker IDs
`[2, 3, 0, 1]`. Two further owned-ROM runs showed the same reset seed
for CPU Random. Each case verified its lobby and three read-only
opening actor samples, matching all 60 resource positions and four
workers per hall.

| CPU rows (Terran human) | Resolved CPU races | Human and CPU marker IDs by row | State before placement |
|---|---|---|---|
| Closed / Random / Closed | – / Zerg / – | 0 / – / 1 / – | `0xa39d7707` |
| Random / Random / Protoss | Zerg / Terran / Protoss | 1 / 2 / 3 / 0 | `0xb953c674` |

The native manifests are `3eaa2d7e49d36bc2dfc3e7d43ab4bf2d0fb5a098bea3c977f5418d9426988dcb`
(sparse) and `bf3bafeca0d1835d7bb2ffaad160aa7c62047adb05faf6f115e9cc02f245ad75`
(dense), SHA-256. The bounded pure fixture uses these captures as
expected vectors and adds malformed and unobserved gap controls.

The opt-in GBA Blood Bath Melee preview now uses one elapsed-time seed
for CPU Random and starts. It persists the same resolved races and v2
start-order fields; a load or restart draws nothing. Default builds,
Outpost and the named-only GBA human selector keep their existing
behavior. Other native modes remain unqualified.
