# Late-Zerg bulk path-grid capability

The normal Z7–Z10 release vectors inherited no `SC64_BULK_TERRAIN_GRID`, while
normal Terran, early Zerg, Protoss and Showcase already use the existing
`CANONICAL_PATH_GRID_FLAGS`. This candidate adds that shared capability to the
Z7 canonical vector, inherited by Z8–Z10. `CANONICAL_PATH_GRID=0` remains an
explicit control. It does not enable walking-word sprites, change route policy,
actor limits, timing, VRAM allocation or the scalar fallback.

On the exact 94bc packaged Z9 source and asset dependencies, an earlier paired
single-flag experiment matched all 500 selected persisted-world fingerprints,
frame advances, screens and modes from a newly created ordinary-button custom
save. The human Zerg remained passive against Terran/Zerg/Protoss AI; the
scene advanced from 46 units/17 buildings to 48/21. Mean mGBA cycles per
update were 611,691.662 control versus 606,218.422 candidate (−0.895%); the
largest call fell from 3,102,721 to 2,527,936 (−18.5%). These are specific
emulator observations, not hardware FPS, full-state proof or a complete game.
The control was the exact delivered ROM SHA-256 `d86f1e23...`; the candidate
was `c5e50ced...`. The guest backend cannot inspect framebuffer or palette.

Using isolated, hash-verified package cover and advisor inputs:

- All eight actual Z7–Z10 control/candidate normal ARM links passed unchanged
  memory guards, with paired vectors differing only in this flag.
- The existing complete-grid scalar/packed ASan/UBSan oracle passed 33
  imported-map, custom-map, procedural-arena and building-overlay cases.
  The new actual Z5 source-action fixture passed door open/close and save/load
  complete-grid parity under the Z9 vector. These are constructed HOST scenes.
- Four HOST full-clear reference oracles passed for identity-checked Z7 and Z9
  control/candidate scenes. Full indexed framebuffer and RGB555 palette traces
  matched across binaries for 91 captures per profile, including repeated page
  rendering and lifecycle changes. The Z7 trace SHA-256 was
  `296b0969d0a503d89aa849ab393ae06206b97986f6c1bbd56a9f4291269c8777`;
  Z9 was `cf7522dbefda9eb13380a352d338dca29eda09e77c90c6436725aa419c1b5943`.
- Eighteen normal-menu HOST custom openings (two maps, three human races and
  one to three AI) matched 41 sampled selected-state/display fingerprints per
  pair. These openings are not match, native N64 or hardware qualification.
- Source boundary, default build and six vector/configuration tests passed.
  The 567 isolated private input hashes and source manifests remained stable.
  The previous full asset-free suite passed on the parent runtime at `5a2177d9`
  before the docs-only rebase/merge (1,007 Python tests, 26 skips, 448.10 s);
  this focused optional-vector change did not rerun that full suite.

The original fresh recorded Z7 campaign loses under the existing baseline;
its strict validator reported 55 failures. The earlier full-field control and
candidate emitted identical traces through that loss, but no new campaign-win
claim or validator waiver follows. Exact guest pixels, broader campaign and
busy skirmish coverage, runtime stack, physical hardware behavior and the
near-30-fps objective remain open.

The prepared-input oracles have the optional `test-sc64-z9-bulk-grid` target;
`SC64_BULK_GRID_TEST_FLAGS` can prepend an independently verified include bank.
An initial target invocation tried to regenerate a prepared header and failed
because this isolated checkout does not contain the private extractor. The underlying two direct exact-vector tests had already passed. A rerun
marking verified inputs old passed both named-target tests with unchanged input
hashes. The first wrapper recorded exit 2, but its detailed output was
unintentionally overwritten by the rerun; the observed failure was a missing
`references/sc64-maps/extract_sc64_maps.py` during header regeneration.
