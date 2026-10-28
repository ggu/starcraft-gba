# Source VFX offscreen cull

Status: enabled with the source-weapon renderer in the canonical Terran, Zerg
Z6, Protoss P7 and Showcase products after current-main integration checks.
Clear `CANONICAL_SOURCE_VFX_CULL_FLAGS` to retain the renderer without this cull.
The original-art default build remains independent of the private bank.

## Behavior and bounds

When `SC64_WEAPON_SOURCE_VFX_OFFSCREEN_CULL` is defined, `draw_tracers` may
skip a tagged source VFX trace before duration, timeline, and overlay work. The
bank envelope is scanned once from the prepared immutable frame, overlay, and
LO-offset tables, then held in a process-local four-byte tri-state cache
(unknown, invalid, valid). It is not serialized. The current prepared bank
header has SHA-256
`9520924358654c101e7995483170545510f519b1ea95e3334aef4220436e3a06`; its
derived radii are 66 by 61 pixels. These dimensions are test expectations for
that bank, not product constants.

The cull is conservative: normal and mirrored crop origins, canvas origins,
child overlay offsets, and LO offsets contribute to the envelope. Actor,
computed launch, and target positions bound supported straight/stationary
trajectories and their births. Unsupported motion types, unknown or invalid
banks, unrecognized overlay operations, out-of-range launch math, and unsafe
endpoint spans fall back to drawing. No gameplay, tracer lifetime, or save
state is changed.

## Source-only host reproduction

These checks compile only the two test translation units and repository
headers. They do not generate art. Set `BANK_DIR` to an existing directory
containing `sc64_weapon_source_generated.h`; use the recorded prepared bank
from the audit environment when reproducing its exact 66x61 expectation.

```sh
mkdir -p build
clang -std=c11 -O2 -Wall -Wextra -Werror \
  tests/sc64_weapon_source_vfx_bounds.c -o build/test-vfx-bounds
./build/test-vfx-bounds

BANK_DIR=/path/to/prepared/weapon-vfx/generated
clang -std=c11 -O2 -Wall -Wextra -Werror -I"$BANK_DIR" \
  tests/sc64_weapon_source_vfx_cull_bank.c -o build/test-vfx-cull-bank
./build/test-vfx-cull-bank
```

The prepared-bank pixel oracle compares enabled and disabled culling against
the actual prepared source art over 1,410,048 cases and all 32 directions. The
synthetic oracle separately checks crop/child placement and clipping boundaries
against per-pixel visibility. The test also covers unsupported motion types,
missing profiles, invalid bounds, and launch/endpoint extremes. The captured
run reported 344,832 cull decisions and 163,060 drawn cases. Those counts
describe this test vector, not expected game-wide rates.

## Bounded ARM result

### Current-main integration

The follow-up comparison includes main `523a5e2`, its render caches, N64
interface and selected vital bars. Enabled and disabled builds use the same
normal product define union and existing absolute private inputs; the cull
define is the only runtime switch between each pair. Each run starts Blood
Bath through the menu, with the ROM's race as human and three mixed-race AIs,
then leaves the human unattended until defeat. All sampled semantic world
records match between host and ARM and between enabled/disabled builds.
Host runs also compare the actual RGB555 frame at each checkpoint: 12 records
for Terran and 14 each for Zerg and Protoss (world and pixel records combined).
These are sampled fixed-camera comparisons, not exhaustive visual coverage.

| Profile | Measured-run mean FPS, disabled → enabled | Mean cycle change | Selected busy window FPS |
| --- | --- | --- | --- |
| Terran | 28.35 → 28.39 | -0.13% | frame 6001: 28.74 → 28.82 |
| Zerg Z6 | 27.51 → 28.16 | -2.31% | frame 12001: 25.78 → 27.71 |
| Protoss P7 | 27.74 → 28.53 | -2.77% | frame 9001: 25.44 → 28.57 |

The final Zerg and Protoss windows regress by 1.23% and 1.47% respectively:
26.43 → 26.11 and 23.54 → 23.19 FPS. This cull helps the off-screen workload;
it does not remove the visible-effect cost or guarantee steady 30 FPS.
The measured-run mean divides all measured cycles by all measured updates;
it is not an average of rounded window FPS. The seven or six semantic
checkpoints per run and the unattended defeat do not prove campaign wins,
all camera positions, every AI configuration or physical-GBA timing.

Additional current-main qualification:

- The full source-only suite passes (643 Python tests, 20 optional skips,
  plus native fixtures), as do the default ROM, source-boundary and whitespace
  checks. A later Make/test-only correction scopes activation to the four qualified
  products and uses an injected temporary Make file to inspect the real
  product flags; the five focused cases pass again.
- The prepared-bank ASan/UBSan pixel oracle still passes all 1,410,048 cases.
- The normal-menu host skirmish/save/reload matrix passes 24 cases in each
  race profile (72 total).
- Separate current-source ARM fresh-process SRAM checks pass for all three
  race ROM profiles: save at frame 1801, continue to 2701, then start a new
  process with the same ROM and unchanged SRAM. Restored and continued
  semantic hashes match their saved/reference records. Each profile has a
  private HOME/config/save directory, and all six owned processes are
  confirmed terminal. This is emulator SRAM continuity, not physical-GBA
  qualification.
- Four normal ARM product links retain the unchanged IWRAM/BSS guards.
  No Unit, Tracer, packed save row, save identity or simulation state changes.
- The directly compiled P7 cull helper retains its 280-byte static stack
  frame. This is a compiler frame report, not a whole-game stack watermark.

The current detailed evidence is local under
`.local/build/vfx-current-paired/`, `.local/build/vfx-current-products/` and
`.local/build/vfx-current-default/`; fresh SRAM records are under
`.local/build/vfx-current-fresh-sram/`. The paired result records identify the
ROMs, argv, source hashes, isolated emulator launch handles and terminal
process exits. No Actions, GDB or private asset generation was used.

### Earlier P7 comparison

The paired direct ARM build used the same prepared P7 long-run input and
produced identical seven world-state records. At frame 9001, total cycles per
frame fell from 703,474.6 to 595,449.4 (-15.36%), equivalent measured rate
rose from 23.85 to 28.18 FPS, and tracer/spell cycles fell from 140,260.5 to
27,288.9 (-80.5%). At frames 6001 and 12001, tracer/spell cycles fell by
84.0% and 78.1%; total cycles fell by 0.68% and 0.58%. At frame 3001 total
cycles were effectively unchanged (+0.000005%). At frame 12888, total cycles
rose by 0.49% and tracer/spell cycles rose by 7.2%, showing overhead when the
late trace workload is less favorable. These results support a busy-window
benefit, not a whole-game frame-rate guarantee.

The cache object is four bytes; linked BSS end grew by eight bytes because of
alignment (2,580 bytes remain under the existing BSS guard). IWRAM end did not
move. The cull helper's recorded stack frame is 280 bytes. The broader
current-main checks above supersede the earlier default-off status; full
campaign and hardware performance qualification remains open.

Detailed local run records from the original audit environment were kept under
`.local/build/vfx-offscreen-cull/` and
`.local/build/vfx-offscreen-cull-arm-final/`; those machine-local artifacts
are not part of this source commit.
