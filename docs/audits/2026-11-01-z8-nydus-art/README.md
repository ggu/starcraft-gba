# Z8 Nydus Canal source art

The recovered Z8 sprite bank had no completed-building frame for raw 134, so
`draw_building_art` used its generic rectangle fallback. The generated
construction bank already contains raw-134 stages 0–2 and their source image
frames, and the Z8 compile vector enables source construction art.

The opt-in `--include-z8-buildings` sprite-import option adds a separate
`SC64_Z8_RUNTIME`-gated Nydus frame bank and raw-134 building lookup. Existing
Z7 and other sprite-generation options remain unchanged. The normal renderer
already asks the building bank for raw 134; no gameplay or building rules are
changed. The Z8 prepared-input test target checks the actual renderer binding,
completed-frame raster and three construction-stage bindings.

From the verified USA source ROM (SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`) and pinned extractor commit
`09b57f787a247107d1de82b8c5eab2734667ea1d`, raw 134 resolves through flingy
29, sprite 174 and image 86 to GRP `00D/064` (SHA-1
`a8cd65fb894eb479dcfde5a6861520f84458b86b`). Its five frames are retained;
the source Init frame is 0, with 64×64 canvas and a 36×36 visible rectangle at
(14,11). The importer uses the existing source Init-layer composition rules;
image 87's modifier-10 shadow remains deferred, as in the established sprite
pipeline. This does not claim animated Nydus working states or shadow fidelity.

To regenerate a Z8 bank, invoke `tools/sc64_race_sprites.py` with the existing
Z8 sprite options and `--include-z8-buildings`, writing both `--out` and
`--header` under ignored local paths. The generated header and previews contain
private source art and are not committed. A stale bank without the new option
continues to use the legacy fallback until regenerated; `test-zerg-z8` checks
for the new binding.
