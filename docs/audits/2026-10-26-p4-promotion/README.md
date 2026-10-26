# Canonical Protoss chapter four

Canonical `protoss` now includes P1–P4, bringing the three canonical race ROMs
to 18 campaign entries: ten Terran, four Zerg and four Protoss. All retain
Outpost/Blood Bath selection, every human race and one to three independently
selected AI races. The 30-mission goal and near-30-fps target remain unfinished.

## Identity and setup

Canonical Protoss uses `StarCraft-Protoss-P4-Storm-GBA.gba` /
`starcraft-protoss-p4-storm.elf`, game code `SCP5`, title `SC P4 STORM`, save
generation 21. The existing Storm variant identity is retained; it is P4,
despite the final digit of the game code. `protoss-p4-storm` is an explicit
alias for this build. `protoss-p3` retains the real P3 files
`StarCraft-Protoss-GBA.gba` / `starcraft-protoss.elf`, code `SCP3`, generation
7; the versioned P3 filenames remain one-way compatibility symlinks.

The wrapper uses `.local/play/protoss-p4-storm` for canonical P4 saves and
preserves `.local/play/protoss` for explicit legacy P3. It performs no save
conversion. The plain P4 experiment (`SCP4`) remains separate.

```sh
./dev sc64 prepare --rom "/path/to/StarCraft 64 USA ROM" --profile protoss
./dev sc64 build --profile protoss
./dev sc64 test --profile protoss
./dev sc64 play --profile protoss
```

Preparation requires the P3 and P4 map payload gates, the complete shared
sprite bank with packed metadata, and the P4 scenario/source manifest. The
manifest must identify P4, catalog ID 23, BOLT path `008/021`. The existing
packer authenticates the source CHK, including all 201 UNIT records and 13
triggers. No maps, art, ROMs or private replay data are published.

## Current-source mission qualification

The P4 controller uses only ordinary menu, selection, movement, construction,
production and harvesting inputs. Its route assigns Probes to the existing
Assimilator, pays for a Cybernetics Core and Dragoons, builds reachable
Pylons/Gateways/Cannons, clears the escort route, then brings Tassadar and
Raynor into the source escape region. Construction helpers verify the exact
live, owned, correctly typed target building and placement. Resources,
actor pools, combat facts and source mission conditions are unchanged.

The cleaned host controller wins at input tick 104,850 / game frame 104,346.
The independent input-only host and no-GDB ARM replays consume the same
1,990 transitions and finish with:

- P4 win, result screen, campaign complete and primary objective set;
- all 87 source actor creations, fired mask `0x1ff8` and zero faults;
- valid runtime, both heroes alive, human-owned and inside the escape region.

The final cleaned trace's input transitions match the already-qualified ARM
tape exactly. Diagnostic path queries are off. Trace output uses exclusive,
PID-suffixed paths, retaining earlier qualified traces. The Make test prints
the new qualified trace path rather than overwriting the historical default.

P1–P3 were separately requalified in the larger P4 pools and final renderer
flags. Host and no-GDB ARM state/framebuffer hashes match at all 10, 33 and 48
checkpoints. P1 wins at tick 9,303 / frame 9,252; P2 at tick 32,220 / frame
32,208; P3 at tick 47,862 / frame 47,238. Fenix survives P1/P2. The P4-only
P3 input controller prioritizes additional Gateways and queues earlier;
legacy P3 retains its prior route and still wins at frame 47,626.

## Live SRAM continuity

A separate ARM process plays the same tape to host tick 30,000 / frame 29,817,
while P4 is running, then saves using the ordinary Start/B controls. A fresh
process loads through title B. Active-unit fields, live buildings and queues,
minerals, campaign state, P4 runtime/Storm state, and clock/economy/RNG hashes
match across save/load. Raw whole-struct hashes differ because the codec
normalizes dead-slot, padding and transient state; those bytes are not used
as the gameplay continuity oracle.

Continuing the remaining ordinary input tape from that loaded SRAM wins at
host tick 104,851 / frame 104,346, with the same source creation/fired/fault
and hero escape checks. The extra input tick is load/replay scaffolding,
not an extra mission frame. This is a live mid-mission cold load, separate
from a terminal-save check. The generic split-profile report checker does
not establish P4 save continuity; this dedicated codec/input proof does.

## Rendering, memory and local checks

The canonical build enables packed unit words, fast vital bars, cold route
flooding, the qualified collision-fact/range culls and the P4 count census.
The actual production
link has IWRAM end `0x030041f0`, BSS end `0x0203d490`, RaceSave 15,364 bytes,
and a 32 MiB padded ROM. Existing stack/RAM/save assertions are unchanged.

A separate quiet no-GDB ARM timing replay before the census consumes the same 1,990 inputs
and passes the same terminal win gates. Across 104 complete 1,000-tick windows
and the final 850 ticks, weighted means are 678,736 update cycles, 126 audio
cycles, 446,261 render cycles and 1,125,323 total cycles per tick. This is
14.91 fps equivalent, far below the 30-fps target. The slowest window averages
1,465,607 total cycles/tick (11.45 fps equivalent); the largest individual
tick is 21,352,256 cycles. These are emulator cycle measurements with private
replay scaffolding, not a physical-GBA or constant-frame-rate result. Per-window
logs and source/trace hashes are under `.local/build/p4-current-timing-luna/`.

`SC64_P4_COUNT_CENSUS_ENABLED` replaces seven count sweeps with one unit pass
and one building pass at the same pre-effect observation point. It retains
Current Player's live-ownership rule, All Players' raw-owner rule, source
raw-type fallback and half-open escape tests. Full entry/exit validators
remain. No actor, model or save fields are added. The disabled branch retains
the original seven predicates as a differential oracle.

The public fixture compares all seven values over 500 randomized actor pools
and independent escape-edge cases. Full host and ARM tapes retain identical
terminal semantic hashes and source win checks. In one quiet ARM pair, mean
update cycles fall from 678,728 to 593,461 (-12.6%); full-loop cycles fall from
1,125,310 to 1,029,730 (-8.5%), about 16.29 fps equivalent. Render timing has
frame-phase variance, and the ~21.35M-cycle maximum full tick is unchanged.
This is material progress but still far below 30 fps. Private paired evidence
is under `.local/build/p4-census-prototype/`.

The tracked-header build also repeats the full input-only ARM win and the
live save/load/continued-win sequence. All six save/load semantic groups
match the pre-census proof. Logs and source hashes are under
`.local/build/p4-census-published/`.

Local checks include the actual canonical P4 Storm and storage fixtures,
all four native-win controllers, startup/save matrix, skirmish economy,
camera/fog/cache/result pixel oracles and canonical HUD checks. The complete
default suite has 491 Python tests plus host fixtures. Canonical Terran's
separate [renderer promotion](../2026-10-26-terran-word-promotion/README.md)
has fresh T7/T10 paired ARM timing results; it remains below 30 fps.

Private P4 proof files are under `.local/build/p4-current-stock/`; carried
mission manifests and independent replay logs are under
`.local/build/p4-carry-luna-3cdd/`. All checks run locally with private emulator
configs, saves and PIDs. GitHub Actions remains disabled/manual-only.
