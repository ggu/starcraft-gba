# Blood Bath through published defeat

Two private ares runs of the owned USA ROM opened Terran versus Zerg,
Protoss and Terran in Blood Bath Free for All. Both restored the same
ordinary lobby capture, pressed Start, and sent neutral gameplay
input. The capture shim observed the game through its emulator save
hotkey. Neither run changed guest memory or ROM bytes. The launch
assigned the human to native slot 1 in the first run and slot 2 in
the second, so player identity was taken from the runtime player kind.

The standard GBA Terran build was driven through a separate ordinary
mGBA match with the same map and named race lineup. The replay had a
blank cartridge, selected all three CPUs in the menu, and continued
from a normal pause save in a fresh process. Its fixed start layout
differs from the sampled N64 assignments. The comparison covers the
defeat condition and continued play after losses. Timing, AI strength
and exact trajectories are separate questions.

## N64 final building

In the second run, human slot 2 still owned the last raw-106 Command
Center at step 9,141. It had 24 whole HP in the snapshot, the building
count was one, and the pending and published result bytes were zero.
The next retained frame at step 9,160 has a zero human building count,
pending and published defeat 2, and human kind 10. The three CPU
kinds remain 1 and their building counts remain 9, 13 and 10. The
dispatcher countdown goes from 6 to 18 across those samples. The
first run independently observes slot 1's count falling from one at
9,129 to zero at 9,296, with pending and published defeat 2. It
retains live buildings for all three CPUs.

The building count is the broad native census at
`0x8010ac80 + 4 * owner`. The opening kinds come from
`0x800aff54 + 36 * owner`, which the eligibility routine reads at
`0x8008c8fc`. The pending and published bytes are at
`0x8010dbe0 + owner` and `0x800b936c + owner`, respectively.
See the [decoded result handler](2026-11-13-n64-blood-bath-end-rules.md)
for the transfer branch and the
[cadence audit](2026-11-13-n64-blood-bath-result-cadence.md)
for scan ordering. The late run has no surviving human mobile actors
at the two retained cuts. The record witnesses the count, kind and
result changes across 19 native updates. A separate run below resolves
the one-update publication boundary with a living human survivor.

### Ordinary human survivor transfer

An ares run restored the authenticated ordinary Blood Bath opening and
sent controller inputs only. During the match, the player used the
ordinary Move command to withdraw SCV slot 67. The first attempt left
that worker at (479,93), where it died before the base. After restoring
the run at step 6,781, the player moved the same worker to (516,51).
It stayed there with 60 HP. The host used save and per-frame hotkeys to
inspect the result boundary without modifying guest state.

| Native update | Command Center HP | Human building census | Countdown | Human kind, pending and published result | SCV slot 67 |
| --- | ---: | ---: | ---: | --- | --- |
| 9,035 | 38 | 1 | 19 | 2, 0, 0 | Owner 2, 60 HP at (516,51) |
| 9,044 | 0 | 1 | 9 | 2, 0, 0 | Owner 2, 60 HP at (516,51) |
| 9,054 | 0 | 1 | 0 | 2, 0, 0 | Owner 2, 60 HP at (516,51) |
| 9,055 | 0 | 0 | 29 | 10, 2, 2 | Owner 11, 60 HP at (516,51) |

The Command Center is already in its Death order when its HP reads
zero at 9,044. It is still present as a dying actor at 9,055. The census,
result, and survivor ownership change in the same sampled native update.
The worker retains its type, idle order, HP, position and pool slot.
The three CPU owners retain kind 1 and keep their counts of 9, 13 and 10
buildings. An independent neutral-input continuation from the earlier
checkpoint also preserves the transferred worker at step 9,067.

The ordinary GBA opt-in transfer below preserves the same survivor
fields. Its live building rows and immediate result check differ from
the native countdown and death census. CPU retirement with a surviving
actor and multi-owner final-building losses still need native cuts.

