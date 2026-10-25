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

The separate seven-mission composition also enables the existing P3
production-exit and nearest-resource-depot options. With the same private
inputs, its host qualification target and ARM build target are:

```sh
make test-sc64-p7-campaign-composition
make protoss-p7-campaign-probe
```

This produces `build/StarCraft-Protoss-P7-Campaign-Probe-GBA.gba`, title
`SC P7 CAMP40`, code `SC7C`. Its host matrix wins P1–P7, tests typed saves
across the catalog, and completes all eighteen full skirmish configurations.
The ARM link passes unchanged reserves (IWRAM end `0x03004a30`, BSS end
`0x0203dcd4`). The composition also passed its own instrumented P7 ARM
completion and fresh-process SRAM continuation, as detailed below.

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

## Save continuation and prior-mission checks

A separate instrumented ARM experiment paused and saved through ordinary
controls at frame 29,862, flushed a 32 KiB SRAM file, and loaded it with the
normal HOME load input in a fresh mGBA process. Mission, hero provenance,
entity census, economy and source bindings matched the observed checkpoint.
The frozen input suffix plus 520 neutral updates reached a fault-free RESULT
win at frame 39,893 with the expected Nexus predicates and all three source
heroes alive. Split telemetry lines and corrected input-page accounting
reported all 1,127 transitions without a scheduler fault.

A separate-process host SRAM export/import check matched its canonical
saved gameplay snapshot hash. The broader ARM hashes differed across load,
so this proves a playable saved-game continuation, not full ARM-state or
uninterrupted-trajectory parity. The private ARM builds stayed within the
unchanged guards (IWRAM end `0x03004a28`, BSS end `0x0203dce4`).

The separate composition profile passed an independent instrumented ARM
run at frame 39,370 with all 1,127 transitions, no scheduler/runtime fault,
the expected hero ownership, and the source victory predicates. Its own
32 KiB SRAM experiment saved at frame 29,862, loaded in a fresh process,
and reached the same source win at frame 39,893 after the frozen suffix
and 520 neutral updates. Checkpoint entity/economy/source telemetry matched;
full ARM-state and uninterrupted-trajectory parity remain unproven. The
instrumented composition stayed within the unchanged reserves (IWRAM end
`0x03004a30`, BSS end `0x0203dce4`). Both private evidence manifests passed
SHA-256 verification.

Host controllers compiled with `P7_TYPED_MOVE_FLAGS` completed P1, P2, P4,
P5 and P6. The same profile completed all eighteen skirmish configurations
(two maps, three human races, one to three rotated AI races): three wins and
fifteen losses, with no incomplete matches. Actual-engine typed save/load
fixtures passed across P1–P7 and all eighteen skirmish setups. These host
checks do not establish physical SRAM behavior or 30-fps performance.

The legacy P3 controller reached its 180,000-frame limit with 38 hostile
buildings remaining. The expansion controller under the same P7 flags also
reached the limit, with 34 hostile buildings remaining. Both are non-wins;
P3 remains an open qualification issue for this development profile. The
P3 expansion and P5 controllers now pass source-owner arguments to optional
P7 targeting diagnostics; P5 also accepts later catalog entries while still
checking its exact mission ordinal and source deployment.

A separate host composition adds the existing `SC64_P3_PRODUCTION_EXIT` and
`SC64_NEAREST_RESOURCE_DEPOT` options to `P7_TYPED_MOVE_FLAGS`. The expansion
controller wins P3 at frame 123,916 with zero hostile buildings. Its entire
6,069-event trace is byte-identical to the established P5 expansion profile's
trace (SHA-256 `321074f1b907846e996c23c5a07cb05687f24ed029b249ab13b25f21528db01a`).
These options are enabled only by the separate composition target above.
Its full host matrix also wins P1, P2 and P4–P7, passes actual-engine typed
save/load fixtures, and completes all eighteen skirmish matches (three wins,
fifteen losses). The P7 trace matches the original typed-move profile exactly;
skirmish timing can change with nearest-depot routing. This establishes an
existing working host composition; its bounded P7 ARM checks above do not
establish complete ARM qualification for all seven missions or skirmishes.

This controller does not promote P7, change canonical menus, or
establish native AI or Carrier combat equivalence. Physical GBA qualification
and mission-wide performance work remain; see [Validation](VALIDATION.md).
