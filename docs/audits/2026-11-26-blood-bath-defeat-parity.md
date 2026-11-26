# Passive Blood Bath defeat on N64 and GBA

A passive Terran human lost a Blood Bath Free for All on both
cartridges against Zerg, Terran and Protoss computers. The two N64
runs are the retained `s8-k0` and `s8-k0b` captures from the
[scenario spikes](2026-11-26-parity-scenario-spikes.md#same-lobby-same-start-step).
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
N64 workers were already gone in this run, so a native last-building
loss with surviving workers remains open. The two computer matches
also followed different battle trajectories despite their matched
start anchors.

## Result controls and readback

In separate fresh GBA processes, HOME L reopened the defeat at frame
24,222 with the recorded live unit and building counts and re-exported
byte-identical 32 KiB SRAM. A from the restored result restarted the
same four-seat Blood Bath lobby in play at frame zero. B from another
copy returned to HOME with the result flags cleared. The retained N64
tapes end on neutral input, leaving native post-result controls and
score tabs open for this map and mode.

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
