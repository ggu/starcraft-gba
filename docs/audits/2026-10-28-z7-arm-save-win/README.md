# Z7 CONTACT mission and save qualification (2026-10-28)

The separate `zerg-z7` candidate has a normal-input ARM win and a fresh-process
save/load continuation. The `zerg` alias still selects the six-mission Z6
product. This evidence supports an explicit candidate build, not a 30-fps or
physical-GBA claim.

## Source boundaries

The complete ARM prefix/resume pair froze runtime sources at `9aabfb0`, before
the default-OFF death-effect storage refactor merged in PR93. Both harnesses
use the selected CONTACT cache/steer-ROM/stock-ROM flags, full rendering,
source art and audio. They call update/audio/render directly with the recorded
ordinary input tape; they do not compile the standalone `common/main.c`.

The post-PR93 candidate uses game.c SHA-256
`71401fb431b2ea9f35f0bf8353f32eece043f9bbf4fcfe0907abdfea2d9fc902`.
Its full host replay matches all 365 checkpoint records and strict victory.
The carried Z1–Z6 controllers, Z7 profile/save/frame/Scourge fixtures, Z6
policy fixture and all 18 map/race/AI skirmishes pass with the selected CONTACT
flags. The post-refactor ARM text is 48 bytes smaller, so the earlier ARM pair
must not be described as a byte-identical current-source full replay.

## ARM mission and saves

The prefix starts through the normal menu and replays 173,418 input calls.
All 334 checkpoints match the expected mission fields and persistent-state
digests. At frame 172,247, ordinary START/release/B/release inputs save through
the pause menu. The 16,296-byte generation-36 payload exactly matches the
host save; the GBA's alternating-record SRAM checksum is valid.

A separate emulator process loads that newly written SRAM through HOME using
L, then replays the remaining inputs. All 31 remaining checkpoints match.
At frame 205,764 the strict result is a win with no loss or mission fault,
50 live player actors, no remaining source-force-1 buildings, and no pending
trigger cursors or clocks. This is the full 365-check mission path; no mission
state was injected at checkpoints.

Victory writes the normal sequence-2 autosave while preserving sequence 1:

| Record | Sequence | Payload bytes | Checksum | Payload SHA-256 |
| --- | --- | --- | --- | --- |
| Pause-menu save | 1 | 16,296 | `7458bda2` | `e08d6175652680cf97653844f913d6c15a68b7f020f78f3c0a400a6fb6de2377` |
| Victory autosave | 2 | 16,296 | `62fc79da` | `5dd7b2e50e2c5bc75f2c9a83415d93635d268d8a54bd671692b537557be76b8d` |

Both use tag `0x53435a54`. An independent post-PR93 host tail reads the final
save through the save API and produces the same victory payload. The parent
monitor initially expected unchanged SRAM after victory; that assertion was
corrected to validate both alternating records. No replay was restarted or
simulation state changed to satisfy it.

A second fresh-process ARM tail used the post-PR93 current runtime and the
actual earlier prefix save. It also passed all 31 remaining checkpoints and
the strict victory check, and wrote the exact same sequence-2 SRAM payload.
This validates current-runtime continuation and save compatibility across
the presentation refactor. It does not replace a current-source ARM replay
of the entire opening prefix.

## Timing and memory

The complete frozen prefix/resume path measured 936,431,779,430 update plus
audio/render cycles over 209,490 input calls. That is approximately 3.75 input
updates/s, or 3.69 advancing simulation frames/s at the GBA clock. This is far
below 30 fps. Timer and log/checkpoint work outside those windows is excluded;
there is no physical-GBA measurement.

Untouched painted IWRAM stack margins were 9,692 bytes in the prefix and
7,648 bytes in the resume. Painting starts at `0x03005000`, ends below startup
SP with a 256-byte gap, and does not measure all possible future workloads.
The current standalone link keeps BSS at `0x0203d7e0` and IWRAM at
`0x03004d28`, leaving 2,080 and 728 bytes before the unchanged guard limits.
Its physical-input-only main loop is covered separately by the keypad edge
regression fixture; debugger-fed inputs are deliberately absent in this ROM.

## Reproduction and evidence

After preparing private inputs, the public entry points are:

```sh
./dev sc64 build --profile zerg-z7
./dev sc64 test --profile zerg-z7
./dev sc64 play --profile zerg-z7
```

The test target runs the selected CONTACT gameplay vector, including carried
missions, controller trace validation and the 18-case skirmish matrix.
Historical probes keep their older flags and identities. ARM replay and
fresh-process save verification are additional checks, not implied by the
host test target.

Private tapes, source freezes, compiler argv, ROMs, logs and saves remain in
ignored local qualification folders. The complete frozen ARM pair is indexed
by `.local/build/z7-selected-full-arm-parent-20261028/result.json`; the
post-refactor host bridge and save export are indexed by
`z7-pr93-bridge-20261028/bridge-summary.json` and
`z7-post-pr93-win-save-20261028/host-win-save-proof.json`. The current ARM
continuation is indexed by
`z7-pr93-arm-continuation-parent-20261028/result.json`. No proprietary game
inputs or output ROMs are part of this source audit.
