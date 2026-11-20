# Empty Carrier training passes in normal Terran custom games

The trial builds on the normal Terran Carrier composition at `5d2b43aa`
with sparse routing enabled. The first live Unit scan of a source update
already checks Interceptors in slot order. It now records whether it saw
a live Carrier-class parent or an Interceptor-type child. When it saw no
parents, it can skip the parent queue scan. When it saw neither type, it
can skip source flight and frame movement. A live detached Interceptor
keeps those passes eligible on its own.

The scan records child presence before the existing training branches
can remove or dock it. The parent scan can start queued training, so
parent presence also keeps flight eligible. Phase 4 keeps its usual
movement without training or flight-order advancement. Projectile
stepping, the source clock and the completed-frame cleanup retain
their existing entry points. The shortcut uses the live Unit rows
after fresh load and has no persisted state.

## Verification

The normal Terran candidate uses the exact compiler options and 38
prepared inputs of the control. Both keep IWRAM end at `0x030049f8` and
EWRAM end at `0x0203aa0c`. The focused Terran HOST fixture uses the frame
wrapper for paid training and launch. It verifies a queued order waits
on phase 4, a loaded projectile damages its target, and the last
detached child uses its source clock after its Carrier dies.

Both normal ROMs replay the ordinary Terran-versus-Zerg Blood Bath win
from blank SRAM to result at call 27,281, simulation frame 26,692. All
186 observations match. Seven cuts also match both framebuffer pages,
the palette and the shared source clock.

| Terran build | Mean guest cycles per call | Peak guest cycles |
| --- | ---: | ---: |
| Released sparse-routing control | 979,650 | 9,563,141 |
| Carrier control | 1,029,927 | 9,828,247 |
| Carrier with empty-pass shortcut | 1,003,694 | 9,772,120 |

The shortcut reduces mean cycles by 2.55% and recovers 52.18% of the
Carrier composition's added cost in this match. Calls 15,000–19,000
improve by 2.22% and calls 19,000–24,000 by 2.81%. The measured scope
includes presentation waits. The comparison uses emulated cycles.

| Case ID | Required evidence |
| --- | --- |
| `carrier-empty-training-pass` | Replay the ordinary Blood Bath win with no Protoss seat, then compare action, terminal, source-clock and framebuffer observations on both ROMs. |
| `carrier-empty-detached-child` | Destroy the last parent with an Interceptor outside. Verify movement on phase 4 and countdown on a source update. Save and resume a valid detached or active child through the generic clock binding in the applicable profile. |

The normal Terran physical comparison receipt is
`377ff273f1cfe15fad9a51783f5abee354569e52a1a9de5362e371b4bc3f9dd3`.
