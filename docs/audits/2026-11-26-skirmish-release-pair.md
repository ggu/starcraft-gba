# Custom Game release comparison

The comparison uses normal builds of merge `d6fd232e` (#913) and
merge `51c7f18f` (#942). All four #942 builds reproduced the Mac
package hashes byte for byte. Their paired ELF symbols were used
with the packaged ROMs for physical mGBA controller input and
read-only state inspection.

## Lobby and fresh HOME continuation

Each matrix entry launches from HOME, plays 120 neutral updates,
uses START and B to save, then compares 60 uninterrupted updates
against a fresh HOME L continuation. The selected layouts cover
one Zerg CPU, Terran and Protoss with an intervening Closed seat,
and Zerg, Terran and Random. Each cartridge plays all three named
human races. Blood Bath is tested with and without allied CPUs.

| Game type | Maps | Passed before/after pairs | Pairs with identical world samples and SRAM |
| --- | --- | ---: | ---: |
| Free for All | Outpost and Blood Bath | 72 / 72 | 72 |
| Melee | Blood Bath | 36 / 36 | 33 |
| Total | Both | 108 / 108 | 105 |

Every run resolves the requested occupied rows, starts on distinct
markers and re-exports its own SRAM byte for byte after HOME loads.
The three differing pairs all use the Z10 cartridge. The chosen
human and occupied CPU races agree. Their start orders differ as
shown below. Blood Bath derives its start draw from elapsed guest
milliseconds at confirmation. Each differing save was also loaded
on the other revision. All six directions restored the writing
revision’s start markers and matched 60 further updates, with
byte-identical SRAM re-export.

| Human | CPU rows | #913 start order | #942 start order |
| --- | --- | ---: | ---: |
| Terran | Terran / Closed / Protoss | 19 | 18 |
| Zerg | Terran / Closed / Protoss | 57 | 56 |
| Zerg | Zerg / Terran / Random resolved Protoss | 114 | 108 |

The private, sorted lobby receipt has SHA-256
`95290238d0151a885ddee72968f124cabb49f5d5adb8d654296e327f421a66fa`.
This matrix covers representative seat arrangements for each count.
The November 21 record covers the earlier exhaustive named/Closed
arrangements separately.

## Paired timing from the same Z10 battle save

Both builds loaded the retained Blood Bath Melee save from world
frame 9,044 against three Zerg CPUs. Replay then used the same
512 recorded controller updates. All seven gameplay checkpoints
and the endpoint fingerprint matched. Both bitmap pages, the
palette, the actor arrays, the camera and the logical work page
matched at all 513 samples. At boundaries 27, 292 and 371,
the displayed page differed and matched again on the next sample.

| Guest timing | #913 | #942 |
| --- | ---: | ---: |
| Mean cycles per advancing update | 920,546 | 901,413 |
| Emulated loop updates/s | 18.00 | 18.35 |
| Interval p95 cycles | 1,265,860 | 1,256,926 |
| Interval p99 cycles | 1,560,442 | 1,560,095 |
| Maximum interval cycles | 1,937,804 | 1,920,982 |
| Intervals over two VBlanks | 509 / 512 | 509 / 512 |

A cold replay of the entire #913 input tape on #942 hit a different
start draw at controller update 40 (147 instead of 156), so the
whole-match validator stopped at the lobby. The retained save
provides the equal-geometry comparison above. A separate 4,096-input
continuation on #942 matches all 58 retained gameplay checks.
The earlier #913
full win is pinned by the same ROM hash as the rebuilt baseline,
with victory at world frame 61,183 and 649 fresh continuation
checks. The release pair includes the intervening Egg-counting and
fighter-selection fix in #936. The
[direct #942 performance trial](2026-11-26-z10-triple-zerg-pacing.md)
isolates the defense change.

The private timing receipt has SHA-256
`c7966452ce1f1f75ce16465c2572ec6749f560275dbe602babd210bb54c3a859`.
Hardware tests and the 30 fps goal remain open.


## Full matches on the new package

The public full-match matrix authored two new win tapes against
the current release. Physical ARM replay confirmed the
Terran Outpost win against one Zerg CPU at world frame 32,082.
All 251 whole-match checkpoints and 116 fresh-pause continuation
checkpoints passed. Both runs wrote the same victory SRAM, and
fresh HOME loaded its victory screen and state. The uninterrupted
match averaged 21.21 emulated calls/s and the tail after the pause
averaged 19.79.

The Protoss Blood Bath Melee win against Terran and Zerg reached
world frame 23,371 with physical inputs. The uninterrupted run
passed 227 checkpoints and wrote its pause and victory journals.
The first fresh-pause continuation hit the headless backend’s
60-second command timeout. The retry started from the retained
pause journal and limited each transport command to 32 updates.
All 64 continuation checkpoints and the final RESULT fingerprint
matched. The uninterrupted and resumed runs wrote identical victory SRAM, and a
fresh HOME reload showed the same result and re-exported the
journal byte for byte. The retry measured 15.78 emulated calls/s
over its 8,033 calls.
