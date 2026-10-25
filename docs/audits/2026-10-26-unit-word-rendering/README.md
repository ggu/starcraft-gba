# Unit word rendering on canonical Zerg

The unit renderer can use four prealigned phases of the same final indexed
sprite pixels. Each four-pixel group stores a color word and an alpha-byte
mask. Transparent color lanes are zero; empty groups skip writes, opaque groups
use one word store and partial groups merge with the existing framebuffer.
Screen X boundaries are word-aligned, and frame-edge padding is transparent,
so the inner loop needs no repeated edge-mask calculation. Scene Y clipping
and source crop offsets retain the indexed blitter's output.

The optional data lives alongside existing `Sc64SpriteFrame` objects. That
ABI, indexed pixels and legacy paths are unchanged. The generated accessor
requires the exact frame pointer and raw ID below 106. Buildings, death frames,
unmapped frames and older headers keep the indexed path. Optional actor groups
retain their existing feature guards and first-mapping precedence. Both
imported raw identities and ordinary trained units (`raw_type == -1`) resolve
the effective raw identity before looking up metadata.

`tools/sc64_race_sprites.py --prepack-unit-words` generates the sidecars. They
are private game-derived data and remain ignored. Canonical Zerg preparation
requests the option and checks the metadata marker, accessor, descriptors and
payload; stale prepared headers require regeneration. Other profiles retain
their previous default flags and preparation requirements.

`SC64_UNIT_WORD_SPRITES` is separate from the existing `GBA_SPRITE_WORDS`
opaque-quad optimization. A comparison with `ZERG_UNIT_WORD_FLAGS=` retains
the indexed blitter's quad optimization and all other canonical tuning.

## Paired full-mission evidence

The full Z4 reference/candidate runs use the same current engine, private
sprite bank and 812-input controller sequence. Host state and whole-frame
hashes match at all 120 recorded 500-tick checkpoints and the terminal result.
The state fingerprint covers actor/resource banks, map/path grids, mission
state, tracers/death effects, RNG, HUD cache state and framebuffer pixels.
Both runs win at input tick 59260 / game frame 59101, with no source faults,
unsupported operations or input errors. Independent ARM input replays reach
the same terminal fields.

Private counters prove the optimized path runs: 793507 packed draws, including
629411 derived-raw draws from ordinarily spawned units and 164124 explicit-raw
draws. The initial prototype only accelerated explicit imports; it was corrected
and requalified before promotion. A kernel-only 95-frame microbenchmark is
separate evidence and must not be reported as the campaign speedup.

| Complete Z4 replay | Indexed reference | Unit word path |
| --- | ---: | ---: |
| Update cycles/tick | 295134 | 295174 |
| Raw draw cycles/tick | 570590 | 408135 |
| Full-loop cycles/tick | 947628 | 787711 |
| Full-loop fps equivalent | 17.70 | 21.30 |

The promoted combination also enables the separately qualified collision fact
hoist and campaign fact range cull. Against word rendering alone, all twelve
windows preserve state and input fields while draw/full means improve again:
update 295163 to 293931, draw 408135 to 407283, full-loop 787702 to 785479
cycles/tick (about 21.36 fps equivalent). The largest full-loop sample differs
by only 90 cycles; route-build hitches remain. The extra query code uses 48
IWRAM bytes, with no additional actor/save state.

The word-only candidate reduces raw rendering by 28.47% and full-loop mean by
16.88%. Every one of twelve measured draw windows improves (7.1–40.5%). Update
work is effectively unchanged. These are tick-weighted Timer-2/3 measurements
in instrumented mGBA drivers, with audio and presentation included in the full
loop. They are not physical-GBA measurements or a claim that 30 fps is reached.

The active Z4 candidate adds about 328460 bytes of ROM text, with 47 compiled
frame descriptors. The fixed output still fits the 32-MiB ROM limit. Diagnostic
instrumentation gives IWRAM end `0x03004e50` and BSS end `0x0203bbc8`; production
links end at `0x03004e70` / `0x0203c410` with the combined flags. No actor
limits, game-update cadence, source
statistics, mission scripts, save fields or reserve assertions are changed.

## Local checks

`make test-gfx-sprite-packed` compares whole framebuffers with the indexed
blitter under ASan/UBSan, with clipping enabled and disabled. The suite covers
transparent/sparse/opaque/random pixels, all alignment phases, negative and
edge coordinates, dimensions through 256 and extreme integer coordinates.
Generator tests check opt-in output, exact-frame/raw bounds, zero transparent
color lanes and legacy-header behavior. Dev tests reject stale or disabled
metadata and verify the canonical preparation command.

Final local checks passed: default build/test (487 Python tests plus host
fixtures), unified, canonical render/HUD, and `make test-race-campaigns`.
The latter retains all four Zerg and all three Protoss controller wins,
54 menu launches and 702 startup/save configurations across the three ROMs.
Explicit indexed scalar/quad and packed clip/no-clip framebuffer checks passed;
the existing scalar/quad target remains separate from the new packed target.
The spawned/explicit-raw game fixture also passes against the standard prepared
Zerg header under ASan/UBSan. Canonical race, unified, Showcase and explicit
legacy Z3 ARM ROM builds pass the unchanged reserve assertions. Existing
indexed sprite arrays retained by canonical regeneration match the prior
prepared bank byte-for-byte. Logs are under `.local/build/unit-word-final/`.

The full goal remains open: 30 original missions, three race ROMs, selectable
maps/all races/1–3 AIs, and performance near 30 fps. Later campaign integration,
remaining native behavior adapters, route-build hitches and physical-hardware
qualification still need work. Source-only checks run locally; Actions stays
disabled/manual-only.

Private evidence is under `.local/build/unit-raster-bench/`,
`.local/build/unit-word-qualification/`, `.local/build/unit-word-query-qualification/`
and `.local/build/z4-query-tuning-current/`.
No assets, ROMs, saves, captures or private traces are published.
