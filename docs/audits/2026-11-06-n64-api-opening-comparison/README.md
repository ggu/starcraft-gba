# N64 callback-runner opening comparison

The owned normalized USA ROM was booted through official ares v148 source
`0aafd85789215e84e1e43415c07d4c88461b7899` with ordinary emulated gamepad
buttons/axes and paraLLEl-RDP. No OS keyboard shim, guest memory access,
savestate injection or edited progress was used. This is emulator evidence,
not physical N64 timing or a matching decompilation.

The source-only harness and its run-specific integrity manifests are described
in the [callback feasibility and run audit](https://github.com/ggu/starcraft-gba/blob/codex/shared-actor-stack-qualification-nov6/docs/audits/2026-11-06-ares-headless-feasibility/README.md).
ROM SHA-256: `e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`.
Frames, audio, saves, ROMs and traces stay private; this audit includes none.

## Direct observations and current source agreement

GBA source reviewed is the merged UI tree corresponding to canonical
`31a5ddf8d3734427ebfef151acfb683d0db54a03`. This comparison checks authored
source values against new N64 observations; it does not replace fresh GBA
runtime qualification of the same scene.

| Item | N64 observation | GBA source | Scope |
| --- | --- | --- | --- |
| Wasteland opening minerals | 40 visible on map | `race_terran_tick()` grants 40 | Value agrees; native grant timing was not measured. |
| Marine HP | Selecting Marine displays 40 | `race_rules.h` and stock raw 0 profile specify 40 | Source agreement; no damage/cooldown test. |
| SCV HP | Selecting SCV displays 60 | `race_rules.h` and stock raw 7 profile specify 60 | Source agreement; no repair test. |
| Supply Depot price | SCV build grid highlights cost 100 | `race_rules.h` B_DEPOT mineral cost is 100 | Source agreement; no paid construction/birth test. |
| Opening supply | HUD displays 10/18 | GBA HUD divides internal doubled supply by two | Numeric native baseline recorded; matching GBA starting census still needs same-scene verification. |
| Build grid | Three populated slots visible in Wasteland | GBA has mission-specific permissions | Slot identities beyond the highlighted Depot and custom availability are not inferred. |

Root independently inspected the native opening-map image and the SCV
build-grid image. Local provenance: `advance-1/preview.png`,
`select2-1/preview-24.png`, `select4-1/preview-24.png`, and
`buildmenu-1/preview-26.png` beneath the isolated harness's ignored audit
folder. The callback output is 640x240; frontend display aspect/crop remains
uncalibrated. Sparse samples cannot establish all-frame renderer correctness.
A transient corrupted capture in another attempted selection is retained and
must not be reclassified as a game-state change or silently discarded.

## Next checks

1. Reach Setup Custom through ordinary controls; verify unrestricted race
   build menus separately from Wasteland's mission permissions.
2. Observe paid worker training and construction, including a Refinery and
   gas collection, and record resource changes without equating VI callbacks
   or controller polls with internal simulation steps.
3. Capture Protoss warp-in and Zerg morph sequences, real weapons/impacts and
   fog boundaries for comparisons to the corresponding current GBA products.
4. Keep script/source-table claims, direct runtime observations and timing
   hypotheses separate. None of these opening checks closes campaign wins,
   AI parity, all actors or 30-fps performance.
