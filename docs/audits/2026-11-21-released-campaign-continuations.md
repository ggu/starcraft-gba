# Campaign continuations on the parked depot flood release

The normal race cartridges built at runtime `0e0250d0` match the
ROM hashes in the local three-cartridge playtest package. The public
source at `1971ab40` contains that runtime. The input observer drives
ordinary buttons in mGBA and reads state at known gameplay and
interface boundaries.

| Cartridge | Release ROM SHA-256 |
| --- | --- |
| Terran | `2e48c6a54ec91368ea22e822120004c8ce457abed2f414edc15b7949b873f27e` |
| Zerg | `ef37fe4a578c81bc4acd711bf200e75b183404bf514d592d8f93f15158ff3e97` |
| Protoss | `18150d57c610ed82ab28881cba79f557fcfe915e8d19accd39d41710fb7d89ec` |

## Terran

Each qualified route starts at HOME, writes a pause save, resumes it
from fresh HOME, wins and loads the result from a third HOME process.
At all listed checks, the observed gameplay state and frames match the
preceding normal Terran ROM with SHA-256
`dcbaf755cc7d66dbf4304b212cd81ad1ce237945d030a2bbc050ca81c62856a4`.
The pause, victory and re-exported journals match byte for byte. The
driver compares named gameplay fields. Paired framebuffer checks for
the opening remain in the [routing audit](2026-11-21-parked-depot-flood-current.md#ordinary-arm-comparisons).

| Mission | Pause frame | Victory frame | Matched checkpoints | Released replay SHA-256 |
| --- | ---: | ---: | ---: | --- |
| T1 | 6,214 | 10,120 | 12 | `479ddee8c5d8e6d30f498f6fd24f00597c1978bcfbfba294a9f6b53afc4c7a25` |
| T2 | 18,204 | 29,366 | 31 | `39abf366a3da7dddaa756af5e9350ef0f3d0e747caf718fae55c9ca60440fef7` |
| T3 | 26,993 | 54,000 | 56 | `71ad27a7b52b5950b59044adfe85143ff9095f157d0b97c1955034197c8baa4f` |
| T4 | 4,968 | 8,911 | 10 | `e7d563d6d33a4dcfbd0753878a7c302756aa1d507e94004b78a7f7804a14f878` |
| T5 | 72,076 | 126,331 | 148 | `821f9d6d079fea5ea36621969b30043fabdf689f79851ebc3911f350ff07a99b` |
| T6 | 19,466 | 23,536 | 42 | `bde6df3567e9130369d688bd622be3cc3fc1add5ba47e4804c67a1a7d64b525e` |
| T7 | 2,289 | 3,931 | 12 | `47ee719c02fc1a71bc5be07b19542d9b4d9a46a4484491b340af23fb9ccc5436` |
| T8 | 2,568 | 139,216 | 268 | `4521b1bc3ace85d2106a7403393ea73e4a7e2c7986f966276d32f249d9aeb0d3` |
| T9 | 35,363 | 47,266 | 51 | `0e1cef3c738abcd483fa7db65d0a369cb303f5a3df9b37f04fcbd29872dab339` |
| T10 | 2,276 | 3,361 | 6 | `16ef763afb72a9d73de0f889d05bd54c5d622641d5eb7e78d49a3361d74b7f00` |

The T10 comparison with the preceding normal cartridge extends the
[earlier pinned T10 control](2026-11-21-parked-depot-flood-current.md#ordinary-arm-comparisons).
The complete ten-mission comparison digest is
`8b71fa6e1757570ee61e3b624625a2bd744bf12ffcc9aee63262458604c99a62`.
T8 averages about 19.74 main-loop updates per emulated second over
the whole route, including presentation waits and UI.

## Z1, Z2, Z4 and P5

Z1 uses the complete HOME route and pauses at frame 50,413. Its new
HOME process matches four continuation checks and reaches victory at
frame 50,669. The route matches 54 HOME checks. A third HOME
instance holds the terminal world fingerprint through eight neutral
updates and exports the identical 32 KiB result journal.

Z2 pauses at frame 3,240, resumes at the same saved state and
matches all nine retained observations. The Chrysalis survives
through victory at frame 8,164. A third HOME process restores the
terminal objectives, holds the loaded fingerprint for eight
neutral updates and re-exports the identical journal. The Z2
driver records the terminal objectives before export and the
world fingerprint after load.

Z4 starts at HOME and pauses at frame 9,900. A fresh HOME process
restores the saved world and selection. The run matches 42 named
observations and wins at frame 34,984. A third HOME load matches
the terminal world fingerprint, holds it for eight neutral updates
and re-exports the identical victory journal.

P5 starts at HOME and writes its pause at frame 2,700 with the
Shuttle in flight. Ordinary HOME load restores that state. All
seven retained checkpoints match and Tassadar survives through
victory at frame 5,057. The driver verifies the completion bit
and a fresh HOME load re-exports the identical victory journal.

| Check | SHA-256 |
| --- | --- |
| Z1 HOME route | `8b3ca1b0fbdf3e0c50839e806bd7fa222bee377957f38bf3c201aba738c29b16` |
| Z1 fresh continuation | `d1a0bc14f89f650a431fd1237842b58c0992ddc125a5bcab5428fa074aa1ed61` |
| Z1 result load | `27476401075cf31f86a34468db065a509f83f1610ddcd9487bc1390aead9a846` |
| Z2 route and fresh continuation | `253094a38c6134fa0a05db1880e07530431d0903fbf1d8309c1ef5863c13e722` |
| Z2 result load | `1bb2146e2f6dfee3faeda93200df1333f19f82b84a3e1b55e1415b90ad077630` |
| Z4 route and fresh continuation | `bbe59ee1f8393a068ed094e7c0e615188bbdedb42d998b4a4b28843c99af5ac6` |
| Z4 result load | `58d05faf4591769eee37f241a993264dfd6a9e4cb49b707cbc737e5b6c5e4819` |
| P5 route and result load | `d7ba5d0e0e05786469faddda8e46a318018dd1c74a422d47ac67ca06bebccbbd` |

## Zerg and Protoss finales

Full Circle on the released Zerg ROM starts at HOME, writes the frame
2,526 pause journal and wins at frame 50,193 through a fresh HOME load.
The 20 retained continuation checks match with no gameplay divergence.
The victory survives eight neutral updates after a third HOME load
and its re-export matches the 32 KiB result journal. The pause and
victory bytes match the [earlier pinned Z10](2026-11-21-z10-current-full-circle.md).

The released Protoss ROM completes its finale from HOME, including the
full route to the frame 36,761 pause, the fresh continuation and the
victory at frame 46,129. All 20 named checkpoints match the earlier
full HOME result. Six follow the new process's load. The pause and
victory journals match the earlier cartridge byte for byte. A third
HOME process holds and re-exports the victory.

| Check | SHA-256 |
| --- | --- |
| Full Circle continuation | `70be27f78215b43fab24c9123c0ee2bb443e8d68f8aa52b4587b5c1e46cb81b1` |
| Full Circle victory journal readback | `2e6e1b828b06c36989be171b781fdabb3c100b0fea221e877384d0fd82fc4cee` |
| Full Protoss finale with save and result readback | `0086ed2cdeb62c54e2a41d32456084d116a5dfb942ba6d84ab51bc496924ed70` |

The Zerg finale continues at about 11.81 main-loop updates per emulated
second. The full Protoss run averages about 10.42. Both include
presentation waits and UI. Hardware performance and broader native
campaign parity remain open.
