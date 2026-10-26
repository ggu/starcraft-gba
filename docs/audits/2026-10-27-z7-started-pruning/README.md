# Z7 started-trigger census pruning probe

`SC64_Z7_PRUNE_STARTED` is a default-off Z7 runtime probe. Authenticated Z7
source trigger rows are non-preserved one-shot programs. Once the runtime's
`started` bit is set, the scheduler cannot start that trigger again. The
private callback therefore returns false for its condition while the scheduler
continues to advance any active action clocks before evaluating conditions.
No AI rules, roster, actor caps, condition cadence, save fields, or campaign
content change.

The optimization keeps `sc64_z7_count_snapshot_begin()` as the strict full
reference census. It validates all five canonical rows and required location
bindings before computing counts. Under the probe, `active` is a five-bit
validity mask; without the probe it retains its original boolean meaning.

- Needed mask `31` takes the full strict census directly. This is the common
  opening state, when none of triggers 0, 2, 4, 6, or 7 has started.
- Needed mask `3` counts only the owner-unit/building loss predicate for source
  player 13 and the hostile force-1 building predicate. It runs only after the
  game adapter has validated all live actors. Neither source condition uses
  type or geometry.
- Other masks use the generic bounded census, computing only requested
  predicates while retaining canonical row and region validation for every
  row.

The fast callback is installed only on a local copy of the verified game
world-ops binding. It checks the canonical context, runtime state, active Z7
map, callback identities, row identity, and non-preserved trigger metadata.
Malformed actors fail before the snapshot. A malformed mask or failed masked
census falls back to the strict full snapshot and baseline prevalidated
callbacks. A stale callback binding, cleared authentication bit, or map
mismatch follows the original checked runtime path and retains its fail-closed
behavior. The public checked scheduler and world callbacks are unchanged.

## Checks and measured replays

`make test-sc64-z7-prune-started` runs the host fixture with the full-render Z7
profile, once with the probe off and once on, under ASan and UBSan. The fixture
compares all 32 possible needed-counter masks against direct live world counts.
It also exercises actor mutations, active waits and pause, save/load, loss and
victory conditions, malformed rows and contexts, corrupted callback/auth
bindings, and actor-provenance failures. Both variants passed.

The opening ARM comparison used the same 12,000-tick input-only tape and full
render flags, with only `SC64_Z7_PRUNE_STARTED` added to the candidate. Both
runs matched all 34 explicit game-state checkpoints, ending at frame 11,726
with no fault, win, or loss. Mean measured cycles were:

| Opening replay | Update | Draw | Full |
| --- | ---: | ---: | ---: |
| Probe off | 494,719 | 574,241 | 1,068,960 |
| Probe on | 487,785 | 575,809 | 1,063,595 |

The observed full-cycle change was about 0.50% lower with the probe; update
was 1.40% lower and draw 0.27% higher. Since this opening keeps mask 31 and
uses the full census, this small difference is not evidence of census pruning
benefit and may reflect code layout or measurement variation.

The late-game comparison started from a genuine input-generated save at tick
90,128 / frame 89,340 with `started=250` (`0xFA`) and a 16,296-byte save
payload. The source input trace had 236 verified checkpoints. Both variants
loaded through the normal UI and matched all 20 continuation digests for 2,000
ticks, ending at frame 91,340 without a fault, win, or loss.

| Late continuation | Update | Draw | Full |
| --- | ---: | ---: | ---: |
| Probe off | 7,820,685 | 436,689 | 8,257,375 |
| Probe on | 7,735,265 | 441,342 | 8,176,607 |

The observed full-cycle change was about 0.98% lower with the probe. Update
was 1.09% lower and draw 1.07% higher. This is a 2,000-tick idle continuation
from one saved state, not an uninterrupted-run comparison or mission
completion.

Both ARM pairs passed the existing linker assertions. For all four links,
IWRAM ended at `0x03004ca8` (856 bytes below the `0x03005000` bound) and BSS
ended at `0x0203d6b4` (2,380 bytes below `0x0203e000`). No new persistent or
BSS fields were added; the masks and temporary counts are local. The four
actual ROM load ends were `0x09eb224c` / `0x09eb2a1c` (opening off/on) and
`0x09eaf0d0` / `0x09eaf8a0` (late off/on), each below the 32 MiB GBA ROM
limit. The late `.gba` files were padded to 32 MiB. These link margins do not
measure stack high-water or physical-hardware performance.

## Scope and provenance

The probe is not part of canonical Z7 build flags; the Make target above
exists for the paired host fixture. Opening evidence is retained privately in
`.local/build/z7-live-20261027/opening-current/`; late-save evidence and its
manifest are in `.local/build/z7-live-20261027/late-snapshot/`. The late
manifest records the save and input-trace hashes. Source hashes shared by the
late pair, before the additional game-level preprocessor dependency guard, were:

- `starcraft/game.c`: `9b1e32d4f220fc0381500fdb90f64a36f11eff6ca3a3119a97d3a390ff63f0d8`
- `starcraft/sc64_routes.h`: `871fab9c1dc79a953c882c72d6e3db74fb9e7bd36f7427c700be00624372a706`
- `starcraft/sc64_z7_runtime_adapter.h`: `73ec22724d0a0677a1acad270e3d031fb3d3231f8c638e945b2c1932b5c6caf8`
- `starcraft/sc64_z7_world_adapter.h`: `bd45a88768420491dd0d3f14754b6cf94635ac1ceaa6755c70eea43c4f38043a`

These probes establish paired digest continuity and measured emulator cycle
counts for the stated inputs only. They do not establish a Z7 win, 30 fps,
stack high-water, or hardware qualification.
