# Native Blood Bath Custom Game victory and first A

An ordinary pad replay of the owned USA N64 cartridge in ares won a
single-player Blood Bath Free for All as Zerg against one Terran CPU.
The two other seats were Closed. The player recovered from the first
attack, removed the enemy production and surviving SCVs, and destroyed
the final Command Center with three Zerglings. All gameplay inputs were
controller commands. Read-only field observations and emulator saves
let the run continue from exact checkpoints.

The [decoded Blood Bath end rules](2026-11-13-n64-blood-bath-end-rules.md)
identify the native building census and published result fields used
here. At native update 16,804 the CPU's registered building count fell
to zero and its published result byte became 2. The human retained
two buildings and published result byte 3 at update 16,835. The
frozen field read **YOU ARE VICTORIOUS!**.

The first A press began at frontend step 70,938. It opened
**Victory! / Overview** with the **Zerg / You** and
**Terran / Omega Squadron** rows beneath **Units**, **Structures**,
**Resources**, and **Score**. At step 71,278, D-pad Right opened
**Units**, with **Produced**, **Killed**, **Lost**, and **Score** for
the same players. The settled screens displayed elapsed time 15:45.

## Shared GBA result controls

On the previous GBA runtime, the **MISSION CLEAR** result showed the
human's time, unit count and building count. A immediately retried the
match. The updated shared Custom Game result opens **VICTORY OVERVIEW**
on the first A press. **COUNTS AND BANKS AT VICTORY** labels each seat's
living units, surviving buildings, and mineral and gas banks. The panel
marks lifetime scores as unavailable. A fresh A press retries the same
matchup. B returns to the title. The release uses the same save format
and initializes the overview after the first A, including after loading
a terminal journal.

In mGBA, the previous standard Terran build restored the ordinary
input-authored Outpost victory at frame 32,082. Its first A retried the
match. The updated build restored the same journal and opened the
overview at frame 32,082. It displayed the Terran player's 73 units,
19 buildings and 23 mineral / 3,764 gas banks, and the Zerg CPU's zero
units and buildings and 42 mineral / 17 gas banks. Holding A left the
world and journal bytes identical. A fresh A retried with the same
seat assignment. B returned to the title, and a fresh load showed the
original field result. The updated build also restored the earlier
[Blood Bath defeat](2026-11-26-blood-bath-defeat-parity.md) and the earned
Terran T1 victory journal. Defeat still opens its overview. T1 still
advances directly to T2.

## Reproduction and evidence

The private archive `.local/qualification-20261126/native-blood-bath-custom-victory.tar.gz`
holds the final terminal state, exact pad tape, source and binary pins,
command and watch logs, linked checkpoint receipts and screenshots. The
score tape restores its terminal state, sends 100 neutral frontend
steps, holds A for ten steps, releases for 330 steps, then tests Right.
The ROM SHA-256 is
`e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`.
The pinned ares binary SHA-256 is
`1a69a1fe6693f45d4635752afda2125def04392ec4dcac766f2c3c966556a81e`.

| Native cut or archive | SHA-256 |
| --- | --- |
| Victory modal raw frame | `9401653099b6b414794308fc3ea985333d35c4bcfa3e6e869721629c35a22589` |
| Settled Overview raw frame | `23829dc8bb271d1c9feab8201198b570d43866352c54db5482acd9839369a9e8` |
| Units raw frame after Right | `335d7cee39f8f40d82d418d1679832f1c18fc499b2370874e885b1aa85a14794` |
| Private native archive | `20b8f76be50312f4e90f4617e1173a59462644d04b79ec2020ab7de45f9cb226` |
| Private GBA before-and-after archive | `ac11ae01bab20bf077f183dc3f6d12758b7a2082a8623f7376b62685f38de6d2` |

The GBA archive is `.local/qualification-20261126/gba-custom-victory-overview.tar.gz`.
It contains the ordinary HOST input trace and standard journal,
the baseline and updated control receipts, replay scripts and
240×160 screenshots.
