# Source VFX offscreen cull

Status: candidate implementation and one P7 ARM comparison are reviewed; the
runtime flag remains off by default pending broader integration qualification.

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
move. The cull helper's recorded stack frame is 280 bytes. Broader profile and
campaign qualification is still required before enabling the flag by default.

Detailed local run records from the original audit environment were kept under
`.local/build/vfx-offscreen-cull/` and
`.local/build/vfx-offscreen-cull-arm-final/`; those machine-local artifacts
are not part of this source commit.
