# Explicit worker gathering and cargo identity

`SC64_WORKER_CARGO_KIND` is a prototype, disabled in every product. It requires
the race engine and field controls, and excludes the separate legacy skirmish
adapter. The change adds a real A-on-mineral command instead of allowing the
open-ground MOVE fallback to consume that click. The exact visible patch is
selected within a bounded hit radius; owned actor selection, refinery and
combat contexts keep their existing priorities. Only selected, unloaded
workers receive the mineral command.

Worker-only flags hold gas cargo (`0x20`), ambiguous legacy cargo (`0x40`)
and an explicit gather trip (`0x80`). The amount remains in `carry`; no actor,
codec row or save payload grows. Source worker rows using reserved flags are
rejected before binding. The P7 typed row cannot encode a negative `build`
sentinel below -2, so this design does not overload that field.

MOVE, ATTACK and STOP retain cargo kind and amount. Gather trips retain old
cargo through travel; successful pickup replaces it with the new resource.
Same-kind cargo reaches the selected resource before returning. This cargo
switching policy follows the [pinned OpenBW gather paths](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h):
recognized old cargo remains during travel, and a positive pickup installs
the new kind and amount. It is behavioral reference evidence, not an N64
execution or timing-equivalence claim. A depleted
patch or invalid pending target cancels the trip without inventing a nearby
target or erasing cargo. Loaded idle human workers wait for another command instead
of being zeroed by the periodic auto-miner. Bot AI explicitly resumes its idle
loaded workers after construction so its economy can keep working. Badges use cargo kind independent
of order; unknown cargo has no guessed mineral/gas badge. Repair's existing
Q8 `carry` payload is treated separately from resources. This prototype does
not add resource retention to the existing repair state machine.

Feature-on worker ticks run in a shared noinline ROM function. The hot combat
loop and the 11 KiB stack reserve stay in place. Initial Terran/Z6 links with
the larger inline worker path exceeded the reserve; the ROM split corrected
that. Guard preflights with audio enabled then reported:

| Profile | BSS end | IWRAM end | Save payload bytes |
| --- | --- | --- | ---: |
| Terran | `0x0203df2c` | `0x03004b38` | 15,816 |
| Zerg Z6 | `0x0203df00` | `0x03004bd8` | 16,296 |
| Protoss P7 | `0x0203ddf8` | `0x03004b68` | 16,292 |
| Zerg Z7 candidate | `0x0203d75c` | `0x03004d58` | 16,296 |

These are link preflights, not hardware or frame-rate measurements.

The feature uses a separate journal tag (`scenario tag + 0x01000000`) so old
ROMs cannot accept the new meanings. Loading first tries that exact tag, then
the previous scenario tag with the same payload size. The payload version must
match its journal. Migration changes only the private candidate: old MINE/GAS
cargo has a known kind, while old interrupted cargo becomes UNKNOWN without
crediting a guessed bank. The normal mission validators still run before any
live actor or resource-bank mutation. Subsequent saves use the new tag.

With the local inputs for the selected profile already prepared:

```sh
make test-sc64-worker-gather-terran
make test-sc64-worker-gather-z6
make test-sc64-worker-gather-p7
make test-sc64-worker-gather-z7
```

Each opt-in target runs interaction-dispatch/tick checks, focused bot-worker
resumption checks and actual-codec old/new save checks. The first uses authored scenes and direct engine entry
points, not a normal controller replay. The save matrix covers all three
human races in each profile (12 combinations), including P7's typed codec,
known/ambiguous old cargo, marker conflicts, version mismatch and atomic
rejection. The fixtures contain no scenario data or imported art.

Before activation, finish normal-button/controller qualification, carried
missions, fresh-process SRAM checks and representative performance/render
checks. Z7's current-controls controller is still under investigation. No
mission win or near-30-fps result is claimed by these fixtures or links.
