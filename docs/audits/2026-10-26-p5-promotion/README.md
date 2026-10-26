# Canonical Protoss P1–P5

Canonical `protoss` selects the existing P5 Shuttle product. It exposes P1–P5
through the normal campaign frontend and retains both skirmish maps, every
human/AI race and one to three independently selected opponents. Together
with Terran 1–10 and Zerg 1–4, canonical coverage becomes 19/30. Later chapters
remain staged; this is not complete campaign or native-engine equivalence.

## Products and inputs

The P5 ROM/ELF are `StarCraft-Protoss-P5-Shuttle-GBA.gba` and
`starcraft-protoss-p5-shuttle.elf`, title/code `SC P5 CARGO` / `SCPB`, save
generation 26 (`0x5343504a`). Emulator saves use
`.local/play/protoss-p5-shuttle`. The explicit `protoss-p5-shuttle` spelling
selects the same product.

Explicit `protoss-p4-storm` preserves its real P4 files, `SCP5`, generation 21
and `.local/play/protoss-p4-storm`. Explicit `protoss-p3` preserves its real
P3 files, `SCP3`, generation 7 and `.local/play/protoss`. No files are relabeled
as a different generation and no save conversion is provided.

P5 adds the existing Shuttle/runtime/query flags to the qualified P4 and
canonical render groups, including the batched minimap. The private sprite
selector is `--base-only --include-zp-campaign-actors --include-z6-actors
--prepack-unit-words`. Preparation and readiness checks require the P5 map
gate, scenario and real unit-word metadata/payload; the set marker alone is
insufficient. The canonical generated bank is byte-identical to the qualified
bank, SHA-256
`78f6936c484b1fc1754022767c153c235b3a8c8577425188429a20383a1b548e`.
No game data or generated asset header is committed.
P5 retains the existing full-storage pools of 240 units, 160 buildings and
144 mineral fields; the explicit P4 product keeps its 240/96/48 pools.
Compact P5 runtime/save records keep these larger pools within the same
memory and SRAM reserve guards.

## Normal-input qualification

The five isolated mGBA replays compile the current runtime with the full
proposed P5 flag vector and that bank. Drivers provide only ordinary key
transitions, update, audio and render calls; they do not grant resources or
write mission/actor state. Each consumes its complete tape and reaches its
source-derived result with no input fault or loss:

| Mission | Input updates | Inputs | Final frame | Full mean cycles | Equivalent updates/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| P1 | 9,303 | 162 | 9,252 | 631,514 | 26.57 |
| P2 | 32,220 | 21 | 32,208 | 630,893 | 26.59 |
| P3 | 47,862 | 3,192 | 47,238 | 889,009 | 18.87 |
| P4 | 104,850 | 1,990 | 104,346 | 1,040,098 | 16.13 |
| P5 | 5,561 | 221 | 5,496 | 983,242 | 17.06 |

P3 reaches sequence 5; P4 retains both heroes and its source-derived escape result,
with 87 creates and fired bits 8,184; P5 wins with valid runtime and no fault.
All five also pass host input replays. Compiler dependency files prove the
actual current `game.c` and minimap header, rather than assuming an intended
include path.

These are full-route emulator cycle measurements, including the recorded menu
inputs. P5 means are update 567,375, draw-loop 343,901 and full 983,242 cycles;
draw-loop time excludes presentation overhead. P5 maxima are update
11,957,445, draw 687,701 and full 12,358,690 cycles. Its 5,498 over-budget
updates are individual updates, not timing windows.

Performance remains unfinished. Carried P4 costs about 5.3% more than the
standalone P4/minimap build's 987,671-cycle mean. This promotion adds chapter
coverage and transport/save functionality; it does not qualify 30 fps or
physical-GBA performance.

## Fresh-process cargo SRAM

Ordinary input saves at update 918 / frame 900 while Shuttle slot 12 contains
Tassadar and two Zealots (three passengers, six cargo spaces). A new mGBA
process loads the preserved 32-KiB SRAM through title-screen B. Semantic,
world, P5 and cargo hashes match exactly: `497978288`, `575039328`,
`1986064928`, `4218757647`. Runtime is valid and fault-free. The remaining
ordinary input tape completes the P5 mission win at update 5,561 / frame 5,496,
with all 221 transitions consumed.

Private evidence, ROM/input hashes, dependency files and timings are recorded
in `.local/build/p5-minimap-carry-prep/qualification-final.json`. The accepted
cargo result is under
`cargo-sram-roundtrip/fresh-process-roundtrip-1792997468181012000/result.json`.
Earlier failed private harness attempts are not qualification evidence.

## Publication checks

Rebuilding all five carry ROMs using public `PROTOSS_ROM_FLAGS` and the
canonical P5 header produces byte-identical qualified ROMs. The cargo save
and resume ROMs also match their accepted artifacts exactly. Compiler
dependency files resolve tracked source and the canonical header, with no
private bank include path. Evidence is in
`.local/build/p5-minimap-carry-prep/public-replay-identity.json`.

The final public checks passed locally:

- `make` and `make test`: 494 Python tests plus the default host checks.
- `make unified test-unified`.
- `./dev build protoss` and `./dev test protoss`, including carried P1–P5,
  canonical transport/runtime/profile/frame checks, storage, Storm and P4
  controller checks, plus the 234-case startup/save matrix.
- `./dev build protoss-p4-storm` and `./dev test protoss-p4-storm`.
- `make race-campaigns showcase test-race-campaigns test-canonical-render`.

The canonical P5 ROM is padded to 32 MiB, SHA-256
`c20a1c09d823f0372455dd4cf8d948efebd9ffcb257603d1522298f7827141c5`.
Its production IWRAM end is `0x030047c0`, below `0x03005000`; BSS ends at
`0x0203d980`, below `0x0203e000`. The save payload is 16,292 bytes, below the
unchanged 16,368-byte limit. Explicit P4 Storm remains a separate verified
product. Source-only staging checks run before publication. Actions remains
disabled/manual-only.

Construction uses imported source art but still omits the later Protoss
materialization states after the build timer. This promotion does not claim
that its construction sequence matches native N64 gameplay.
