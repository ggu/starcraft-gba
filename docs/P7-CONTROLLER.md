# Homeland ordinary-input host controller

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
title `SC P7 MOVE40`, code `SC7M`. Building it does not establish an ARM win.
Canonical race profiles and their mission menus are unchanged.

## Host evidence on 2026-10-25

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

This is a host proof for a development profile. Full normal-input ARM replay,
fresh-process SRAM continuation, mission-wide performance and physical GBA
qualification remain separate work. It does not promote P7 or establish
native AI or Carrier combat equivalence; see [Validation](VALIDATION.md).
