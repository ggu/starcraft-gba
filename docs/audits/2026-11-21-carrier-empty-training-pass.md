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
includes presentation waits. The optimized build still averages 2.45%
more guest cycles than the released sparse-routing control in this
match. The comparison uses emulated cycles.

The optimized Terran ROM `455b9e1c1b74a5b8cfe5491d861a6b01b904a6c10270136eb3068c43ae576bf8`
also repeats 25 observations in the ordinary human Outpost run, including
the active layer-5 pulse at frame 16,100 and the docked child at frame
16,140. It matches the fresh active and docked payload re-saves. In the
three-AI Blood Bath run it repeats both computer purchase debits, the
live arena at frame 17,335 and all 12 sampled world cuts after fresh
load. Each build independently confirms 192 matching arena updates
between uninterrupted and fresh continuation. All six paired physical
SRAM images are byte-identical.

The mixed match averages 876,475 versus 876,311 cycles in the paired
pre-save window and 843,909 versus 845,744 during its uninterrupted
continuation. The measured stack watermark matches at 6,812 system bytes
and 120 IRQ bytes above their boundaries.

| Case ID | Required evidence |
| --- | --- |
| `carrier-empty-training-pass` | Replay the ordinary Blood Bath win with no Protoss seat, then compare action, terminal, source-clock and framebuffer observations on both ROMs. |
| `carrier-empty-detached-child` | Destroy the last parent with an Interceptor outside. Verify movement on phase 4 and countdown on a source update. Save and resume a valid detached or active child through the generic clock binding in the applicable profile. |

The normal Terran physical comparison receipt is
`377ff273f1cfe15fad9a51783f5abee354569e52a1a9de5362e371b4bc3f9dd3`.
The paired active Carrier and journal receipt is
`7edefdde11e34005adec58cc2efa5a048ddde750a9b68ee6fabe0d0ac7122690`.

## Shared cartridge qualification

The combined runtime tree `23a813fd` links all four normal builds. Z10,
P10 and Showcase use their preceding compiler arguments and prepared
inputs and have the same RAM endpoints. The normal Showcase ROM is
byte-identical because its build excludes Carrier flight. The Z10 actor
facts also remain byte-identical and pass all 2,019 comparisons.

| Product | Optimized ROM SHA-256 | IWRAM end | EWRAM end |
| --- | --- | --- | --- |
| Terran | `455b9e1c1b74a5b8cfe5491d861a6b01b904a6c10270136eb3068c43ae576bf8` | `0x030049f8` | `0x0203aa0c` |
| Z10 | `86324a99034096e9448d2bf486eedb949a0681d15688cc6db7112bfc26430bf0` | `0x03004d60` | `0x02039b30` |
| P10 | `91af2b848c2f57cd24c5d84744d1167e178189fd272a131eb39b33a473a0b7a7` | `0x03004e20` | `0x0203b62c` |
| Showcase | `a2ea4dcbd92f8050b41f76b739a7bc1786e126aa7058b9f3b1158f9d67c59fe3` | `0x03004fb8` | `0x02039168` |

Eight paired ARM checks match state and SRAM across both Custom Game
maps on Z10, P10 and Showcase and the Z1 and P1 campaign openings.
Z10 writes the same 32 KiB journal at frame 17,305 with an active pulse.
The next 180 ticks match the world, projectile arena and both display
pages after a fresh HOME load. Guest cycle cost rises about 0.20% in
that window. P10 writes the same active-shot journal at frame 37,589
and matches 36 ticks after fresh load through damage and dissipation,
including the palette and both pages.

The cross-profile comparison receipt SHA-256 is
`60b0a23e81a58d4d181bb27947d1634740829362b38dbffa674d0fe5d3a0b56d`.

The three exact race ROMs also repeat the
[1,134-launch Custom Game matrix](2026-11-21-current-skirmish-matrix.md#three-race-roms-with-the-optimized-carrier-service).
Each of the 54 sampled journals matches its continued world and seat
assignment after fresh HOME L. The
[Random-row matrix](2026-11-21-current-skirmish-matrix.md#independent-random-rows)
adds 1,098 starts and 54 independent fresh-save checks.

In the exact optimized Terran ROM, the normal T10 campaign wins at
frame 3,361. The pause at frame 2,276, fresh load, all six named
checkpoints and the full victory journal match the stock sparse-route
Terran. The immediate third-launch victory readback also matches all
32 KiB. The optimized replay averages 1,038,845 guest cycles per
main-loop call against 1,026,885 on stock. Its receipt SHA-256 is
`ed5653ce83c35d5b9b8d042c8ccbf96a477eac4412e712bf2287ea8e9248d89a`.

Normal P10 completes Eye of the Storm on both the sparse-route control
and the optimized ROM. The same 47,266 ordinary button calls reproduce
the briefing and dialogue, all 20 named checkpoints, the fresh pause
load at frame 36,761, and victory at 46,129 with all three heroes at
full HP. The pause, result and fresh result readback match all three
SRAM files byte for byte. The frozen victory persists for another
eight calls. Mean cycle cost increases 0.30% and peak cost is unchanged
to two decimal places. The private paired receipt SHA-256 is
`71135b730e268334c4e4acbd81d8ce542f89e9ae1ffd9f6ca4db565b1f985aa1`.

The optimized Z10 also wins its finale at frame 50,193 from HOME
and from fresh HOME L after pausing at frame 2,526. It matches all
22 HOME and 20 fresh named checkpoints and world observations, the
dialogue cuts and the crystal Drone transfers. The 32 KiB pause image
matches the sparse-route Z10 byte for byte. The whole HOME tape
averages 12.08 emulated main-loop calls per second on either ROM.
The private paired receipt SHA-256 is
`3e6b112ef02a429f08d5823b260a5b456e0d79ed2e8f51a8b43dcc6fe2a1300a`.
