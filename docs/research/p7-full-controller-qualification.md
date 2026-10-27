# P7 full-controller replay qualification

This note records two related but separate results: a host run of the repaired
ordinary-input P7 controller, and a fixed-input full-profile replay that was
compared on host and ARM. The ARM result qualifies the frozen input tape; it
does not run the controller policy on ARM.

## Repaired controller host run

The private controller candidate drives the game through ordinary held and
pressed inputs. It chooses reachable Nexus objectives by route length, locks
the source force, retries a stalled target within that force, refreshes orders
every 480 frames, and adjusts Gateway and production cadence. It changes only
the test controller. `starcraft/game.c` is unchanged, and the strict campaign
win predicate is byte-for-byte identical to the base test.

The host candidate completed mission 26 at frame 39,738 with RESULT, `won=1`,
`lost=0`, all three heroes alive, source outcome WON, no runtime fault, the
victory switch set, the victory trigger started at cursor zero, all action
timers clear, and the Force 3 Nexus source condition true. Its 1,667 input
transitions pass the offline held/pressed edge validator. This controller
result is host-only.

## Published controller check

The optional `make test-sc64-p7-campaign-controller` target compiles the
controller with `P7_SIGHT_AXIS_FLAGS` and runs the offline trace validator.
The final census records `total_calls`, including the neutral tail after the
last input transition. The public logger also identifies this build profile
and labels its route policy. A fresh ASan/UBSan run of this version reached
the same strict victory and produced exactly the same 1,667 input records as
the frozen trace below; only diagnostic metadata differs.

Its trace SHA-256 is `4787ac4b010dfe019c266b8d8b051801fe65320e12db4a9117bc94fd8a57958e`.
This trace is private at `.local/build/p7-controller-public-20261027-v2/`.
The asset-free validator tests use synthetic traces and require no game data.

## Frozen full-profile input replay

The same 1,667 held-input transitions were frozen as a fixed tape and replayed
from a fresh host and ARM initialization. Each tick applies the held buttons
to `game_update`, with `pressed = held & ~previous_held`, then calls
`audio_update` and `game_render`. The replay has no adaptive controller or world-state writes; expected
state is checked without changing the input sequence. The tape entries exactly match
`(call_index + 1, held)` from the trace, and every recorded `pressed` value
matches the edge formula.

Host and ARM state records match at all 67 checkpoints after normalizing the
host and ARM field labels. Both the 32-bit and 64-bit digests match at every
checkpoint. The replay ends at call 39,976 and game frame 39,738 with RESULT,
`won=1`, `lost=0`, source outcome WON, no runtime fault, and all three heroes
alive.

The strict terminal predicate also checks that both runtime and projectile
faults are clear, the victory switch is set, the four defeat-trigger bits are
clear, the victory trigger started with cursor zero, all action timers are
clear, and at least one Force 2 or Force 3 source Nexus condition is true.
This guards against treating the menu win alone as proof of the campaign
objective.

The ARM build uses the frozen full P7 profile, including projectiles, Carrier
training, interceptor flight, and `SC64_P7_SIGHT_AXIS_CULL`. Its 228-file
source manifest has no missing files or hash mismatches. The postlinked ROM is
32 MiB, has the expected Nintendo logo and valid header checksum, and is
identified as `SC P7 FIXED` / `SP7F`. Its unpadded linked image is 20,182,844
bytes; BSS ends at `0x0203d794` and IWRAM at `0x03004ae8`. Measured stack
high-water use is 5,012 bytes, leaving a 6,764-byte margin above the reserved
stack boundary.

## Bounded SRAM continuation

A separate two-process ARM replay qualified one normal pause-menu save/load
continuation on the same frozen P7 input tape. The prefix process saved at
frame 28,000 after 1,174 input transitions; a fresh process loaded from the
title screen and replayed the remaining inputs to the same strict victory.
The 32 KiB SRAM image contained a valid 16,292-byte payload in slot offset 0,
sequence 1, after the prefix. After resume, that record remained intact and a
second valid 16,292-byte payload appeared at offset 16,384, sequence 2. Both
records' FNV-1a checksums validated (`0x46c08ddd` and `0x104a10eb`). Their
payload SHA-256 values were
`b401174ce8531e6cb48b016afd49f1274a173d1a4d4652db0950e0a9205e5d43` and
`42221acf76b607845857dbc4fcf344c27ff98f696d8209ebca7e87091d3d4114`.

