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
before ordinary Start input. Each opening had two full Command Centers
and four SCVs per owner; the linked actor list also matched all 60
source resource positions. The camera origin identifies the human base.

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
read-only samples again matched all 60 map resources. The one- and three-CPU
runs do not establish the rules for two CPUs, other human races, or Random.
Preserve the resolved placement explicitly before changing the startup rule:
already-written GBA saves must keep their old AI fallback locations.
The Melee setup screens showed no difficulty selector; other modes and
any internal difficulty policy are unverified.
