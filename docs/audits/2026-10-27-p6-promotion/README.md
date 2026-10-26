# Canonical Protoss P6 promotion — 2026-10-27

Base default branch: `8335a7ce445b410d9ac2ae894e2f7e8f78af927f`.
The P3 controller repair is commit `e8f4053`; the Z5 actor lookup fix is
`5d65d89`. This promotion carries both repairs.
Canonical campaign coverage becomes **T10 / Z6 / P6 = 22/30**, not 30/30.
Zerg 7–10, Protoss 7–10 and near-30-fps performance remain unfinished.

## Product and preparation

`protoss` and `protoss-p6` now select:

- `build/StarCraft-Protoss-P6-GBA.gba` / `build/starcraft-protoss-p6.elf`.
- Title/code `SC PROTOSS6` / `SCP6`, save generation 30.
- Emulator save directory `.local/play/protoss-p6`.
- Packed sprite bank `.local/sc64/protoss-p6/generated`, separate from the
  internal P6 probe bank and the preserved P5 bank.

Preparation includes authenticated P4/P5/P6 map/scenario inputs, P6 abilities,
doodads, and the complete P6 actor bank with packed unit words. Build/status
checks reject a marker-only or incomplete packed bank. Canonical flags pin the
validated P5 and P6 query options independently of probe overrides; an
asset-free test evaluates Make with overridden `P5_QUERY_FLAGS`,
`P6_QUERY_FLAGS` and `P6_EXTRA_FLAGS` and confirms those overrides do not enter
the product vector.

Explicit `protoss-p5-shuttle` keeps its original real ROM/ELF, `SCPB`,
generation 26 and `.local/play/protoss-p5-shuttle`. Rebuilding it produces the
same SHA-256 as before this promotion:
`0015922c21587703df17c3e0581b4370fddd9f16d003608a3724eb2692fd0d46`.
The internal P6 probe retains `SCPC`; no saves are converted between products.
Terran and Zerg product selection is unchanged.

The full suite exposed a real P6 Civilian sprite lookup gap: the Z5 enum had
source art, but the generic raw-15 lookup required Terran mission flags. The
resolver now prefers Z5 actor art for Z5 enum types, preserving the generic
T4 and P6 hero mappings. The existing all-profile roster test catches the
original failure and now passes without removing actors or weakening checks.
The source expression for profiles without Z5 actors is unchanged.

## Current host qualification

All six controllers use ordinary buttons under the full canonical P6 flags,
including its packed bank, facing, weapon/attack/death art and shared skirmish
runtime. Wins:

| Mission | Recorded completion |
| --- | ---: |
| P1 | 9,303 controller input updates |
| P2 | 32,220 controller input updates |
| P3 | gameplay frame 120,733 |
| P4 | gameplay frame 104,346 |
| P5 | gameplay frame 5,496 |
| P6 | gameplay frame 49,599 |

P3's previous controller stalled on both current P5 and P6. Its repair selects
reachable targets across both enemy forces, frees pathless builders using
normal STOP, balances gas/mineral workers, permits more purchased pylons, and
rebuilds before the final assault. It changes controller decisions, not game
stats, resources, source objectives or victory assertions. The paired P5/P6
runs match all 122 recorded checkpoints and complete at the same frame.
A separate host replay feeds the 4,898 recorded inputs without controller
route queries and invokes update/audio/render each tick. It matches all 31
serialized checkpoint fields at all 122 checkpoints and wins. These fields
cover mission/campaign state, resources, supply, counts and UI state; they are
not a bytewise world or pixel comparison. Host audio_update is inert.

Other current canonical checks pass:

- Normal-menu startup/save matrix: 234 configurations, six campaign entries.
- Storage codec: 240 units, 160 buildings, 144 minerals; 16,348-byte RaceSave.
- Canonical framebuffer comparisons across cold/warm caches, camera movement,
  fog, transitions, buildings, save/load, restart and P6 doodads; HUD checks.
- Packed worker cargo and both clipped/unclipped blue Warp color fixtures.
- Mid-match skirmish resume on both maps, all human races and 1–3 mixed-race
  opponents, including groups of three same-race opponents. These 24 Protoss
  cases compare restored state and 900 continued updates. The 234-case setup
  matrix separately covers independently chosen bot-race permutations.
- P6 save/game fixtures and carried P5 Shuttle UI/runtime.
- P4 Storm fixture under P6 generation 30, preserved P5 generation 26 and
  standalone P4 generation 21.
- 551 Python checks and the asset-free host suite; unified build/host suite;
  source-boundary, doctor and whitespace checks.
- After the shared actor lookup fix, canonical Z6 also passes split/save,
  its 234-case startup matrix, framebuffer/HUD and skirmish resume fixtures.

The setup/resume cases do not prove a completed skirmish for every possible
configuration. Earlier P1–P5 ARM wins remain evidence for the preserved P5
image; this promotion does not claim six new complete ARM campaign replays.

