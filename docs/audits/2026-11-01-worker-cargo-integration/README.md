# Worker cargo on the shared release engine

The candidate enables persistent worker cargo in the normal Terran, Zerg Z7,
Protoss P7 and Showcase builds through `CANONICAL_WORKER_CARGO_FLAGS`. Campaign
and custom games use the same engine. An explicit empty value selects the
previous cargo behavior for comparison. Late Z9/Z10 builds inherit the shared
capability. This does not promote additional campaign missions or complete the
project's 30-fps goal.

## Behavior and persistence

Gather commands use the current interaction priority and the HUD's visible
mineral target. Workers retain cargo through MOVE, ATTACK, STOP and construction
travel; a successful new pickup replaces it. Construction preserves arrival
validation, payment and cancellation; a Drone loses its cargo when construction
consumes it. Bot recovery, gas balancing, construction and scouting preserve
cargo identity. Resource badges reflect the carried resource during movement
and idle; unknown cargo has no guessed badge. Repair remainder state remains
separate from resources. The badges are authored UI cues, not newly imported
N64 cargo animation assets.

Migration uses the existing impact envelope and Transport1 trailer. New cargo
tags toggle `0x01000000` from the previous scenario version, which already has
that bit set; OR-ing it would not distinguish formats. The Transport1 bit
remains separate. Old records are validated and migrated in scratch before
live mutation. Publication preserves the selected valid predecessor, including
when a newer record has a valid checksum but invalid semantics. Older ROMs do
not accept the new cargo tag.

No Unit, serialized row or save payload grows. Dead worker remnants do not block
cargo validation; normal codec validation remains. Save scratch uses the
transient flood queue, not persistent route state. Failed selection/publication
preserves route caches; successful saves retain PR #134's canonicalization.
Cargo save support requires impact events and excludes the separate Scarab
namespace experiment. Unsupported combinations fail explicitly at compile time.

## Qualified runtime and limits

Runtime commit `81083fbfb4a2010b1173cdd5793bf516929c97f1` contains the integrated
engine. `9aad50ddcdc48315bd9231fae4ab16cf9015a007` adds test fixture updates and
real-codec journal fallback checks; runtime/common/Makefile are identical to
810. Later candidate wiring and render checks need their final build receipts.
Historical October 28 qualification does not establish this combination.

Completed local checks:

- Fresh unprepared 810 source boundary, doctor, default build and full suite:
  952 Python checks, 26 optional skips, separate 5/8 suites and native checks.
- Cargo-enabled interaction, AI and real-codec fixtures at 810 for Terran, Z7,
  P7 and Showcase. All three human races are covered in each codec group.
  The expanded real-codec fallback/publication cases pass in all four at 9aad.
- Six actual production ARM vectors with cargo enabled pass original guards.
  EWRAM/IWRAM headroom in bytes: T10 28336/2192, Z7 2168/128, P7 23668/1496,
  Showcase 28544/832, Z9 and Z10 8/1040. An initial Z9 link failed because
  compiler alignment added eight bytes; storing the boolean path-cache flag
  as a byte restores the original margin. Reserves and limits are unchanged.
- 936 normal-menu cases at 810: four products x two maps x three human races x
  every ordered 1/2/3-AI race assignment. Each runs 60 live updates and checks
  same-process save/load/resave payload equality. This is not a mission win.
- Four physical fresh-process SRAM runs at 810: normal map0 Terran versus Zerg,
  60 updates before saving and 60 resumed updates equal uninterrupted state.
  Restored lobby, selected semantic state, 32-KiB SRAM and unchanged load bytes
  pass. The existing physical-mode rejection test also passes. No debugger
  world writes or forged guest SRAM are used.
- Unified ARM and host aggregate at 810 pass with their supported legacy flags.
  The optional nonproduction BC/pre-energy codec case passes at 9aad with
  transport, bunker and production-core-required flags removed; this does not
  qualify a current full-feature product.
- Actual-pixel render checks pass for Z7/P7, cargo OFF/ON, all three worker
  types, both resources, MOVE/IDLE, unknown/empty suppression, fog and clipping.
  These profiles do not exercise the conditional repair-render branch.

The full Terran aggregate needs the final normal-flag run. Earlier attempts to
force cargo into every fixture stopped at an old save-writer call and then at
legacy fixtures without impact support. The fixed helper preserves rejection
assertions while isolating intentionally invalid mock candidates from valid
fallback records. Current production flags should select cargo automatically;
legacy isolated fixtures retain their own supported flags.

## ARM performance

The unchanged public crowded-scene harness runs real terrain, starting workers
and ordinary AI, adding attackers only at boot. At 810, three OFF/ON pairs run
500 physical updates each, with identical initial hashes and no sampled counter
differences. Mean full-frame guest cycles:

| Scene | Cargo OFF | Cargo ON | Change |
| --- | ---: | ---: | ---: |
| Z7, 48 actors, 2 AIs | 758507 | 759069 | +0.07% |
| Z7, 96 actors, 3 AIs | 1160241 | 1164173 | +0.34% |
| P7, 48 actors, 2 AIs | 872249 | 866069 | -0.71% |

Mean update changes are +0.53%, +0.41% and -0.97%. Full-frame counts include
update, audio, render, presentation wait and measurement overhead; update counts
also include the probe observer. These are authored emulated ARM workloads, not
hardware FPS or isolated orders/render timings. Busy scenes remain below 30fps.

## Reproduction

With the selected private inputs already prepared:

```sh
make test-sc64-worker-gather-terran
make test-sc64-worker-gather-z7
make test-sc64-worker-gather-p7
make test-sc64-worker-gather-showcase
make test-sc64-terran-full-campaign
```

The cargo fixture groups use authored scenes and actual codecs, not ordinary
campaign-win controllers. The public menu, crowded ARM and physical continuation
tools retain their narrower contracts. Native checks use the local shared lock,
frozen source/input hashes and disk guard. No Actions were enabled or dispatched;
no game data, ROMs, saves, generated banks or proprietary captures are committed.
