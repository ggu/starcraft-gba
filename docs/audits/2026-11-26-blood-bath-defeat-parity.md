# Passive Blood Bath defeat on N64 and GBA

A passive Terran human lost a Blood Bath Free for All on both
cartridges against Zerg, Terran and Protoss computers. The two N64
runs are the retained `s8-k0` and `s8-k0b` captures from the
[scenario spikes](2026-11-26-parity-scenario-spikes.md#s8-seeds).
This comparison uses their watch rows and sampled live unit rows. The
paired captures have identical ticks, sparse raw rows and result
watches. The standard Terran GBA ROM ran with physical buttons and
read-only observation. Both sides remained passive after launch.

The N64 launch used seed 84,421. GBA start order 75 put the same race
and human/computer seat at every start anchor after coordinate scaling.

| Seat | N64 owner and anchor | GBA owner and anchor |
|---|---|---|
| Terran human | 1, (288, 1808) | 0, (144, 904) |
| Zerg computer | 3, (1760, 240) | 1, (880, 120) |
| Terran computer | 2, (128, 240) | 3, (64, 120) |
| Protoss computer | 0, (1760, 1808) | 4, (880, 904) |

## Terminal structure and surviving workers

The N64 human opened with four SCVs and one Command Center. By tick
12,724, only the Command Center retained a live sprite row. Its
candidate HP field read approximately 105 at tick 12,924 and zero at
ticks 12,949 and 12,974. Both result watch words first became
`0x20000` at frontend step 49,497, during tick 12,960. At tick 12,977
the pause word became one. That tick's live-row hash matches the
sampled rows at tick 12,999, where owner 1 has no sprite rows. All three
computer owners still have rows. The health reads describe this
specific row during destruction and leave the layout's candidate
classification unchanged.

On GBA the last active sample at frame 24,065 still had four human
SCVs and one human building. The result at frame 24,222 had lost set,
won clear, zero human buildings and four SCVs at 60 HP each. The
Protoss computer was the remaining opponent, with 44 units and 19
buildings. The [shared cartridge outcome check](../../starcraft/race_adapter.h)
counts human buildings for defeat. The field result panel displayed
“Your forces were eliminated” alongside Units 4 and Buildings 0.

Both results follow the loss of the human's final structure. The
[earlier ordinary native survivor capture](2026-11-21-blood-bath-live-results.md#ordinary-human-survivor-transfer)
established that a surviving SCV changes to neutral on the defeat
update. All four GBA SCVs retain their human owner on the standard
cartridge here. The opt-in GBA transfer already has its own coverage
in that earlier audit. This run adds a comparison with the same start
anchors and shows the standard cartridge's current result panel,
surviving-actor counts and result controls. The two computer matches
followed different battle trajectories.

## Result controls and readback

In separate fresh GBA processes, HOME L reopened the defeat at frame
24,222 with the recorded live unit and building counts and re-exported
byte-identical 32 KiB SRAM. A from the restored result restarted the
same four-seat Blood Bath lobby in play at frame zero. B from another
copy returned to HOME with the result flags cleared.

### Native A opens the defeat score overview

A fresh ares process replayed the same passive Blood Bath Free for All
from the pinned menu state. Through frontend step 50,026, all 24 full RAM
checkpoints, tick rows and sparse actor rows match the two retained S8
captures byte for byte. Both result words again became `0x20000` at
step 49,497 during tick 12,960. The continuation used only ordinary
controller buttons.

| Frontend step | Native ares observation |
| --- | --- |
| 50,146 | The field shows “You failed to achieve victory!” with an OK button. |
| 50,147–50,156 | The tape holds A. |
| 50,216 | A neutral capture shows the Defeat / Overview score page during its reveal. |
| 50,466 | The settled Overview shows four player rows and Units, Structures, Resources and Score columns. |

The settled human row displays 200 Units, 400 Structures, 2,010
Resources and 2,610 Score. This is a screen readout from one passive
match. The Overview displays elapsed time 12:23. The last parity
tap on the defeated field recorded 810 in its seconds field. These
readouts leave the score clock's formula open.

The first A on the standard GBA defeat retries Blood Bath. The first A
on this native defeat opens Overview. This capture covers only entry
to Overview. Its exit and the other score tabs remain open.

To reproduce, use the original S8 `s8-k0` tape and replace its final
`quit` with the following commands. Keep the original menu state,
JIT mode, frontend binary and owned ROM. Each `shot` captures the
last step of the numeric command that follows it.

```text
tap off
shot defeat-before-a
120 0 0 0
10 1 0 0
shot after-a-early
60 0 0 0
shot after-a-settled
250 0 0 0
quit
```

The private qualification archive
`.local/qualification-20261126/native-blood-bath-defeat-overview.tar.gz`
keeps the amended tape, provenance, screenshot triplet and checked
input and checkpoint logs. Its SHA-256 is
`e2414c0c9a3928aef3417bf9ecab7878e2f62d490da399face0ada899e92e862`.

| Native capture | Raw frame SHA-256 |
| --- | --- |
| Field defeat before A | `804eadba22ae8e709d1c7ee2e820894bc7294929e5db374ba8c4d99808f53329` |
| Overview during its reveal | `2db2cd0d6643dbe81040d5f707e7589660e82844af858937797aa7d00804b4b0` |
| Settled Overview | `2db05434954647169031b859db669b6431e915cedc3c2a46edb1ad90412bddc1` |

## Evidence

The private qualification archive
`.local/qualification-20261126/native-blood-bath-defeat-parity.tar.gz`
contains the launch and replay commands, decoded result records,
GBA result screenshot and defeat SRAM. It uses the prior native
captures in the private fixture home. The result measurement is a
report; the existing golden parity gates are unchanged.

| Artifact | SHA-256 |
|---|---|
| N64 watch rows, both captures | `c7b4e11ec5240d392286898cde6b62118af998c2d7aa49af06b04b4fcf8f40db` |
| N64 sparse raw rows, both captures | `528c02322f9f7dc54b3c0cced07dc7f5a0024b461b58245483b9902806876d55` |
| Standard Terran GBA ROM | `17f3b4ce1d02e4619cb82828d283bdd523df9e2fe8afe9d12613bfaa41ea86e7` |
| Matching GBA ELF | `463d0e2819b83266b435e831340e9961257af987a5bec3f5aece939d2d67ec7a` |
| GBA defeat and fresh re-export | `9682d6364895e1b8d18097d34b82c5523ce06ee4ead14f6c6ae6109152424325` |
| Private qualification archive | `173be2c27b9ff302021c72d7ac6cdd5d38fd6bd0ba605def3faf07c59413aad0` |

## GBA Custom Game defeat overview

In the updated shared GBA runtime, the first A press on a Custom Game
defeat opens an overview. The player rows report living units, surviving
buildings, and mineral and gas banks at defeat. The panel marks lifetime
scores as unavailable because the runtime and its saves do not record the
required match totals. A fresh A press retries the same matchup. B from
either panel returns to the title. Custom Game wins and campaign results
keep their existing controls.

The standard Terran build restored the earlier Blood Bath defeat SRAM at
frame 24222. Holding A kept the overview open and left the SRAM byte
identical. The next A press retried at frame 0 with the same four seats
and start order. B returned to the title from either panel, and loading
the save again opened the original defeat panel. An earlier T1 victory
save advanced to T2 on the first A press. The private qualification archive
`.local/qualification-20261126/gba-custom-defeat-overview.tar.gz`
contains the recheck script, decoded readback and 240×160 screenshots.