The persistent gameplay projection matched the host reference at the cut. It
covers actors, buildings, resources, explored fog, P7 model/clocks/wallets,
invincibility, numeric source bindings and the projectile arena. Derived
visibility/camera, load-reset UI/toast state and callback addresses are
excluded; source outputs are checked separately. The save intentionally
does not preserve the UI's selected-building field: selection 4 at save loaded
as -1. That reset was recorded explicitly; all 493 subsequent input/UI-event
records, all 31 tail gameplay projections, all 31 tail UI projections, and all
31 tail source-output projections
matched the host reference. The prefix produced 1,321 host-corresponding
records and the resume 591, excluding each run's separate stack record. The
resume ended at call 39,976/frame 39,738 with the strict win predicate, all
three heroes alive, and no runtime or projectile fault.

The two postlinked 32 MiB ROMs had valid headers and checksums. Prefix ROM
SHA-256 is
`df7237e76016160871c4541c7b7b99f29d2ef1a840ac8ca8a00f6d01d7a135a1`; its
linked image is 20,216,212 bytes, BSS ends at `0x0203d794`, and IWRAM ends at
`0x03004ae8`. Resume ROM SHA-256 is
`eccb64a7515a8da7d039cd7913be8bd28ee749ccc5c0df3640ccc0ac1cbc89d5`; its
linked image is 20,176,348 bytes, BSS ends at `0x0203d784`, and IWRAM ends at
`0x03004ae8`. Stack high-water use was 5,036 bytes for prefix (6,740-byte
margin) and 4,128 bytes for resume (7,648-byte margin).

This qualifies one bounded save/load path under the emulator and this frozen
tape. It does not establish behavior for arbitrary save states or physical
GBA hardware.

## Evidence identities

| Artifact | SHA-256 |
| --- | --- |
| Repaired controller source | `684573634a54f036a986f2aaf26891e584c15a8937900785325e1382eab91bb6` |
| Controller patch | `ae0b35b333609d6231f51a8fdac7f39851b6dae068ce2dd6075447bd2c421f68` |
| Base controller test | `1d53d8bf68236184a0f368a6173fed5a64f484ebcb68d29fad44e7780c4f645f` |
| Frozen `game.c` | `3d0a26b9da4a5efd8abb2897c7ba993d7e9dbb8fcbf210fafd951779ff96b4b8` |
| Controller input trace | `7ef4a1866ac1ee1b8d14b39105f3b3ef8faebde95717ecba44eed7a8ec14ee4f` |
| Fixed input tape | `13fa95a1e8d34275e66ee75b9fbd7c4fc0adc469b09680522c59a48fe3c7136a` |
| Full host replay log | `047f3e093b3a6b9964acda96ff9f39d7fab3e8228e5295dc851d7546bac20b4e` |
| ARM replay harness | `25f6fbd65e3b179b7080fef819315ada9f6b12dc846321753cf774e388d56218` |
| ARM checkpoint goldens | `69f9df1b0e52360151692eae58f9dfe32275e45e6871eb78a65652c0bcef8b6a` |
| Fixed ARM ROM | `a3fc77ca78bbbf71468671d778555aa7e77e5af5242a70f6fdf30ac453d2e933` |
| ARM result record | `e7bf461d356d857edaeb710e3690f6bd5a928713b8c305a67af6f2df6e8f6983` |
| SRAM continuation harness | `9777ef76fc4db3b7a59d1847918e245ee580ab6904f198adb236f039c498a799` |
| SRAM continuation input tape | `13fa95a1e8d34275e66ee75b9fbd7c4fc0adc469b09680522c59a48fe3c7136a` |
| SRAM continuation source manifest | `fc6238e2ed43bcbcdd195fbbb82fa5bdedc3bbe23c49432c1d7bc81d2f9888fe` |
| SRAM continuation prefix ROM | `df7237e76016160871c4541c7b7b99f29d2ef1a840ac8ca8a00f6d01d7a135a1` |
| SRAM continuation resume ROM | `eccb64a7515a8da7d039cd7913be8bd28ee749ccc5c0df3640ccc0ac1cbc89d5` |
| SRAM continuation result | `7e1e2395b7ce05a3b7fefcdd396ef515c715badb800e11c6ada7ae3692b0137a` |

The controller artifacts are under
`.local/build/p7-controller-repair-qualification-20261027/private/`. The
fixed-tape host and ARM artifacts are under
`.local/build/p7-current-fixed-tape-qualification-20261027/`. The bounded
SRAM continuation artifacts are under
`.local/build/p7-sram-continuation-20261027/root-arm-v1/`.

## Limits

Canonical race progression remains **22/30**. The bounded SRAM result does
not qualify arbitrary save/load states or physical GBA behavior. Neither
replay qualifies performance or 30-fps operation, or native enemy Carrier AI.
These results apply to the frozen P7 profile and input tape.