| Private survivor-transfer receipt | SHA-256 |
| --- | --- |
| Native step 9,044 normalized RDRAM | `64ea1639bf8e5facd1a54a2544bf6332688517e9bceeb2e0dd41346a9d78f218` |
| Native step 9,054 normalized RDRAM | `1686c449cd0a608ba1de1e6eb89c1bed357c113496aa1ab9ade108549045a8e5` |
| Native step 9,055 normalized RDRAM | `a2289010c308c25be34e7c42b3f92b829e77dec5b15be40220d47845e02862bc` |
| Ordinary controller and capture journal | `1e04f770f4a591a06a5a9c32973d227ecd489c1f0fe696fce1f4a1164b6059ea` |
| Per-frame capture journal | `a746478d382107c3fa9fbd85931808a48953dbb644a52d41f571ffbdbe65f3be` |
| Authenticated emulator and native input pins | `7eb1ae904d80b49b124f9252502af69c79c7267d0b42739bd2a248b8d4bfed97` |

## Normal GBA result and fresh load

The current Terran ROM `526a3329…`, built with the renderer in #657,
started with all four bases and saved through START then B at frame 601.
Loading the 32 KiB journal from HOME with L reproduced the world and
all 93 sampled checkpoints through the same loss at frame 22,817.
The human still had a building when the sampled Zerg and Terran CPU
building counts previously reached zero. No outcome published at
those points. At the human loss, no human structures remained and the
sole surviving Protoss CPU had 23 buildings and 44 units. This normal
ROM tracks the remaining enemy structures in aggregate.

| Observation | Last sampled state before the threshold | First sampled state after the threshold |
| --- | --- | --- |
| Zerg CPU, Terran GBA | Frame 15,721, one building | Frame 15,961, zero buildings and play continues |
| Terran CPU, Terran GBA | Frame 21,481, one building | Frame 21,721, zero buildings and play continues |
| Terran human, Terran GBA | Frame 22,681, one building | Frame 22,817, zero buildings and defeat |

The two eliminated GBA CPUs already had zero mobile units in their
last nonzero-building samples. A human win, natural CPU retirement
with a surviving mobile actor on N64, simultaneous last-building
losses, and the behavior of in-flight damage remain separate cases.
Keep the neutral-transfer preview in [#566](https://github.com/ggu/starcraft-gba/pull/566)
under its explicit build option while those cases are open.

## Current elimination preview

The opt-in branch from #566 also links against the current release's
four normal build vectors, including the Z10 Carrier format. All four
ROM and RAM guards pass. The Showcase linker ends at `0x03004fe8`,
with 24 bytes up to its IWRAM link limit. The four prepared
elimination host fixtures pass under ASan and UBSan.

The new exact Terran preview `53bdb479…` started another ordinary
three-CPU match with a seeded start order. All 61 sampled checkpoints
match a fresh load of its frame 601 pause journal through defeat. A
single-update follow-up places the Terran CPU retirement at frame
14,196. Its last building was destroyed, its elimination bit was set,
and both other CPUs and the human continued to play. A new pause
journal written at 14,196 loads in a fresh process with the retired
rank still recorded and reaches the identical human loss at 15,112.
At the defeat update all four living human SCVs retain their unit slot,
type and 60 HP, and their owner changes to neutral raw owner 11.
These checks use ordinary controller inputs and the ROM's save path.
The native and preview start layouts differ, so runtime duration and
defeat frame comparisons do not establish timing parity.

| Private receipt | SHA-256 |
| --- | --- |
| First native run | `9e613ccff5820439c81cb6639819a662441b2d1c0a885df531c2ed9183a8cff1` |
| Retained native run | `d145d825ac306b5e5a75f0bf0e3856ecf60d7d26c520bde4cf4af1977ac174c6` |
| Native last living Command Center state | `3f968d8404c5a52f2137aab84a68248eea6a485adcb5c6ecd9b5fc55dc5a5e0c` |
| Native terminal state | `79f3e0707e831d59c967dd3f95dd647a0020e0eca1e603b3a6294e9889f4fe21` |
| Normal Terran blank and fresh continuation | `ab71d5d579d8e06229e22422cebbb9ab58aef1c660ccbe6a19d50ac872fed109` |
| Opt-in Terran blank and fresh continuation | `2adcfc74e691f844923a7b709bd4857002d7ce6e3a45925d4fa0c210e81abeea` |
| Opt-in retirement, new pause journal and defeat | `a7fb208603dce95e261f4e2f8439f784366b4ec6b35ea15b7c16303f6f75bc62` |
