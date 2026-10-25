# Homeland ordinary-input controller

This isolated controller exercises the existing generation-40 P7 typed-move
probe. It launches from HOME, uses ordinary menu and gameplay held/pressed
inputs, grows an economy and army, and attacks the required enemy force.
It reads live state and the engine's current walkability plane for steering;
it does not write actors, orders, resources or mission state directly.

With the private P7 inputs prepared as described in
[Development](DEVELOPMENT.md), run:

```sh
make test-sc64-p7-controller
make protoss-p7-typed-move-probe
```

The host target uses `P7_TYPED_MOVE_FLAGS` under ASan/UBSan. Its private JSONL
trace is `.local/build/p7-controller/controller-trace.jsonl`. Sparse input
transitions use zero-based `call_index` values after `game_init`; the trace
also records frame values before/after each input transition, production,
commands, source trigger state and the final source actor census.

The matching ARM ROM is `build/StarCraft-Protoss-P7-Typed-Move-Probe-GBA.gba`,
title `SC P7 MOVE40`, code `SC7M`.
Canonical race profiles and their mission menus are unchanged.

## Completion evidence on 2026-10-25

On staged base `fad7926`, an ordinary-input run reached RESULT at game frame
39,370 with `won=1`, `lost=0`, source outcome WON, no runtime fault and the
victory switch set. Source owner 4's Nexus count was zero; source owner 0
retained its two Nexuses. Source condition 8 was true and condition 9 false.
This satisfies the source's alternative force predicates;
it does not require clearing both forces. The three source heroes (raw
75, 78 and 79) remained alive in their rescuable source force. No rescue
to player control is claimed by that census.

The policy buys Probes, assigns mineral/gas workers through ordinary inputs,
adds one Gateway, produces Dragoons, and stages before attacking the weaker
force. Controller-side terrain waypoints keep the attack progressing toward
its Nexus. The exact winning executable, source snapshot and input trace
are preserved privately; generated inputs and traces are excluded from Git.

An instrumented ARM replay of the frozen ordinary-input tape also reached
RESULT at frame 39,370. It reported the same hero ownership/alive census,
Nexus counts, source conditions, resources (8,228 minerals and 832 gas) and
supply (124/178), with source and input-scheduler faults both zero. The
probe uses the normal update, render and frame-deadline loop, with private
ROM-resident input paging and read-only telemetry. It does not use a debugger
to write game state.

The observed interval from call 12 through call 39,546 consumed
107,832,567,192 GBA cycles: about 6.15 rendered updates per second at
16,777,216 Hz. Telemetry and input paging are private instrumentation, so
this is a performance warning for the development probe, not a standard-ROM
or physical-hardware benchmark. It is far below the 30-fps target.

The terminal telemetry line was truncated after the victory timer, and its
transition index double-counted the final page. Neither field establishes
full-state hash or transition-count parity. The visible source verdict and
ending census support the bounded ARM completion result above.

Fresh-process SRAM continuation and physical GBA qualification remain
separate work. This controller does not promote P7, change canonical menus,
or establish native AI or Carrier combat equivalence; see
[Validation](VALIDATION.md).
