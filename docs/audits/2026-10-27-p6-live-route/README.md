# P6 live rescue route qualification — 2026-10-27

Base main: `dc4b8fad639f913d4645272675c1c12927764beb`.
This audit records the earlier probe repair. The later canonical product,
packed-bank, carried-mission and SRAM qualification are recorded in the
[P6 promotion audit](../2026-10-27-p6-promotion/README.md).
The game runtime matches the earlier `c3c8ac9` baseline; PR52 added only tests
and status documentation. This change repairs the host controller, without
changing mission actors, source objectives, stock stats, player controls,
combat, pathfinding, save generations or product selection.

## Problem and change

The former default controller replayed 102,005 held-key ticks before handing
control to its live policy. After source-derived movement/contact changes,
that opening missed selections and no longer recruited the intended army.
A fresh run lost at frame 143,714 with zero source-trigger faults. Merely
correcting the first missed selection did not repair the later route.

The controller now starts the six-entry probe with six ordinary menu taps
and immediately steers from live actors. It recruits the two early Marine
groups, approaches within the game's native rescue radius, regroups the four
western Marines with the five eastern Marines, and commits their group
attack-move ahead of Tassadar once he has a spell/shield reserve. The existing
Storm, rescue and extraction logic then completes the mission.

Every gameplay action still goes through `game_update` button input.
The added policy reads the world and updates only controller-local decisions;
it does not assign actor ownership, positions, health, energy, resources,
source-trigger state or completion. In particular, Storm friendly fire and
source melee/contact rules remain intact. The old authored RLE header remains
historical input data and is no longer included by the default controller.
`--prefix-inputs` remains available for explicit older-tape experiments.

`P6_CONTROLLER_TRACE` now lets the Make target write an isolated proof file
without overwriting an earlier frozen trace.

## Current evidence

The tracked controller and the independently compiled private prototype both
win at frame **49,599**, after **50,252 controller updates**, with 3,058 held-key
transitions. The source outcome is WON, the P6 completion bit is set, there
are zero adapter faults, and both heroes are alive. Zeratul is inside the
authenticated objective location 15. Existing terminal/extraction assertions
were retained, not relaxed.

The intended escort is observable: the regroup orders select the early
Marines and the escort commit selects nine actors. Seven reach the front
before subsequent combat losses. The result does not rely on keeping all
rescued soldiers alive or changing the mission's win condition.

A separate replay contains only the recorded buttons and checkpoint
expectations. It includes the unchanged game, not the controller's route
queries. Both its sanitizer-enabled host run (with audio/render calls) and
an owned no-GDB mGBA ARM run match all **57** observed mission/hero checkpoints,
including the final win. Comparisons cover frame/screen/outcome/completion,
started triggers, action cursors/timers, switch bits, fault status and the
recorded hero identities, ownership, positions, health and orders. This is
not a full byte-for-byte comparison of every world object or framebuffer.

The strict trace validator also accepts the tape. A 1,000-tick scheduler
window needs 375 transitions, exceeding a 256-entry queue; a 250-tick dense
window passes. The private no-GDB wrapper streams the const tape directly and
has no debugger or external world-memory writes.

Additional local checks pass:

- `make test-sc64-p6-game test-sc64-p6-save`: real source-world updates, gate
  behavior and the current save codec fixtures.
- `./dev build` and `./dev test`: 548 Python checks plus asset-free host
  regressions. The original-art ROM hash stays `2b89b77d…1cec19a`.
- Source-boundary and whitespace checks; no proprietary payloads are staged.

No Actions were enabled or dispatched. Shared generated banks were read
without regeneration, and only owned emulator processes were stopped.

## Timing and promotion limits

The ARM replay observes 50,241 calls that begin on the play screen (including
command-menu inputs that do not advance simulation). Mean cost is:

| Work | Mean cycles |
| --- | ---: |
| Game update | 1,148,439 |
| Audio and drawing | 448,493 |
| Total | 1,596,933 |

At 16,777,216 CPU cycles/second this is **10.51 fps equivalent throughput**.
It is not a physical GBA measurement or an apples-to-apples comparison with
the different historical 12.75-fps route. Reporting/checkpoint comparison
work is outside the measured section. Update cost dominates this route;
render-only improvements will not by themselves reach 30 fps.

A separate private compile appends the full canonical P5 feature vector to
P6: packed drawing/HUD, minimap, facing, weapon/attack/death art, census and
collision/range optimizations. It links under the unchanged guards:

| Bound | Canonical-feature P6 candidate |
| --- | --- |
| ROM load end | `0x09e0120c` (2092532 bytes below 32 MiB end) |
| IWRAM end | `0x03004990` (1,648 bytes below guard) |
| BSS end | `0x0203dc68` (920 bytes below guard) |

This is compile evidence only. The replay above qualifies the existing P6
probe flags, not that canonical-feature candidate. P6 still needs feature
runtime validation, carried P1–P5 qualification and a current fresh-process
SRAM restart before promotion. Current codec fixtures do not replace that
restart proof. The canonical Protoss product remains P5 and canonical campaign
coverage stays **21/30**. Full campaign delivery and near-30-fps performance
remain active work.

## Reproduce and private artifacts

With existing P6 probe inputs prepared:

```sh
make test-sc64-p6-controller \
  P6_CONTROLLER_TRACE=.local/build/p6-live-route/controller-win.json
make test-sc64-p6-game test-sc64-p6-save
```

The standard trace-fed ARM tools still require the dense probe for this tape;
see the staged campaign replay instructions. The local qualification used an
isolated no-GDB wrapper; its source and generated input tape stay private.

Local artifacts beneath `.local/build/p6-route-20261027/`:

- `root/tracked-controller-win.json` and `root/tracked-host.log`.
- `arm/replay.c`, `arm/tape.h`, `arm/host-replay.log`, `arm/arm-command.txt`,
  `arm/arm.log`, `arm/result.json`: input-only replay and matched checkpoints.
- `root/default-tests.log` and `root/p6-fixtures.log`.
- `canonical-candidate/compile.shcmd`, ELF and map: compile-only fit evidence.
- `adaptive/`: bounded route experiments and the independently winning route.

Trace SHA-256: `7213ff257aa247ee5f09badd532fea1530f882cf02888a0631614078de339244`.
ARM replay ROM SHA-256: `d4e4b74d120629897631eeb6a0f62f5ae202c78fce02c2db445449fd5bb8970c`.
Imported inputs, generated headers, ROMs, saves and captures remain ignored.
