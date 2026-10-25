# Static-art redraw and current-stock P3 qualification

This follows the [canonical rendering audit](../2026-10-26-canonical-rendering/README.md).
The standard Terran and Protoss builds now enable `SC64_STATIC_DIRTY_ROWS`.
Zerg retains its previous path: the sampled trace exercised only five dirty
bands and showed no useful timing gain. Existing campaign/probe flags remain
separate from canonical draw flags.

## Static redraw behavior

Unchanged screen-space culls now run before visibility, actor-fact and sprite
queries. On a partial terrain-cache repaint, the renderer unions dirty tile
rows into at most five disjoint screen bands. Each band replays minerals,
geysers and building art in the original painter order. Untouched rows retain
their restored pixels. Source props, including reflected doors, use the same
clipped drawing primitives.

The optimization requires a valid restored cache, matching current view and
the normal map clip `[12,120)`. Full cache misses, invalid contexts and direct
full-screen callers keep the full static pass. The caller clip is restored
before cache capture and dynamic overlays. Construction, Photon Cannon frames,
unit/vital drawing, projectiles and death effects remain single-pass.

The pure planner uses bounded local storage, validates the view alignment and
avoids signed overflow even for extreme integer inputs. Its diagnostic band
counter is host-only. There are no new production BSS or save fields.

## Pixel and cycle evidence

```sh
make test-sc64-static-dirty-rows test-sc64-static-dirty-rows-game
make test-canonical-render race-campaigns
./dev build
./dev test
make unified test-unified
./dev sc64 test --profile terran
make test-protoss
python3 -m unittest tests.test_sc64_p3_controller_replay
```

The planner test covers all 512 nine-row masks across 16 camera phases. T4
whole-frame fixtures cover overlapping props, disjoint fog changes, viewport
edges, 1/2/3/15/16/17-pixel camera movements, clip restoration and full-jump
fallback. A host counter proves the optimized path ran. Photon Cannon and
construction frames also match a forced fresh oracle during partial reuse.

Normal 9000-update input tapes for all three profiles have identical full-frame
hashes at 18 checkpoints with the feature off/on. The Terran candidate also
matches the pre-cull baseline. The selected ARM state hashes match at all 19
checkpoints, including startup. Hashes are sampled evidence, not every-frame
or every-transient state comparisons.

| ROM profile | Full loop cycles, reference / row candidate | Fps equivalent | Raw drawing cycles, reference / candidate |
| --- | ---: | ---: | ---: |
| Terran | 664783 / 584047 | 25.24 / 28.73 | 395311 / 337185 |
| Zerg | 577264 / 577389 | 29.06 / 29.06 | 253564 / 258080 |
| Protoss | 581599 / 576913 | 28.85 / 29.08 | 360263 / 301221 |

The reference includes the cull reorder and full static passes. The setup is
map 1, human Terran and three AI players, with a separate normal-menu tape per
ROM. Each comparison uses the same tape within its profile. All prefixes end
nonterminal. Timer 2/3 measures CPU cycles; full-loop timing includes audio and
the existing page-flip deadline wait. Raw drawing excludes that wait. Observer
work runs between intervals and can affect cadence. No physical GBA timing is
claimed.

A deliberately fragmented synthetic scene toggles fog cells in four disjoint
rows while the camera and simulation are frozen. All 1999 frames use partial
cache reuse, with 7996 executed bands. Reference/candidate state and pixel
hashes match. Background cycles fall 260581 to 252523, map cycles 287615 to
279561 and raw drawing 373290 to 369513; full-loop cycles are effectively
unchanged at 558408 / 558406. This arrangement did not justify a band-count
fallback threshold; it is not a result for every possible static-art density.

## P3 qualification after stock-stat corrections

The former controller route no longer won after the stock-stat corrections.
The repaired policy uses normal inputs for a protected building layout,
Probe/production growth, cannons and a committed assault. It limits active
Probe build orders rather than counting orphaned unfinished blueprints as
permanently busy builders. Producer selection uses the actual training choice,
and Photon Cannon availability is checked after selecting a Probe.

The deadline remains 180000 frames. Success now explicitly requires no loss,
the result screen, a valid native model in DONE_WIN, completed campaign phase,
the primary objective and zero hostile buildings. The host controller and a
fresh input-only replay both win at host tick 58063 / native frame 57457.

The public P3 replay helper now defaults to the `stock` route and
`.local/sc64/p3-controller-stock-asan-win.json`, produced by `make test-protoss`.
It authenticates the seven menu-launch transitions, launch state, 2985 input
transitions, 57 live checkpoints, terminal source fields and window capacity.
The historical garrison/expansion routes remain explicitly selectable.

A private ARM driver includes the current engine and production Protoss flags,
replays only the recorded buttons from a fresh initialization, and renders each
update. It performs no controller planning queries or world-state injection.
It matches the native launch at tick 7 and terminal at tick 58063: WON,
sequence DONE_WIN, fired mask 31, valid model, completed phase, primary objective,
no hostile buildings, 22 side units and four buildings. This is an instrumented
driver, separate from unmodified-ROM debugger and fresh-SRAM qualification.

The complete P3 route averages 306338 update cycles, 459755 raw drawing cycles
and **839963 full-loop cycles (about 20.0 fps equivalent)**. The full-loop
maximum is 26965284 cycles, and 42210 of 58063 updates exceed the two-VBlank
561792-cycle budget. The full-mission 30-fps target remains unmet; the faster
opening skirmish sample does not supersede this evidence.

## Boundaries and remaining work

The canonical IWRAM and BSS guards remain unchanged; Protoss still has only
56 IWRAM bytes to spare. No stats, opponents, unit caps, objective conditions,
mission deadlines or simulation frequency were reduced to improve performance.
Remaining campaign stages and full-mission/stress performance still need work.

Private evidence is under `.local/build/static-row-perf/`,
`.local/build/static-rows-pixel-parity/`, `.local/build/static-row-worstcase/`,
`.local/build/p3-stock-requalification/` and `.local/build/p3-stock-arm-replay/`.
No ROMs, extracted art, tapes, screenshots or private manifests are published.
All validation runs locally; GitHub Actions remain disabled/manual-only.
