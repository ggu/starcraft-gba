# Canonical race rendering qualification

The three standard race ROMs now use qualified bitmap drawing options through
`TERRAN_ROM_FLAGS`, `ZERG_ROM_FLAGS` and `PROTOSS_ROM_FLAGS`. Their underlying
campaign/probe flag sets remain separate. The original-art default and
experimental campaign probes retain their existing configurations.

## Changes

- `GBA_SPRITE_WORDS` copies four opaque pixels with aligned source/destination
  word accesses, skips all-zero groups and retains the original pixel-pair
  compositor for mixed groups. Odd alignment, clipping, widths and source
  strides remain supported without unaligned ARM7 loads or source overreads.
- `GBA_WINDOW_BANDS` merges a pure vertical scroll of a 240-byte-stride cache
  into one contiguous transfer. Out-of-source rows remain untouched. Terran
  and Protoss use it; Zerg's canonical cache does not scroll, so it omits this
  option.
- `GBA_SCENE_CLIP` limits map drawing primitives to rows 12 through 119.
  Drawing returns to the full screen before HUD, pause and result overlays.
  Explicit validated band/window copies remain independent of the scene clip.
- `SC64_HUD_BUFFER_REUSE` extends independent backbuffer HUD reuse to Zerg and
  Protoss. Without scene clipping, the HUD always restores both cached bands.
  This also fixes the older Terran path when used without the new draw flags.

Whole-frame regression exposed an existing Terran bug: a Hatchery near the
viewport edge painted palette index 105 into resource-panel pixel `(36,0)`;
the correct forced-redraw value was 3. Retaining a clean HUD page could leave
that sprite pixel visible. Scene clipping prevents map primitives overwriting
cached HUD bands; the un-clipped fallback repairs them each frame instead.

Clipping uses the scene bounds directly rather than repeating screen-bound
checks. Number formatting executes from ROM with clipping enabled; glyph
drawing and bitmap loops remain in IWRAM. No gameplay stats, objectives,
resources, saves or campaign deadlines are changed by these render options.

## Pixel and state checks

```sh
make test-gfx-sprite test-gfx-window test-gfx-clip
make test-canonical-render
make race-campaigns
./dev build
./dev test
make unified test-unified
```

The asset-free blitter fixtures compare all 38400 framebuffer pixels against
independent copy/transparency oracles, covering edge clipping, source offsets
0 through 3, odd/even widths and placement, masks, sizes through 256, seeded
random cases and invalid arguments. Clip tests compare clipped drawing with
a cropped full-render oracle, including empty ranges and begin/reset behavior.
All variants run under ASan/UBSan.

Canonical coverage compares complete output against an un-clipped map followed
by opaque, freshly drawn HUD bands. That independent reference intentionally
overdraws invisible map pixels. It covers poisoned pages, scene transitions,
construction, fog/camera mutation, restart/save/load and consecutive edge
sprites. The HUD fixture also warms both pages and moves edge sprites before
comparing with a forced HUD redraw. It reproduced the old corruption and
passes with both the repaired fallback and scene clipping.

The default suite passed 463 Python tests and the host fixtures. Canonical
frame/clip/blitter fixtures and ARM ROM verification passed. Broader campaign
controller qualification remains distinct from these rendering checks.

The current canonical startup/save binaries also passed all 234 explicit
map/human/opponent-race configurations per ROM (702 total). The normal-input
menu fixtures passed 18 rotated-opponent setups per ROM (54 total). The former
sets frontend choices directly; it is not 234 independent menu-navigation or
fresh-process hardware save/load runs.

All ten Terran normal-input mission controllers passed again. Stock values
exposed an older Z3 controller strategy failure: it attacked distant structures
while the Chrysalis was under attack. The revised ordinary-input route defends
nearby enemy units first and waits for a larger army before expeditions. It
reaches the unchanged native win at frame 49424 with the Chrysalis alive at
165 HP. No objective, deadline, resource or strict result assertion changed.
The old P3 controller also fails against the current stock values; its route
requires separate requalification rather than treating an older win as proof.

## ARM timing

Each ROM uses a fresh normal-menu input tape for map 1, human Terran and three
AIs (Zerg, Protoss, Terran), measured over its first 9000 updates. Comparisons
within each profile use the same tape. The setup is shared across profiles;
the different profiles do not necessarily produce identical simulation states.
All runs remain nonterminal at the end of the measured prefix.

| ROM profile | Previous mean loop cycles | Updated mean loop cycles | Previous / updated fps equivalent | Previous / updated raw drawing cycles |
| --- | ---: | ---: | ---: | ---: |
| Terran | 748992 | 670654 | 22.40 / 25.02 | 429616 / 400056 |
| Zerg | 579355 | 577389 | 28.96 / 29.06 | 278261 / 254710 |
| Protoss | 649294 | 582942 | 25.84 / 28.78 | 408700 / 364461 |

Timer 2/3 measures update, audio and render. Total loop cycles include the
existing page-flip/VBlank deadline wait; `gfx_draw_cycles` excludes that wait.
FPS equivalent is 16777216 divided by mean loop cycles, rather than desktop
wall-clock throughput. Timers and selected-state hashes are emitted outside
the measured interval; observer work can affect cadence between intervals.

All 19 checkpoints per profile, including startup, match the old selected
gameplay-state hashes and state summaries. This verifies those shared fields
at 500-update checkpoints, not every transient, every update or physical
hardware timing. The old reference had the HUD overdraw bug; pixel correctness
is established separately against the full-redraw oracle.

## Memory and remaining work

| Canonical ROM | IWRAM end | BSS end |
| --- | --- | --- |
| Terran | `0x03004f10` | `0x0203ddd4` |
| Zerg | `0x03004f38` | `0x02039c58` |
| Protoss | `0x03004fc8` | `0x02039e00` |

All links retain the strict IWRAM limit below `0x03005000`, BSS below
`0x0203e000`, and existing save-size guards. Protoss has only 56 IWRAM bytes
before that guard; future changes must preserve the reserve. No guard changed.

The target is still unfinished: Terran remains below 30 fps in this sample,
and the measurements do not qualify all human races, maps, opponent counts,
dense late-game battles or entire campaigns. A separate private breakdown
identifies repeated resource/building-art redraws during partial terrain-cache
scrolls as the largest remaining background cost. Reducing that work must
preserve exact layers, fog and dirty-region behavior.

Private source snapshots, instrumentation manifests, tapes, ROMs and logs are
under `.local/build/render-candidates/`, `.local/build/render-stage-audit/` and
`.local/build/render-background-audit/`. They are not published. All tests ran
locally; no GitHub Actions were enabled or dispatched.
