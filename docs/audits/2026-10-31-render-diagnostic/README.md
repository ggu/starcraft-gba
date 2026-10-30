# Opt-in render diagnostic

Compile with `SC64_DIAGNOSTIC_TRACER_PROFILE` to measure imported rendering. Normal product flags do not set it. The diagnostic probe substitutes its own main loop and an external, private `tape.h`; it is not a production build or hardware FPS measurement. Keep private inputs, tapes, outputs, screenshots and ROM data outside Git.

On shared source 4049136, the normal Zerg/three-AI tape and ordered compiler vector from the shared-shape audit were used. All 90 sampled actor/fog/display/frame checkpoints matched the earlier shape-candidate run; this is sampled parity, not full state equivalence. The last 1,000 calls recorded 285.04M map cycles, including 122.64M background and 110.03M unit drawing. Tracers accounted for 74.21M in that nested measurement; lookup 9.11M, culling 6.00M, and resolved source-trace execution 48.54M are subscopes. The latter includes motion 10.37M, actor animation/overlays 12.75M, projectile setup/motion/frame drawing 16.85M, and child overlays 7.11M. Early returns and per-event timer overhead mean these pieces are neither complete nor additive. Instrumentation changes timing, so these numbers identify candidate work, not a production speedup or hardware frame rate.

`sc64_map_cycles` starts before scene clipping and `draw_map` and ends after clip restoration. Background, building and unit-draw clocks are inside `draw_map`; tracer and static-key clocks are further nested. `gfx_end` is called after map and HUD clocks stop. Under the production `GBA_PRESENT_DEADLINE` vector, it polls `gba_present_ready` and VCOUNT before flipping the display; this wait is included in the outer update/audio/render loop total, not in the map or tracer timings. Other configurations use vblank waits instead. An older call profiler's `game_render` inclusive timing includes `gfx_end` and cannot be interpreted entirely as useful drawing work.

## Reproduce with prepared inputs

Use a source checkout at the reviewed base with the same prepared generated input directories used by the original compiler arguments. Supply an external JSON array containing the exact normal-product compiler argv, and a private directory containing `tape.h` in the probe's `{t,h}` format. The tape must have ordinary held inputs; do not inject world state. Use a fresh output path.

```sh
python3 tools/sc64_render_profile_build.py /private/normal-argv.json /private/new-build /private/tape-directory
python3 tools/sc64_render_profile_read.py --runner /private/sc64-headless-replay --rom /private/new-build/profile.gba --elf /private/new-build/profile.elf --out /private/new-result.json
```

The first command preserves the supplied compiler features and adds the shared shape flag and diagnostic flag; it compiles a diagnostic-only main and writes ELF, ROM, map, and exact link arguments to the new private directory. The second uses the port-free in-process mGBA transport with an in-process breakpoint at the probe's completion marker. It performs no guest-memory writes and emits counters and sampled checkpoints only to the requested new private file. Build the runner against matching static mGBA following `docs/research/sc64-headless-skirmish-replay.md`. Compare the private result against a separately retained baseline; the script makes no parity or performance claim. The test tape hash was `0d79706afb77e58c958a6dd622a56843f611f4400bfa014bdbeb7db751f6ef2b`.