## P6 input-only ARM replay

An isolated no-GDB mGBA process streams the recorded buttons from ROM const
data, without the live controller or external memory writes. The wrapper calls
game update, audio update and render in the ordinary order. All **57** observed
checkpoints match, ending with victory at frame **49,599**, both heroes alive,
Zeratul inside source objective location 15, P6 completion set and fault zero.
Comparisons cover the mission model, started triggers, switch bits, action
cursors/timers and the recorded hero identity/ownership/position/HP/order.
They do not compare every actor byte or framebuffer.

Measured across 50,241 calls beginning on the play screen (including command
menu input calls that do not advance simulation):

| Work | Mean ARM cycles |
| --- | ---: |
| Update | 1,117,845 |
| Audio and drawing | 432,911 |
| Combined | 1,550,756 |

At 16,777,216 cycles/second this is **10.82 fps equivalent throughput**.
Checkpoint/reporting comparisons are outside the measured section. This
instrumented replay is not a physical GBA measurement, a benchmark of every
campaign/skirmish or proof of 30 fps. The historical probe's 1,596,933 cycles
and older 12.75-fps route have different feature/workload scopes. The
[simulation stage profile](../2026-10-27-p6-simulation-profile/README.md)
identifies mission/order work as the main update costs.

## Fresh-process SRAM restart

A separate replay uses two owned mGBA processes and a fresh isolated save
folder; no savestates, debugger or external world writes are used.

1. Replay the ordinary P6 inputs to host tick 26,461 / gameplay frame 26,100.
   Held input is zero, normal mode is active, and menus/box gestures are closed.
   Send START then B through game_update to save without advancing simulation.
2. Send SIGTERM to that owned process and preserve its 32,768-byte SRAM file.
   Launch the same ROM in a new process; HOME L loads through the normal UI.
3. At the restored frame, send START/B to reserialize through the ordinary UI.
   Both the in-game checksum and a later direct byte comparison confirm that
   the original sequence-1 payload equals the cold-loaded sequence-2 payload
   across all **16,348 bytes**. The sequence-2 record remains in the alternate
   slot after the victory autosave; this makes the byte comparison independent
   of the wrapper's 32-bit hash.
4. Resume with A and replay the remaining original inputs. All remaining
   observed checkpoints match and the mission wins at frame 49,599, fault zero.

Payload SHA-256: `3979a079b950c17b71cebed79ed4f574594b0bb4a61732492cb36ece91847c8e`.
This is live mid-mission continuity for the observed route, not every possible
save position or a physical power-loss test.

## Production fit and local reproduction

Production ROM SHA-256:
`c34c043b028c3bfc410297f42652a23cde3cd173bc46a736e8226ecb1e75c776`.
The instrumented replay ROMs have different titles/code and SHA values; they
are not the production image.

| Bound | Production result | Remaining guard margin |
| --- | --- | ---: |
| ROM load end | `0x09e53588` | 1,755,768 bytes below 32 MiB cartridge end |
| IWRAM end | `0x03004990` | 1,648 bytes below `0x03005000` |
| BSS end | `0x0203dc68` | 920 bytes below `0x0203e000` |
| Save payload | 16,348 bytes | 20 bytes below 16,368-byte limit |

The ROM is padded to exactly 32 MiB. None of these bounds was relaxed.
With an entitled, verified local input ROM:

```sh
./dev sc64 prepare --rom /path/to/StarCraft64-USA.z64 --profile protoss
./dev sc64 build --profile protoss
./dev sc64 test --profile protoss
./dev sc64 play --profile protoss
```

The focused controller target supports a separate trace path:

```sh
make test-sc64-p6-canonical \
  PROTOSS_P6_CONTROLLER_TRACE=.local/build/my-p6-controller.json
```

For this qualification, shared generated inputs were read without regeneration.
Production compiler/objcopy/fix/verify recipes were extracted from make -n and
executed directly. The full test-protoss suite also pins already-prepared
inputs with Make's -o option. Local commands, logs and ignored evidence are
under `.local/build/p6-promotion-20261027/`:

- `root/product-final/`: actual production and preserved P5 build commands,
  linker/verification outputs.
- `root/final-arm/`: input-only wrapper build, runner, log and result JSON.
- `root/final-restart/`: two-process logs, SRAM snapshots, result JSON and
  `payload-compare.json` with the bytewise comparison result.
- `carried/p3-repair/`: paired controller traces, independent rendered replay
  source/command and checkpoint log.
- `integration/`: sprite manifest parity and focused host fixture commands/logs.
- `root/canonical-suite/`, `root/default-final.log`, `root/unified/`: suite logs.

The private replay wrappers are local qualification artifacts, not a shipped
portable replay tool. Game data, generated banks, ROMs, saves and captures stay
ignored. No Actions were enabled or dispatched.
